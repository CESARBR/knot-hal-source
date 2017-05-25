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


typedef struct {
	uint8_t enaa,
	en_rxaddr,
	rx_addr,
	rx_pw;
} pipe_reg_t;

static const pipe_reg_t pipe_reg[] = {
	{ NRF24_AA_P0, NRF24_EN_RXADDR_P0, NRF24_RX_ADDR_P0, NRF24_RX_PW_P0 },
	{ NRF24_AA_P1, NRF24_EN_RXADDR_P1, NRF24_RX_ADDR_P1, NRF24_RX_PW_P1 },
	{ NRF24_AA_P2, NRF24_EN_RXADDR_P2, NRF24_RX_ADDR_P2, NRF24_RX_PW_P2 },
	{ NRF24_AA_P3, NRF24_EN_RXADDR_P3, NRF24_RX_ADDR_P3, NRF24_RX_PW_P3 },
	{ NRF24_AA_P4, NRF24_EN_RXADDR_P4, NRF24_RX_ADDR_P4, NRF24_RX_PW_P4 },
	{ NRF24_AA_P5, NRF24_EN_RXADDR_P5, NRF24_RX_ADDR_P5, NRF24_RX_PW_P5 }
};

#define DATA_SIZE	sizeof(uint8_t)

/* Time delay in microseconds (us) */
#define TPD2STBY	5000
#define TSTBY2A		130

/* Send to spi transfer the read command
* return the value that was read in reg
*/
static inline int8_t inr(int8_t spi_fd, uint8_t reg)
{
	uint8_t value = NRF24_NOP;

	reg = NRF24_R_REGISTER(reg);
	spi_transfer(spi_fd, &reg, DATA_SIZE, &value, DATA_SIZE);
	return (int8_t)value;
}

static inline void inr_data(int8_t spi_fd, uint8_t reg, void *pd, uint16_t len)
{
	memset(pd, NRF24_NOP, len);
	reg = NRF24_R_REGISTER(reg);
	spi_transfer(spi_fd, &reg, DATA_SIZE, pd, len);
}

/* Send to spi transfer the write command
 * and the data that will be written
 */
static inline void outr(int8_t spi_fd, uint8_t reg, uint8_t value)
{
	reg = NRF24_W_REGISTER(reg);
	spi_transfer(spi_fd, &reg, DATA_SIZE, &value, DATA_SIZE);
}

static inline void outr_data(int8_t spi_fd, uint8_t reg, void *pd, uint16_t len)
{
	reg = NRF24_W_REGISTER(reg);
	spi_transfer(spi_fd, &reg, DATA_SIZE, pd, len);
}

/* Send to spi transfer the write command
* the commands are in pages 53-58 from datasheet
*/
static inline int8_t command(int8_t spi_fd, uint8_t cmd)
{
	spi_transfer(spi_fd, NULL, 0, &cmd, DATA_SIZE);
	/* Return device status register */
	return (int8_t)cmd;
}

static inline int8_t command_data(int8_t spi_fd, uint8_t cmd, void *pd,
						uint16_t len)
{
	spi_transfer(spi_fd, &cmd, DATA_SIZE, pd, len);
	/* Return device status register */
	return command(spi_fd, NRF24_NOP);
}

/* Set address in pipe */
static void set_address_pipe(int8_t spi_fd, uint8_t reg, uint8_t *pipe_addr)
{
	uint8_t addr[5];
	uint16_t len;

	/* memcpy is necessary because outr_data cleans value after send */
	memcpy(addr, pipe_addr, sizeof(addr));

	switch (reg) {
	case NRF24_TX_ADDR:
	case NRF24_RX_ADDR_P0:
	case NRF24_RX_ADDR_P1:
		len = NRF24_AW_RD(inr(spi_fd, NRF24_SETUP_AW));
		break;
	default:
		len = DATA_SIZE;
		break;
	}

	outr_data(spi_fd, reg, &addr, len);
}

