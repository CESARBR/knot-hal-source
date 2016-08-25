/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "spi.h"
#include "nrf24l01.h"

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

// time delay in microseconds (us)
#define TPD2STBY	5000

#define CE 25

#define DELAY_US(us)	usleep(us)

#define BCM2709_RPI2	0x3F000000

#ifdef RPI2_BOARD
#define BCM2708_PERI_BASE		BCM2709_RPI2
#elif RPI_BOARD
#define BCM2708_PERI_BASE		BCM2708_RPI
#else
#error Board identifier required to BCM2708_PERI_BASE.
#endif

#define GPIO_BASE	(BCM2708_PERI_BASE + 0x200000)
#define PAGE_SIZE	(4*1024)
#define BLOCK_SIZE	(4*1024)

#define INP_GPIO(g)	(*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3)))
#define OUT_GPIO(g)	(*(gpio+((g)/10)) |=  (1<<(((g)%10)*3)))
/// sets   bits which are 1 ignores bits which are 0
#define GPIO_SET		(*(gpio+7))
// clears bits which are 1 ignores bits which are 0
#define GPIO_CLR		(*(gpio+10))

static volatile unsigned	(*gpio);

static inline void enable(void)
{
	GPIO_SET = (1<<CE);
}

static inline void disable(void)
{
	GPIO_CLR = (1<<CE);
}

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

static inline int8_t command(uint8_t cmd)
{
	spi_transfer(NULL, 0, &cmd, DATA_SIZE);
	// return device status register
	return (int8_t)cmd;
}


static void io_setup(void)
{
	//open /dev/mem
	int mem_fd = open("/dev/mem", O_RDWR|O_SYNC);

	if (mem_fd < 0) {
		printf("can't open /dev/mem\n");
		exit(-1);
	}

	gpio = (volatile unsigned*)mmap(NULL,
						BLOCK_SIZE,
						PROT_READ | PROT_WRITE,
						MAP_SHARED,
						mem_fd,
						GPIO_BASE);
	close(mem_fd);
	if (gpio == MAP_FAILED) {
		printf("mmap error\n");
		exit(-1);
	}

	GPIO_CLR = (1<<CE);
	INP_GPIO(CE);
	OUT_GPIO(CE);

	disable();
	spi_init("/dev/spidev0.0");
}

int8_t nrf24l01_init(void)
{
	uint8_t	value;

	io_setup();

	// reset device in power down mode
	outr(CONFIG, CONFIG_RST);
	// Delay to establish to operational timing of the nRF24L01
	DELAY_US(TPD2STBY);
	m_mode = POWER_DOWN_MODE;

	// reset channel and TX observe registers
	outr(RF_CH, inr(RF_CH) & ~RF_CH_MASK);
	// Set the device channel
	outr(RF_CH, CH(NRF24L01_CHANNEL_DEFAULT));

	// set RF speed and output power
	value = inr(RF_SETUP) & ~RF_SETUP_MASK;
	outr(RF_SETUP, value | RF_DR(NRF24L01_DATA_RATE)\
						| RF_PWR(NRF24L01_POWER));

	// set address widths
	value = inr(SETUP_AW) & ~SETUP_AW_MASK;
	outr(SETUP_AW, value | AW(NRF24L01_ADDR_WIDTHS));

	// set device to standby-I mode
	value = inr(CONFIG) & ~CONFIG_MASK;
	value |= CFG_MASK_RX_DR | CFG_MASK_TX_DS;
	value |= CFG_MASK_MAX_RT | CFG_EN_CRC;
	value |= CFG_CRCO | CFG_PWR_UP;
	outr(CONFIG, value);

	DELAY_US(TPD2STBY);
	m_mode = STANDBY_I_MODE;

	outr(SETUP_RETR, RETR_ARC(ARC_DISABLE));

	// disable all Auto Acknowledgment of pipes
	outr(EN_AA, inr(EN_AA) & ~EN_AA_MASK);

	// disable all RX addresses
	outr(EN_RXADDR, inr(EN_RXADDR) & ~EN_RXADDR_MASK);

	// enable dynamic payload to all pipes
	outr(FEATURE, (inr(FEATURE) & ~FEATURE_MASK)\
		| FT_EN_DPL | FT_EN_ACK_PAY | FT_EN_DYN_ACK);

	value = inr(DYNPD) & ~DYNPD_MASK;
	value |= DPL_P5 | DPL_P4 | DPL_P3 | DPL_P2 | DPL_P1 | DPL_P0;

	outr(DYNPD, value);

	// reset pending status
	value = inr(STATUS) & ~STATUS_MASK;
	outr(STATUS, value | ST_RX_DR | ST_TX_DS | ST_MAX_RT);


	// reset all the FIFOs
	command(FLUSH_TX);
	command(FLUSH_RX);

	m_pipe0_addr = NRF24L01_PIPE0_ADDR;

	return 0;
}
