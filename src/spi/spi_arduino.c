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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "spi.h"

#define CSN		2
#define MOSI	3
#define MISO	4
#define SCK		5

#define DELAY_US	5	// CSN delay in microseconds

static bool	m_init = false;

int spi_init(const char *dev)
{
	if (!m_init) {

		m_init = true;

		//Put CSN HIGH
		PORTB |= (1 << CSN);

		//CSN as output
		DDRB |= (1 << CSN);

		//Enable SPI and set as master
		SPCR |= (1 << SPE) | (1 << MSTR);

		//MISO as input, MOSI and SCK as output
		DDRB &= ~(1 << MISO);
		DDRB |= (1 << MOSI);
		DDRB |= (1 << SCK);

		//SPI mode 0
		SPCR &= ~(1 << CPOL);
		SPCR &= ~(1 << CPHA);

		//vel fosc/16 = 1MHz
		SPCR &= ~(1 << SPI2X);
		SPCR &= ~(1 << SPR1);
		SPCR |= (1 << SPR0);
	}
	return 0;
}

void spi_deinit(void)
{
	if (m_init) {

		m_init = false;
		PORTB |= (1 << CSN);
		//Disable SPI and reset master
		SPCR &= ~((1 << SPE) | (1 << MSTR));
	}
}
