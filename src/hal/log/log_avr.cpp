/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <Arduino.h>

#include "avr_errno.h"
#include "log.h"

#define SERIAL_DATA_RATE 9600

static bool status_enabled =  false;

static HardwareSerial* _serial;

int hal_log_open(const char *pathname)
{
	if (strncmp("serial", pathname, 6) != 0)
		return -EINVAL;

	if (strlen(pathname) != 7)
		return -EINVAL;

	switch(pathname[6]) {
#ifdef SERIAL_PORT_HARDWARE1
	case '1':
		_serial = &Serial1;
		break;
#endif

#ifdef SERIAL_PORT_HARDWARE2
	case '2':
		_serial = &Serial2;
		break;
#endif

#ifdef SERIAL_PORT_HARDWARE3
	case '3':
		_serial = &Serial3;
		break;
#endif
	default:
		return -EINVAL;
	}

	if (!status_enabled) {
		_serial->begin(SERIAL_DATA_RATE);

		status_enabled = true;
		_serial->print("Serial communication enabled (");
	} else {
		_serial->print("Serial communication already enabled (");
	}
	_serial->print(SERIAL_DATA_RATE);
	_serial->println(")");
	return 0;
}

void logger(const char *file, const char *function, long line,
				const char *args, const char *category)
{
	_serial->print(category);
	_serial->print(file);
	_serial->print("::");
	_serial->print(function);
	_serial->print("(");
	_serial->print(line);
	_serial->print("): ");
	_serial->println(args);
}

void hal_log_close(void)
{
	if (status_enabled) {
		_serial->print("Serial communication disabled (");
		_serial->print(SERIAL_DATA_RATE);
		_serial->println(")");
		_serial->end();
		status_enabled = false;
	}
}