/*Get address of pipe */
static uint8_t *get_address_pipe(int8_t spi_fd, uint8_t pipe)
{
	uint16_t len;
	static uint8_t pipe_addr[8];

	memset(pipe_addr, 0, sizeof(pipe_addr));
	len = NRF24_AW_RD(inr(spi_fd, NRF24_SETUP_AW));

	switch (pipe_reg[pipe].rx_addr) {
	case NRF24_TX_ADDR:
	case NRF24_RX_ADDR_P0:
	case NRF24_RX_ADDR_P1:
		break;

	default:
		inr_data(spi_fd, NRF24_RX_ADDR_P1, pipe_addr, len);
		len = DATA_SIZE;
		break;
	}

	inr_data(spi_fd, pipe_reg[pipe].rx_addr, pipe_addr, len);

	return pipe_addr;
}

static int8_t set_standby1(void)
{
	disable();
	return 0;
}

int8_t nrf24l01_set_standby(int8_t spi_fd)
{
	set_standby1();
	return command(spi_fd, NRF24_NOP);
}

/*
* nrf24l01_init:
* Init spi
* Configure the radio to data rate of 1Mbps
*/
int8_t nrf24l01_init(const char *dev, uint8_t tx_pwr)
{
	uint8_t	value;
	int8_t spi_fd;
	/* example of dev = "/dev/spidev0.0" */
	spi_fd = io_setup(dev);
	if (spi_fd < 0)
		return spi_fd;

	/* Reset device in power down mode */
	outr(spi_fd, NRF24_CONFIG, NRF24_CONFIG_RST);
	/* Delay to establish to operational timing of the nRF24L01 */
	delay_us(TPD2STBY);

	/* Reset channel and TX observe registers */
	outr(spi_fd, NRF24_RF_CH, inr(spi_fd, NRF24_RF_CH) & ~NRF24_RF_CH_MASK);
	/* Set the device channel */
	outr(spi_fd, NRF24_RF_CH, NRF24_CH(NRF24_CHANNEL_DEFAULT));

	/* Set RF speed and output power */
	value = inr(spi_fd, NRF24_RF_SETUP) & ~NRF24_RF_SETUP_MASK;
	outr(spi_fd, NRF24_RF_SETUP, value | NRF24_RF_DR(NRF24_DATA_RATE)|
			NRF24_RF_PWR(tx_pwr));

	/* Set address widths */
	value = inr(spi_fd, NRF24_SETUP_AW) & ~NRF24_SETUP_AW_MASK;
	outr(spi_fd, NRF24_SETUP_AW, value | NRF24_AW(NRF24_ADDR_WIDTHS));

	/* Set device to standby-I mode */
	value = inr(spi_fd, NRF24_CONFIG) & ~NRF24_CONFIG_MASK;
	value |= NRF24_CFG_MASK_RX_DR | NRF24_CFG_MASK_TX_DS;
	value |= NRF24_CFG_MASK_MAX_RT | NRF24_CFG_EN_CRC;
	value |= NRF24_CFG_CRCO | NRF24_CFG_PWR_UP;
	outr(spi_fd, NRF24_CONFIG, value);

	delay_us(TPD2STBY);

	/* Disable Auto Retransmit Count */
	outr(spi_fd, NRF24_SETUP_RETR, NRF24_RETR_ARC(NRF24_ARC_DISABLE));

	/* Disable all Auto Acknowledgment of pipes */
	outr(spi_fd, NRF24_EN_AA, inr(spi_fd, NRF24_EN_AA)
			& ~NRF24_EN_AA_MASK);

	/* Disable all RX addresses */
	outr(spi_fd, NRF24_EN_RXADDR, inr(spi_fd, NRF24_EN_RXADDR)
		& ~NRF24_EN_RXADDR_MASK);

	/*
	 * Features available:
	 * EN_DPL Enable dynamic payload to all pipes -> Enable
	 * EN_ACK_PAY: Enables Payload with ACK -> Disable
	 * EN_DYN_ACK: Enables the W_TX_PAYLOAD_NOACK command -> Disable
	 */

	/*
	 * A problem occurred when nrf2l01+PA+LNA communicates
	 * with nrf2l01+ (without antenna), the ack packages was lost.
	 * The feature EN_DYN_ACK was disable and the communicates now work.
	 * As EN_DYN_ACK isn't enable all the pipe will work with
	 * ack by default.
	 * The option no ack can be set in function nrf24l01_set_ptx.
	 */
	outr(spi_fd, NRF24_FEATURE, (inr(spi_fd, NRF24_FEATURE)
		& ~NRF24_FEATURE_MASK) | NRF24_FT_EN_DPL);

	value = inr(spi_fd, NRF24_DYNPD) & ~NRF24_DYNPD_MASK;
	value |= NRF24_DPL_P5 | NRF24_DPL_P4;
	value |= NRF24_DPL_P3 | NRF24_DPL_P2;
	value |= NRF24_DPL_P1 | NRF24_DPL_P0;

	outr(spi_fd, NRF24_DYNPD, value);

	/* Reset pending status */
	value = inr(spi_fd, NRF24_STATUS) & ~NRF24_STATUS_MASK;
	outr(spi_fd, NRF24_STATUS, value | NRF24_ST_RX_DR
		| NRF24_ST_TX_DS | NRF24_ST_MAX_RT);


	/* Reset all the FIFOs */
	command(spi_fd, NRF24_FLUSH_TX);
	command(spi_fd, NRF24_FLUSH_RX);

	return spi_fd;
}

