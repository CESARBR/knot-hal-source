/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include "spi.h"

/*
 * FIXME: This code works only with Arduino UNO e Pro Mini
 * It's necessary change the values of pins to work with other Arduinos
 * e.g., MEGA: CSN = 0, MOSI = 2, MISO = 3, SCK = 1.
*/
#define CSN		2
#define MOSI		3
#define MISO		4
#define SCK		5

/* CSN delay in microseconds */
#define DELAY_US	5

static int m_init = 0;

int8_t spi_init(const char *dev)
{
	if (m_init)
		return 0;

	m_init = 1;

	/* Put CSN HIGH */
	PORTB |= (1 << CSN);

	/* CSN as output */
	DDRB |= (1 << CSN);

	/* Enable SPI and set as master */
	SPCR |= (1 << SPE) | (1 << MSTR);

	/* MISO as input, MOSI and SCK as output */
	DDRB &= ~(1 << MISO);
	DDRB |= (1 << MOSI);
	DDRB |= (1 << SCK);

	/* SPI mode 0 */
	SPCR &= ~(1 << CPOL);
	SPCR &= ~(1 << CPHA);

	/* vel fosc/16 = 1MHz */
	SPCR &= ~(1 << SPI2X);
	SPCR &= ~(1 << SPR1);
	SPCR |= (1 << SPR0);

	return 0;
}

void spi_deinit(int8_t spi_fd)
{
	if (m_init) {

		m_init = 0;
		PORTB |= (1 << CSN);

		/* Disable SPI and reset master */
		SPCR &= ~((1 << SPE) | (1 << MSTR));
	}
}

int spi_transfer(int8_t spi_fd, const uint8_t *tx, int ltx, uint8_t *rx,
			int lrx)
{
	const uint8_t *tpd;
	uint8_t *rpd;

	if (!m_init)
		return -1;

	PORTB &= ~(1 << CSN);
	_delay_us(DELAY_US);

	if (tx != NULL && ltx != 0) {

		for (tpd = tx; ltx != 0; --ltx, ++tpd) {
			SPDR = *tpd;
			asm volatile("nop");
			while (!(SPSR & (1 << SPIF)));
			SPDR;
		}
	}

	if (rx != NULL && lrx != 0) {

		for (rpd = rx; lrx != 0; --lrx, ++rpd) {
			SPDR = *rpd;
			asm volatile("nop");
			while (!(SPSR & (1 << SPIF)));
			*rpd = SPDR;
		}
	}

	PORTB |= (1 << CSN);

	return 0;
}
