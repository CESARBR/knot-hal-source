/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "spi.h"
#include "nrf24l01.h"
#include "nrf24l01_io.h"

/* Pipes addresses base */
#define PIPE0_ADDR_BASE 0x55aa55aa5aLL
#define PIPE1_ADDR_BASE 0xaa55aa55a5LL

typedef struct {
	uint8_t enaa,
	en_rxaddr,
	rx_addr,
	rx_pw;
} pipe_reg_t;

static const pipe_reg_t pipe_reg[] = {
	{ AA_P0, EN_RXADDR_P0, RX_ADDR_P0, RX_PW_P0 },
	{ AA_P1, EN_RXADDR_P1, RX_ADDR_P1, RX_PW_P1 },
	{ AA_P2, EN_RXADDR_P2, RX_ADDR_P2, RX_PW_P2 },
	{ AA_P3, EN_RXADDR_P3, RX_ADDR_P3, RX_PW_P3 },
	{ AA_P4, EN_RXADDR_P4, RX_ADDR_P4, RX_PW_P4 },
	{ AA_P5, EN_RXADDR_P5, RX_ADDR_P5, RX_PW_P5 }
};

typedef enum {
	UNKNOWN_MODE,
	POWER_DOWN_MODE,
	STANDBY_I_MODE,
	RX_MODE,
	TX_MODE,
	STANDBY_II_MODE,
} en_modes_t;

static en_modes_t m_mode = UNKNOWN_MODE;

static uint8_t m_pipe0_addr = NRF24L01_PIPE0_ADDR;

#define DATA_SIZE	sizeof(uint8_t)

/* Time delay in microseconds (us) */
#define TPD2STBY	5000
#define TSTBY2A		130

static inline int8_t inr(uint8_t reg)
{
	uint8_t value = NOP;

	reg = R_REGISTER(reg);
	spi_transfer(&reg, DATA_SIZE, &value, DATA_SIZE);
	return (int8_t)value;
}

static inline void inr_data(uint8_t reg, void *pd, uint16_t len)
{
	memset(pd, NOP, len);
	reg = R_REGISTER(reg);
	spi_transfer(&reg, DATA_SIZE, pd, len);
}

static inline void outr(uint8_t reg, uint8_t value)
{
	reg = W_REGISTER(reg);
	spi_transfer(&reg, DATA_SIZE, &value, DATA_SIZE);
}

static inline void outr_data(uint8_t reg, void *pd, uint16_t len)
{
	reg = W_REGISTER(reg);
	spi_transfer(&reg, DATA_SIZE, pd, len);
}

static inline int8_t command(uint8_t cmd)
{
	spi_transfer(NULL, 0, &cmd, DATA_SIZE);
	/* Return device status register */
	return (int8_t)cmd;
}

static inline int8_t command_data(uint8_t cmd, void *pd, uint16_t len)
{
	spi_transfer(&cmd, DATA_SIZE, pd, len);
	/* Return device status register */
	return command(NOP);
}

static void set_address_pipe(uint8_t reg, uint8_t pipe_addr)
{
	uint16_t len;
	uint64_t  addr = (pipe_addr == NRF24L01_PIPE0_ADDR) ?
					PIPE0_ADDR_BASE : PIPE1_ADDR_BASE;

	switch (reg) {
	case TX_ADDR:
	case RX_ADDR_P0:
	case RX_ADDR_P1:
		len = AW_RD(inr(SETUP_AW));
		break;
	default:
		len = DATA_SIZE;
	}

	addr += (pipe_addr << 4) + pipe_addr;
	outr_data(reg, &addr, len);
}

static int8_t set_standby1(void)
{
	disable();
	return 0;
}

int8_t nrf24l01_set_standby(void)
{
	set_standby1();
	return command(NOP);
}

int8_t nrf24l01_init(void)
{
	uint8_t	value;

	io_setup();

	/* Reset device in power down mode */
	outr(CONFIG, CONFIG_RST);
	/* Delay to establish to operational timing of the nRF24L01 */
	delay_us(TPD2STBY);

	/* Reset channel and TX observe registers */
	outr(RF_CH, inr(RF_CH) & ~RF_CH_MASK);
	/* Set the device channel */
	outr(RF_CH, CH(NRF24L01_CHANNEL_DEFAULT));

	/* Set RF speed and output power */
	value = inr(RF_SETUP) & ~RF_SETUP_MASK;
	outr(RF_SETUP, value | RF_DR(NRF24L01_DATA_RATE)\
						| RF_PWR(NRF24L01_POWER));

	/* Set address widths */
	value = inr(SETUP_AW) & ~SETUP_AW_MASK;
	outr(SETUP_AW, value | AW(NRF24L01_ADDR_WIDTHS));

	/* Set device to standby-I mode */
	value = inr(CONFIG) & ~CONFIG_MASK;
	value |= CFG_MASK_RX_DR | CFG_MASK_TX_DS;
	value |= CFG_MASK_MAX_RT | CFG_EN_CRC;
	value |= CFG_CRCO | CFG_PWR_UP;
	outr(CONFIG, value);

	delay_us(TPD2STBY);

	outr(SETUP_RETR, RETR_ARC(ARC_DISABLE));

	/* Disable all Auto Acknowledgment of pipes */
	outr(EN_AA, inr(EN_AA) & ~EN_AA_MASK);

	/* Disable all RX addresses */
	outr(EN_RXADDR, inr(EN_RXADDR) & ~EN_RXADDR_MASK);

	/* Enable dynamic payload to all pipes */
	outr(FEATURE, (inr(FEATURE) & ~FEATURE_MASK)\
		| FT_EN_DPL | FT_EN_ACK_PAY | FT_EN_DYN_ACK);

	value = inr(DYNPD) & ~DYNPD_MASK;
	value |= DPL_P5 | DPL_P4 | DPL_P3 | DPL_P2 | DPL_P1 | DPL_P0;

	outr(DYNPD, value);

	/* Reset pending status */
	value = inr(STATUS) & ~STATUS_MASK;
	outr(STATUS, value | ST_RX_DR | ST_TX_DS | ST_MAX_RT);

	/* Reset all the FIFOs */
	command(FLUSH_TX);
	command(FLUSH_RX);

	m_pipe0_addr = NRF24L01_PIPE0_ADDR;

	return 0;
}

