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
#include "spi.h"

#define CE	1

void delay_us(float us)
{
	delayMicroseconds(us);
}

void delay_ms(float ms)
{
	delay(ms);
}

void enable(void)
{
	PORTB |= (1 << CE);
}

void disable(void)
{
	PORTB &= ~(1 << CE);
}

int io_setup(void)
{
	/*
	* PB1 = 0 (digital PIN 9) => CE = 0
	* standby-l mode
	*/
	PORTB &= ~(1 << CE);
	/* PB1 as output */
	DDRB |= (1 << CE);
	disable();
	spi_init("");
	return 0;
}

void io_reset(void)
{
	disable();
	spi_deinit();
}