int8_t nrf24l01_deinit(int8_t spi_fd)
{

	disable();
	/* Power down the radio */
	outr(spi_fd, NRF24_CONFIG, inr(spi_fd, NRF24_CONFIG) &
							~NRF24_CFG_PWR_UP);

	/* Deinit SPI and GPIO */
	io_reset(spi_fd);

	return 0;
}

/*
* nrf24l01_set_channel:
* Bandwidth < 1MHz at 250kbps
* Bandwidth < 1MHZ at 1Mbps
* Bandwidth < 2MHz at 2Mbps
* 2Mbps -> channel max is 54
* 1Mbps -> channel max is 116
*/
int8_t nrf24l01_set_channel(int8_t spi_fd, uint8_t ch)
{
	uint8_t max;

	max = NRF24_RF_DR(inr(spi_fd, NRF24_RF_SETUP)) == NRF24_DR_2MBPS ?
					NRF24_CH_MAX_2MBPS : NRF24_CH_MAX_1MBPS;

	if (ch != _CONSTRAIN(ch, NRF24_CH_MIN, max))
		return -1;

	if (ch != NRF24_CH(inr(spi_fd, NRF24_RF_CH))) {
		set_standby1();
		outr(spi_fd, NRF24_STATUS, NRF24_ST_RX_DR
			| NRF24_ST_TX_DS | NRF24_ST_MAX_RT);
		command(spi_fd, NRF24_FLUSH_TX);
		command(spi_fd, NRF24_FLUSH_RX);
		/* Set the device channel */
		outr(spi_fd, NRF24_RF_CH,
			NRF24_CH(_CONSTRAIN(ch, NRF24_CH_MIN, max)));
	}

	return 0;
}

