/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <Arduino.h>

#include "hal/avr_errno.h"
#include "hal/avr_log.h"

#define SERIAL_BAUD_RATE	9600
#define LOG_BUFFER_LEN		50

static bool status_enabled =  false;

#ifdef LEONARDO // Arduino Leonardo
static Serial_* _serial = &Serial;
#else
static HardwareSerial* _serial = &Serial;
#endif

extern "C" int _hal_log_open(const char *pathname)
{
	if (strncmp("serial", pathname, 6) != 0)
		return -EINVAL;

	if (strlen(pathname) == 7) {
		switch(pathname[6]) {
#ifdef HAVE_HWSERIAL0
		case '0':
			_serial = &Serial;
			if (Serial) status_enabled = true;
			break;
#endif

#ifdef HAVE_HWSERIAL1
		case '1':
			_serial = &Serial1;
			break;
#endif

#ifdef HAVE_HWSERIAL2
		case '2':
			_serial = &Serial2;
			break;
#endif

#ifdef HAVE_HWSERIAL3
		case '3':
			_serial = &Serial3;
			break;
#endif
		default:
			return -EINVAL;
		}
	}


	if (!status_enabled) {
		_serial->begin(SERIAL_BAUD_RATE);

		status_enabled = true;
		_serial->print(F("Serial communication enabled ("));
	} else {
		_serial->print(F("Serial communication already enabled ("));
	}
	_serial->print(SERIAL_BAUD_RATE);
	_serial->println(F(")"));
	return 0;
}

extern "C" void _hal_log_str(const char *str)
{
	_serial->println(str);
}

extern "C" void _hal_log_int(int value)
{
	_serial->println(value);
}

extern "C" void _hal_log_long(long value)
{
	_serial->println(value);
}

extern "C" void _hal_log_double(double value)
{
	_serial->println(value);
}

extern "C" void _hal_log_hex(int value)
{
	_serial->println(value, HEX);
}


extern "C" void _hal_log_close(void)
{
	if (status_enabled) {
		_serial->print(F("Serial communication disabled ("));
		_serial->print(SERIAL_BAUD_RATE);
		_serial->println(F(")"));
		_serial->end();
		status_enabled = false;
	}
}
