/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "spi_bus.h"
#include "nrf24l01.h"
#include "nrf24l01_io.h"

#define DATA_SIZE	sizeof(uint8_t)

/* Time delay in microseconds (us) */
#define TPD2STBY	5000
#define TSTBY2A		130
#define	THCEN		10

#define NRF24_ADDR_SIZE		5

static bool pipe0_open = false;
static uint8_t pipe0_address[NRF24_ADDR_SIZE];

/*
 * Send to spi transfer the read command
 * return the value that was read in reg
 */
static inline int8_t nrf24reg_read(int8_t spi_fd, uint8_t reg)
{
	uint8_t value = NRF24_NOP;

	reg = NRF24_R_REGISTER(reg);
	spi_bus_transfer(spi_fd, &reg, DATA_SIZE, &value, DATA_SIZE);
	return (int8_t)value;
}

static inline void nrf24data_read(int8_t spi_fd, uint8_t reg,
					void *pd, uint16_t len)
{
	memset(pd, NRF24_NOP, len);
	reg = NRF24_R_REGISTER(reg);
	spi_bus_transfer(spi_fd, &reg, DATA_SIZE, pd, len);
}

/*
 * Send to spi transfer the write command
 * and the data that will be written
 */
static inline void nrf24reg_write(int8_t spi_fd, uint8_t reg, uint8_t value)
{
	reg = NRF24_W_REGISTER(reg);
	spi_bus_transfer(spi_fd, &reg, DATA_SIZE, &value, DATA_SIZE);
}

static inline void nrf24data_write(int8_t spi_fd, uint8_t reg,
					void *pd, uint16_t len)
{
	reg = NRF24_W_REGISTER(reg);
	spi_bus_transfer(spi_fd, &reg, DATA_SIZE, pd, len);
}

/*
 * Send to spi transfer the write command
 * the commands are in pages 53-58 from datasheet
 */
static inline int8_t command(int8_t spi_fd, uint8_t cmd)
{
	spi_bus_transfer(spi_fd, NULL, 0, &cmd, DATA_SIZE);
	/* Return device status register */
	return (int8_t)cmd;
}

static inline int8_t command_data(int8_t spi_fd, uint8_t cmd, void *pd,
						uint16_t len)
{
	spi_bus_transfer(spi_fd, &cmd, DATA_SIZE, pd, len);
	/* Return device status register */
	return command(spi_fd, NRF24_NOP);
}

static inline void set_standby1(void)
{
	disable();
}

/* Set address in pipe */
static void set_address_pipe(int8_t spi_fd, uint8_t reg, uint8_t *pipe_addr)
{
	uint8_t addr[NRF24_ADDR_SIZE];
	int8_t len;

	/* memcpy is necessary because nrf24data_write cleans value after send */
	memcpy(addr, pipe_addr, sizeof(addr));

	switch (reg) {
	case NRF24_TX_ADDR:
	case NRF24_RX_ADDR_P0:
	case NRF24_RX_ADDR_P1:
		len = NRF24_AW_RD(nrf24reg_read(spi_fd, NRF24_SETUP_AW));
		break;
	default:
		len = DATA_SIZE;
		break;
	}

	nrf24data_write(spi_fd, reg, &addr, len);
}