int8_t nrf24l01_set_channel(uint8_t ch)
{
	uint8_t max;

	max = RF_DR(inr(RF_SETUP)) == DR_2MBPS ? CH_MAX_2MBPS : CH_MAX_1MBPS;
	if (ch != _CONSTRAIN(ch, CH_MIN, max))
		return -1;

	if (ch != CH(inr(RF_CH))) {
		set_standby1();
		outr(STATUS, ST_RX_DR | ST_TX_DS | ST_MAX_RT);
		command(FLUSH_TX);
		command(FLUSH_RX);
		/* Set the device channel */
		outr(RF_CH, CH(_CONSTRAIN(ch, CH_MIN, max)));
	}
	return 0;
}

int8_t nrf24l01_open_pipe(uint8_t pipe, uint8_t pipe_addr)
{
	pipe_reg_t rpipe;

	if (pipe > NRF24L01_PIPE_MAX || pipe_addr > NRF24L01_PIPE_ADDR_MAX)
		return -1;

	memcpy(&rpipe, &pipe_reg[pipe], sizeof(pipe_reg_t));

	if (!(inr(EN_RXADDR) & rpipe.en_rxaddr)) {
		if (rpipe.rx_addr == RX_ADDR_P0)
			m_pipe0_addr = pipe_addr;

		set_address_pipe(rpipe.rx_addr, pipe_addr);
		outr(EN_RXADDR, inr(EN_RXADDR) | rpipe.en_rxaddr);
		outr(EN_AA, inr(EN_AA) | rpipe.enaa);
	}
	return 0;
}


int8_t nrf24l01_set_ptx(uint8_t pipe_addr)
{
	set_standby1();
	set_address_pipe(RX_ADDR_P0, pipe_addr);
	set_address_pipe(TX_ADDR, pipe_addr);
	#if (NRF24L01_ARC != ARC_DISABLE)
		/*
		* Set ARC and ARD by pipe index to different
		* retry periods to reduce data collisions
		* compute ARD range: 1500us <= ARD[pipe] <= 4000us
		*/
		outr(SETUP_RETR, RETR_ARD(((pipe_addr * 2) + 5))
			| RETR_ARC(NRF24L01_ARC));
	#endif
	outr(STATUS, ST_TX_DS | ST_MAX_RT);
	outr(CONFIG, inr(CONFIG) & ~CFG_PRIM_RX);
	/* Enable and delay time to TSTBY2A timing */
	enable();
	delay_us(TSTBY2A);
	return 0;
}


int8_t nrf24l01_ptx_data(void *pdata, uint16_t len, bool ack)
{

	if (pdata == NULL || len == 0 || len > NRF24L01_PAYLOAD_SIZE)
		return -1;

	return command_data(!ack ? W_TX_PAYLOAD_NOACK : W_TX_PAYLOAD,
			pdata, len);
}

int8_t nrf24l01_ptx_wait_datasent(void)
{
	uint16_t value;

	while (!((value = inr(STATUS)) & ST_TX_DS)) {
		if (value & ST_MAX_RT) {
			outr(STATUS, ST_MAX_RT);
			command(FLUSH_TX);
			return -1;
		}
	}
	return 0;
}

int8_t nrf24l01_set_prx(void)
{
	set_standby1();
	set_address_pipe(RX_ADDR_P0, m_pipe0_addr);
	outr(STATUS, ST_RX_DR);
	outr(CONFIG, inr(CONFIG) | CFG_PRIM_RX);

	/* Enable and delay time to TSTBY2A timing */
	enable();
	delay_us(TSTBY2A);

	return 0;
}

int8_t nrf24l01_prx_pipe_available(void)
{
	uint8_t pipe = NRF24L01_NO_PIPE;

	if (!(inr(FIFO_STATUS) & FIFO_RX_EMPTY)) {
		pipe = ST_RX_P_NO(inr(STATUS));
		if (pipe > NRF24L01_PIPE_MAX)
			pipe = NRF24L01_NO_PIPE;
	}
	return (int8_t)pipe;
}

int8_t nrf24l01_prx_data(void *pdata, uint16_t len)
{
	uint16_t		rxlen = 0;

	outr(STATUS, ST_RX_DR);

	command_data(R_RX_PL_WID, &rxlen, DATA_SIZE);

	/* Note: flush RX FIFO if the value read is larger than 32 bytes. */
	if (rxlen > NRF24L01_PAYLOAD_SIZE) {
		command(FLUSH_RX);
		return 0;
	}

	if (rxlen != 0) {
		rxlen = _MIN(len, rxlen);
		command_data(R_RX_PAYLOAD, pdata, rxlen);
	}
	return (int8_t)rxlen;
}
