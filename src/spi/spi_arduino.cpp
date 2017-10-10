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
#include <SPI.h>
#include "spi.h"

/*
 * FIXME: This code works only with Arduino UNO and Pro Mini
 * It's necessary change the values of pins to work with other Arduinos
 * e.g., MEGA: CSN = 0, MOSI = 2, MISO = 3, SCK = 1.
*/
#define CSN		10			// CS pin

#define	SPEED	10000000	// 10Mbps

static int m_init = 0;

static SPIClass *pspi = &SPI;

int8_t spi_init(const char *dev)
{
	if (m_init)
		return 1;

	m_init = 1;

	/* Put CSN disable */
	digitalWrite(CSN, HIGH);

	/* CSN as output */
	pinMode(CSN, OUTPUT);

	/* Initialize SPI interface */
	pspi->begin();

	return 0;
}

void spi_deinit(int8_t spi_fd)
{
	if (m_init) {
		m_init = 0;
		/* Put CSN disable */
		digitalWrite(CSN, HIGH);
		/* Finalize SPI interface */
		pspi->end();
	}
}

int spi_transfer(int8_t spi_fd, const uint8_t *tx, int ltx,
							uint8_t *rx, int lrx)
{
	if (!m_init)
		return -1;

	/* Setup communication to device */
	pspi->beginTransaction(SPISettings(SPEED, MSBFIRST, SPI_MODE0));
	/* Put CSN enable */
	digitalWrite(CSN, LOW);

	if (tx != NULL) {
		for (; ltx != 0; --ltx) {
			pspi->transfer(*tx++);
		}
	}

	if (rx != NULL) {
		for (; lrx != 0; --lrx, ++rx) {
			*rx = pspi->transfer(*rx);
		}
	}

	/* Put CSN disable */
	digitalWrite(CSN, HIGH);
	/* Finish communication to device */
	pspi->endTransaction();

	return 0;
}
