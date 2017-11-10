/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <Arduino.h>
#include <stdint.h>
#include "nrf24l01_io.h"
#include "spi_bus.h"

#define CE	1

/* Time delay in microseconds (us) */
#define	TPECE2CSN		4

void delay_us(float us)
{
	delayMicroseconds(us);
}

void enable(void)
{
	PORTB |= (1 << CE);
	delayMicroseconds(TPECE2CSN);
}

void disable(void)
{
	PORTB &= ~(1 << CE);
}

int io_setup(const char *dev)
{
	/*
	* PB1 = 0 (digital PIN 9) => CE = 0
	* standby-l mode
	*/
	PORTB &= ~(1 << CE);
	/* PB1 as output */
	DDRB |= (1 << CE);
	disable();
	return spi_bus_init("");
}

void io_reset(int spi_fd)
{
	disable();
	spi_bus_deinit(spi_fd);
}