/*
* nrf24l01_open_pipe:
* 0 <= pipe <= 5
* Pipes are enabled with the bits in the EN_RXADDR
*/
int8_t nrf24l01_open_pipe(int8_t spi_fd, uint8_t pipe, uint8_t *pipe_addr,
				bool ack)
{
	pipe_reg_t rpipe;

	memcpy(&rpipe, &pipe_reg[pipe], sizeof(rpipe));

	/* Enable pipe */
	if (!(inr(spi_fd, NRF24_EN_RXADDR) & rpipe.en_rxaddr)) {
		set_address_pipe(spi_fd, rpipe.rx_addr, pipe_addr);
		outr(spi_fd, NRF24_EN_RXADDR, inr(spi_fd, NRF24_EN_RXADDR)
			| rpipe.en_rxaddr);

		if (!ack)
			outr(spi_fd, NRF24_EN_AA, inr(spi_fd, NRF24_EN_AA)
					& ~pipe_reg[pipe].enaa);
		else
			outr(spi_fd, NRF24_EN_AA, inr(spi_fd, NRF24_EN_AA)
					| pipe_reg[pipe].enaa);
	}

	return 0;
}

/*
* nrf24l01_close_pipe:
* 0 <= pipe <= 5
*/

int8_t nrf24l01_close_pipe(int8_t spi_fd, int8_t pipe)
{
	pipe_reg_t rpipe;

	if (pipe < NRF24_PIPE_MIN || pipe > NRF24_PIPE_MAX)
		return -1;

	memcpy(&rpipe, &pipe_reg[pipe], sizeof(rpipe));

	if (inr(spi_fd, NRF24_EN_RXADDR) & rpipe.en_rxaddr) {
		/*
		* The data pipes are enabled with the bits in the EN_RXADDR
		* Disable the EN_RXADDR for this pipe
		*/
		outr(spi_fd, NRF24_EN_RXADDR, inr(spi_fd, NRF24_EN_RXADDR)
				& ~rpipe.en_rxaddr);
		/* Disable auto ack in this pipe */
		outr(spi_fd, NRF24_EN_AA, inr(spi_fd, NRF24_EN_AA)
				& ~rpipe.enaa);
	}

	return 0;
}

/* nrf24l01_set_ptx:
* set pipe to send data;
* the radio will be the Primary Transmitter (PTX).
* See page 31 of nRF24L01_Product_Specification_v2_0.pdf
*/
int8_t nrf24l01_set_ptx(int8_t spi_fd, uint8_t pipe)
{

	/* put the radio in mode standby-1 */
	set_standby1();
	/* TX Settling */

	/*
	 * If the ack is enable in this pipe is necessary enable
	 * the ack in pipe0 too. Because ack always arrive in pipe 0.
	 */
	if (inr(spi_fd, NRF24_EN_AA) & pipe_reg[pipe].enaa
				&& pipe != NRF24_PIPE0_ADDR)
		outr(spi_fd, NRF24_EN_AA, inr(spi_fd, NRF24_EN_AA)
					| NRF24_AA_P0);
	else
		outr(spi_fd, NRF24_EN_AA, inr(spi_fd, NRF24_EN_AA)
				& ~NRF24_AA_P0);

	set_address_pipe(spi_fd, NRF24_RX_ADDR_P0,
						get_address_pipe(spi_fd, pipe));
	set_address_pipe(spi_fd, NRF24_TX_ADDR, get_address_pipe(spi_fd, pipe));
	#if (NRF24_ARC != NRF24_ARC_DISABLE)
		/*
		* Set ARC and ARD by pipe index to different
		* retry periods to reduce data collisions
		* compute ARD range: 1500us <= ARD[pipe] <= 4000us
		*/
		outr(spi_fd, NRF24_SETUP_RETR,
			NRF24_RETR_ARD(((pipe * 2) + 5))
			| NRF24_RETR_ARC(NRF24_ARC));
	#endif
	outr(spi_fd, NRF24_STATUS, NRF24_ST_TX_DS | NRF24_ST_MAX_RT);
	outr(spi_fd, NRF24_CONFIG, inr(spi_fd, NRF24_CONFIG)
			& ~NRF24_CFG_PRIM_RX);
	/* Enable and delay time to TSTBY2A timing */
	enable();
	delay_us(TSTBY2A);
	return 0;
}

