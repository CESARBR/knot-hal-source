/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <Arduino.h>

#include "include/avr_errno.h"
#include "include/avr_log.h"

#define SERIAL_BAUD_RATE	9600
#define LOG_BUFFER_LEN		50

static bool status_enabled =  false;

#ifdef LEONARDO // Arduino Leonardo
static Serial_* _serial = &Serial;
#else
static HardwareSerial* _serial = &Serial;
#endif

int hal_log_open(const char *pathname)
{
	if (strncmp("serial", pathname, 6) != 0)
		return -EINVAL;

	if (strlen(pathname) == 7) {
		switch(pathname[6]) {
#ifdef HAVE_HWSERIAL0
		case '0':
			_serial = &Serial;
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
		_serial->print("Serial communication enabled (");
	} else {
		_serial->print("Serial communication already enabled (");
	}
	_serial->print(SERIAL_BAUD_RATE);
	_serial->println(")");
	return 0;
}

void logger(PGM_P category, const char *format, ...)
{
	char buf[LOG_BUFFER_LEN] = {0};
	va_list args;
	char cat[8];

	strcpy_P(cat, category);

	_serial->print(cat);
	_serial->print(" ");

	va_start(args, format);
	vsnprintf(buf, LOG_BUFFER_LEN, format, args);
	va_end(args);

	_serial->println(buf);
}

void hal_log_close(void)
{
	if (status_enabled) {
		_serial->print("Serial communication disabled (");
		_serial->print(SERIAL_BAUD_RATE);
		_serial->println(")");
		_serial->end();
		status_enabled = false;
	}
}