/* Get address of pipe */
static void get_address_pipe(int8_t spi_fd, uint8_t pipe, uint8_t *pipe_addr)
{
	int8_t len = NRF24_AW_RD(nrf24reg_read(spi_fd, NRF24_SETUP_AW));

	switch (NRF24_RX_ADDR_PIPE(pipe)) {
	case NRF24_TX_ADDR:
	case NRF24_RX_ADDR_P0:
	case NRF24_RX_ADDR_P1:
		break;

	default:
		nrf24data_read(spi_fd, NRF24_RX_ADDR_P1, pipe_addr, len);
		len = DATA_SIZE;
		break;
	}

	nrf24data_read(spi_fd, NRF24_RX_ADDR_PIPE(pipe), pipe_addr, len);
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
	nrf24reg_write(spi_fd, NRF24_CONFIG, NRF24_CONFIG_RST);
	/* Delay to establish to operational timing of the nRF24L01 */
	delay_us(TPD2STBY);

	/* Set device to standby-I mode */
	value = nrf24reg_read(spi_fd, NRF24_CONFIG) & ~NRF24_CONFIG_MASK;
	value |= NRF24_CFG_MASK_RX_DR | NRF24_CFG_MASK_TX_DS;
	value |= NRF24_CFG_MASK_MAX_RT | NRF24_CFG_EN_CRC;
	value |= NRF24_CFG_PWR_UP | NRF24_CFG_CRCO;
	nrf24reg_write(spi_fd, NRF24_CONFIG, value);

	/* Reset channel and TX observe registers */
	nrf24reg_write(spi_fd, NRF24_RF_CH,
		nrf24reg_read(spi_fd, NRF24_RF_CH) & ~NRF24_RF_CH_MASK);
	/* Set default channel to device */
	nrf24reg_write(spi_fd, NRF24_RF_CH, NRF24_CH(NRF24_CHANNEL_DEFAULT));

	/* Set RF speed and output power */
	value = nrf24reg_read(spi_fd, NRF24_RF_SETUP) & ~NRF24_RF_SETUP_MASK;
	nrf24reg_write(spi_fd, NRF24_RF_SETUP, value |
			NRF24_RF_DR(NRF24_DATA_RATE) | NRF24_RF_PWR(tx_pwr));

	/* Set address widths */
	value = nrf24reg_read(spi_fd, NRF24_SETUP_AW) & ~NRF24_SETUP_AW_MASK;
	nrf24reg_write(spi_fd, NRF24_SETUP_AW,
			value | NRF24_AW(NRF24_ADDR_SIZE));

	/* Disable Auto Re-transmission Counter */
	nrf24reg_write(spi_fd, NRF24_SETUP_RETR,
			NRF24_RETR_ARC(NRF24_ARC_DISABLE));

	/* Disable all Auto Acknowledgment of pipes */
	nrf24reg_write(spi_fd, NRF24_EN_AA,
			nrf24reg_read(spi_fd, NRF24_EN_AA) & ~NRF24_EN_AA_MASK);

	/* Disable all RX addresses */
	nrf24reg_write(spi_fd, NRF24_EN_RXADDR,
			nrf24reg_read(spi_fd, NRF24_EN_RXADDR)
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
	nrf24reg_write(spi_fd, NRF24_FEATURE,
			(nrf24reg_read(spi_fd, NRF24_FEATURE)
			 & ~NRF24_FEATURE_MASK) | NRF24_FT_EN_DPL);

	value = nrf24reg_read(spi_fd, NRF24_DYNPD) & ~NRF24_DYNPD_MASK;
	value |= NRF24_DPL_P5 | NRF24_DPL_P4;
	value |= NRF24_DPL_P3 | NRF24_DPL_P2;
	value |= NRF24_DPL_P1 | NRF24_DPL_P0;
	nrf24reg_write(spi_fd, NRF24_DYNPD, value);

	/* Reset pending status */
	value = nrf24reg_read(spi_fd, NRF24_STATUS) & ~NRF24_STATUS_MASK;
	nrf24reg_write(spi_fd, NRF24_STATUS, value | NRF24_ST_RX_DR
		| NRF24_ST_TX_DS | NRF24_ST_MAX_RT);


	/* Reset all the FIFOs */
	command(spi_fd, NRF24_FLUSH_TX);
	command(spi_fd, NRF24_FLUSH_RX);

	return spi_fd;
}

int8_t nrf24l01_deinit(int8_t spi_fd)
{
	set_standby1();
	/* Power down the radio */
	nrf24reg_write(spi_fd, NRF24_CONFIG,
			nrf24reg_read(spi_fd, NRF24_CONFIG) &
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
 * Auto Acknowledgement enable/disable
 */
int8_t nrf24l01_set_channel(int8_t spi_fd, uint8_t ch, bool ack)
{
	uint8_t max  = NRF24_RF_DR(nrf24reg_read(spi_fd, NRF24_RF_SETUP)) ==
			NRF24_DR_2MBPS?NRF24_CH_MAX_2MBPS:NRF24_CH_MAX_1MBPS;

	if (ch != _CONSTRAIN(ch, NRF24_CH_MIN, max))
		return -1;

	set_standby1();

	if (ch != NRF24_CH(nrf24reg_read(spi_fd, NRF24_RF_CH))) {
		nrf24reg_write(spi_fd, NRF24_STATUS, NRF24_ST_RX_DR
			| NRF24_ST_TX_DS | NRF24_ST_MAX_RT);
		command(spi_fd, NRF24_FLUSH_TX);
		command(spi_fd, NRF24_FLUSH_RX);
		/* Set the device channel */
		nrf24reg_write(spi_fd, NRF24_RF_CH,
			NRF24_CH(_CONSTRAIN(ch, NRF24_CH_MIN, max)));
	}

	/* Set Auto Acknowledgment to the pipes */
	if (ack)
		nrf24reg_write(spi_fd, NRF24_EN_AA,
				nrf24reg_read(spi_fd, NRF24_EN_AA) | NRF24_EN_AA_MASK);
	else
		nrf24reg_write(spi_fd, NRF24_EN_AA,
				nrf24reg_read(spi_fd, NRF24_EN_AA) & ~NRF24_EN_AA_MASK);

	return 0;
}

/*
 * nrf24l01_set_standby:
 * set radio to standby mode, transceiver is down
 */
int8_t nrf24l01_set_standby(int8_t spi_fd)
{
	set_standby1();
	return command(spi_fd, NRF24_NOP);
}

/*
 * nrf24l01_open_pipe:
 * 0 <= pipe <= 5
 * Pipes are enabled with the bits in the EN_RXADDR
 */
int8_t nrf24l01_open_pipe(int8_t spi_fd, uint8_t pipe, uint8_t *pipe_addr)
{
	uint8_t value;

	/* Out of range? */
	if (pipe > NRF24_PIPE_MAX)
		return -1;

	/* Enable pipe */
	value = nrf24reg_read(spi_fd, NRF24_EN_RXADDR);
	if (!(value & NRF24_EN_RXADDR_PIPE(pipe))) {
		nrf24reg_write(spi_fd, NRF24_EN_RXADDR,
				value | NRF24_EN_RXADDR_PIPE(pipe));
		set_address_pipe(spi_fd, NRF24_RX_ADDR_PIPE(pipe), pipe_addr);
		/* Hold pipe0 address whether case on */
		if (pipe == NRF24_PIPE0_ADDR) {
			memcpy(pipe0_address, pipe_addr, sizeof(pipe0_address));
			pipe0_open = true;
		} else {
			/*
			 * Applied to pipe 2 to 5:
			 * when using 5 octets for addressing, 4 octets (MSB)
			 * are copied from pipe 1 access address register.
			 */
			if (pipe > NRF24_PIPE1_ADDR &&
					!(value & NRF24_EN_RXADDR_P1))
				set_address_pipe(spi_fd, NRF24_RX_ADDR_P1, pipe_addr);
		}
	}

	return 0;
}

/*
 * nrf24l01_close_pipe:
 * 0 <= pipe <= 5
 */
int8_t nrf24l01_close_pipe(int8_t spi_fd, int8_t pipe)
{
	/* Out of range? */
	if (pipe < NRF24_PIPE_MIN || pipe > NRF24_PIPE_MAX)
		return -1;

	if (nrf24reg_read(spi_fd, NRF24_EN_RXADDR) & NRF24_EN_RXADDR_PIPE(pipe)) {
		/*
		 * The data pipes are enabled with the bits in the EN_RXADDR
		 * Disable the EN_RXADDR for this pipe
		 */
		nrf24reg_write(spi_fd, NRF24_EN_RXADDR,
				nrf24reg_read(spi_fd, NRF24_EN_RXADDR)
				& ~NRF24_EN_RXADDR_PIPE(pipe));
		if (pipe == NRF24_PIPE0_ADDR)
			pipe0_open = false;
	}

	return 0;
}


/*
 * nrf24l01_set_ptx:
 * set pipe to send data;
 * the radio will be the Primary Transmitter (PTX).
 * See page 31 of nRF24L01_Product_Specification_v2_0.pdf
 */
int8_t nrf24l01_set_ptx(int8_t spi_fd, uint8_t pipe)
{
	uint8_t pipe_addr[NRF24_ADDR_SIZE];

	/* Out of range? */
	if (pipe > NRF24_PIPE_MAX)
		return -1;

	/* Switch radio to standby-1 */
	set_standby1();

	/* TX Settling */

	/*
	 * If the ack is enable on channel, enable the
	 * Automatic Re-transmission.
	 */
	if (nrf24reg_read(spi_fd, NRF24_EN_AA) & NRF24_EN_AA_MASK) {
		/*
		* Set ARC and ARD by pipe index to different
		* retry periods to reduce data collisions
		* compute ARD range: 500us <= ARD[pipe] <= 1750us
		* Each pipe is increased by 250us
		* Values for each pipe:
		* pipe 0 - 500us
		* pipe 1 - 750us
		* pipe 2 - 1000us
		* pipe 3 - 1250us
		* pipe 4 - 1500us
		* pipe 5 - 1750us
		*/
		nrf24reg_write(spi_fd, NRF24_SETUP_RETR,
			NRF24_RETR_ARD((pipe + 1))
			| NRF24_RETR_ARC(NRF24_ARC));
	} else {
		/* Disable Auto Re-transmission Count */
		nrf24reg_write(spi_fd, NRF24_SETUP_RETR,
				NRF24_RETR_ARC(NRF24_ARC_DISABLE));
	}

	/* Set pipe address from pipe register */
	memset(pipe_addr, 0, sizeof(pipe_addr));
	get_address_pipe(spi_fd, pipe, pipe_addr);
	if (nrf24reg_read(spi_fd, NRF24_SETUP_RETR) & NRF24_RETR_ARC_MASK) {
		nrf24reg_write(spi_fd, NRF24_EN_RXADDR,
			nrf24reg_read(spi_fd, NRF24_EN_RXADDR) | NRF24_EN_RXADDR_P0);
		set_address_pipe(spi_fd, NRF24_RX_ADDR_P0, pipe_addr);
	}
	set_address_pipe(spi_fd, NRF24_TX_ADDR, pipe_addr);

	/* Set PTX mode */
	nrf24reg_write(spi_fd, NRF24_STATUS,
			NRF24_ST_TX_DS | NRF24_ST_MAX_RT);
	nrf24reg_write(spi_fd, NRF24_CONFIG,
			nrf24reg_read(spi_fd, NRF24_CONFIG) & ~NRF24_CFG_PRIM_RX);

	return 0;
}

/*
 * nrf24l01_ptx_data:
 * Transfer data packet to TX FIFO and fire the transmission
 */
int8_t nrf24l01_ptx_data(int8_t spi_fd, void *pdata, uint16_t len)
{
	uint8_t st;

	if (pdata == NULL || len == 0 || len > NRF24_PAYLOAD_SIZE)
		return -1;

	/*
	 * Returns the current operating status
	 * of NRF24L01 after entering data in TX FIFO
	 * 1: TX FIFO full or 0: Available locations in TX FIFO.
	 */
	st = ST_TX_FULL(command_data(spi_fd, NRF24_W_TX_PAYLOAD,
			pdata, len));
	if (st == 0) {
		/* Trigger PTX mode */
		enable();
		delay_us(THCEN);
		set_standby1();
		delay_us(TSTBY2A-THCEN);
	}

	return (int8_t)st;
}

/*
 * nrf24l01_ptx_wait_datasent:
 * wait while data is being sent
 * check Data Sent TX FIFO interrupt
 */
int8_t nrf24l01_ptx_wait_datasent(int8_t spi_fd)
{
	uint8_t value;

	do {
		value = nrf24reg_read(spi_fd, NRF24_STATUS);
		/* Send failed: Max number of TX retransmits? */
		if (value & NRF24_ST_MAX_RT) {
			command(spi_fd, NRF24_FLUSH_TX);
			return -1;
		}
	} while(!(value & NRF24_ST_TX_DS));

	return 0;
}

/*
 * nrf24l01_set_prx:
 * set pipe to receive data;
 * the radio will be the Primary Receiver (PRX).
 */
int8_t nrf24l01_set_prx(int8_t spi_fd)
{
	set_standby1();
	/*
	 * The statement line recover pipe 0 address case
	 * Auto Acknowledgment is ON.
	 */
	if (pipe0_open)
		set_address_pipe(spi_fd, NRF24_RX_ADDR_P0, pipe0_address);
	else
		nrf24reg_write(spi_fd, NRF24_EN_RXADDR,
			nrf24reg_read(spi_fd, NRF24_EN_RXADDR)
						& ~NRF24_EN_RXADDR_P0);

	/* Set PRX mode */
	nrf24reg_write(spi_fd, NRF24_STATUS, NRF24_ST_RX_DR);
	nrf24reg_write(spi_fd, NRF24_CONFIG,
			nrf24reg_read(spi_fd, NRF24_CONFIG) | NRF24_CFG_PRIM_RX);
	/* Trigger PRX mode */
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

	if (!(nrf24reg_read(spi_fd, NRF24_FIFO_STATUS) &
						NRF24_FIFO_RX_EMPTY)) {
		pipe = NRF24_ST_RX_P_NO(nrf24reg_read(spi_fd, NRF24_STATUS));
		if (pipe > NRF24_PIPE_MAX)
			pipe = NRF24_NO_PIPE;
	}

	return (int8_t)pipe;
}

/*
 * nrf24l01_prx_data:
 * return the len of data received
 * Read data packet from RX FIFO by received data packet width
 */
int8_t nrf24l01_prx_data(int8_t spi_fd, void *pdata, uint16_t len)
{
	uint8_t rxlen = 0;

	command_data(spi_fd, NRF24_R_RX_PL_WID, &rxlen, DATA_SIZE);

	/* Note: flush RX FIFO if the value read is larger than 32 bytes.*/
	if (rxlen > NRF24_PAYLOAD_SIZE) {
		command(spi_fd, NRF24_FLUSH_RX);
		rxlen = 0;
	} else if (rxlen != 0) {
		rxlen = _MIN(len, rxlen);
		command_data(spi_fd, NRF24_R_RX_PAYLOAD, pdata, rxlen);
	}

	/* Reset Rx status */
	nrf24reg_write(spi_fd, NRF24_STATUS, NRF24_ST_RX_DR);

	return (int8_t)rxlen;
}