/* nrf24l01_ptx_data:
* TX mode Transmit Packet
* See NRF24L01 datasheet table 20. 'ack' parameter selects
* the SPI NRF24 command to be sent.
*/
int8_t nrf24l01_ptx_data(int8_t spi_fd, void *pdata, uint16_t len)
{

	if (pdata == NULL || len == 0 || len > NRF24_PAYLOAD_SIZE)
		return -1;

	/*
	 * Returns the current operating status
	 * of NRF24L01 after entering data in TX FIFO
	 * 1: TX FIFO full or 0: Available locations in TX FIFO.
	 */
	return ST_TX_STATUS(command_data(spi_fd, NRF24_W_TX_PAYLOAD,
			pdata, len));
}

/*nrf24l01_ptx_wait_datasent:
* wait while data is being sent
* check Data Sent TX FIFO interrupt
*/
int8_t nrf24l01_ptx_wait_datasent(int8_t spi_fd)
{
	uint16_t value;

	while (!((value = inr(spi_fd, NRF24_STATUS)) & NRF24_ST_TX_DS)) {
		/* if arrived in Maximum number of TX retransmits
		 * the send failed
		 */
		if (value & NRF24_ST_MAX_RT) {
			outr(spi_fd, NRF24_STATUS, NRF24_ST_MAX_RT);
			command(spi_fd, NRF24_FLUSH_TX);
			return -1;
		}
	}

	return 0;
}

/* nrf24l01_set_prx:
* set pipe to send data;
* the radio will be the Primary Receiver (PRX).
* See page 33 of nRF24L01_Product_Specification_v2_0.pdf
* Receive the value of pipe 0 addr
*/
int8_t nrf24l01_set_prx(int8_t spi_fd, uint8_t *pipe0_addr)
{
	/* The addr of pipe 0 is necessary to avoid
	 * save this value internally. If there are more than
	 * one radio using this lib, the value of pipe 0 addr
	 * can be different.
	 */
	set_standby1();
	set_address_pipe(spi_fd, NRF24_RX_ADDR_P0, pipe0_addr);
	/* RX Settings */
	outr(spi_fd, NRF24_STATUS, NRF24_ST_RX_DR);
	outr(spi_fd, NRF24_CONFIG, inr(spi_fd, NRF24_CONFIG)
			| NRF24_CFG_PRIM_RX);
	/* Enable and delay time to TSTBY2A timing */
	enable();
	delay_us(TSTBY2A);

	return 0;
}

/*
* nrf24l01_prx_pipe_available:
* Return the pipe where the first data of the RX FIFO are.
*/
int8_t nrf24l01_prx_pipe_available(int8_t spi_fd)
{
	uint8_t pipe = NRF24_NO_PIPE;

	if (!(inr(spi_fd, NRF24_FIFO_STATUS) & NRF24_FIFO_RX_EMPTY)) {
		pipe = NRF24_ST_RX_P_NO(inr(spi_fd, NRF24_STATUS));
		if (pipe > NRF24_PIPE_MAX)
			pipe = NRF24_NO_PIPE;
	}

	return (int8_t)pipe;
}

/*nrf24l01_prx_data:
* return the len of data received
* Send command to read data width for the RX FIFO
*/
int8_t nrf24l01_prx_data(int8_t spi_fd, void *pdata, uint16_t len)
{
	uint16_t rxlen = 0;

	outr(spi_fd, NRF24_STATUS, NRF24_ST_RX_DR);

	command_data(spi_fd, NRF24_R_RX_PL_WID, &rxlen, DATA_SIZE);
	/* Note: flush RX FIFO if the value read is larger than 32 bytes.*/
	if (rxlen > NRF24_PAYLOAD_SIZE) {
		command(spi_fd, NRF24_FLUSH_RX);
		return 0;
	}

	if (rxlen != 0) {
		rxlen = _MIN(len, rxlen);
		command_data(spi_fd, NRF24_R_RX_PAYLOAD, pdata, rxlen);
	}

	return (int8_t)rxlen;
}
