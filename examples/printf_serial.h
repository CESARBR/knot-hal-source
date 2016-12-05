/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifndef __PRINTF_SERIAL_H__
#define __PRINTF_SERIAL_H__

#ifdef ARDUINO
#include <stdio.h>
#include <HardwareSerial.h>

static int _putchar(char ch, FILE*)
{
	Serial.write(ch);
	return ch;
}

void printf_serial_init(void)
{
	/* Initialize serial and wait for port to open: */
	Serial.begin(115200);
	while (!Serial)
		/*wait for serial port to connect. Needed for native USB*/
		(void)0;

	fdevopen(&_putchar, NULL);
}
#endif

#endif

