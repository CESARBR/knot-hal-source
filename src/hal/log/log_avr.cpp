/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <Arduino.h>

#include "log.h"

#define SERIAL_DATA_RATE 9600

static bool status_enabled =  false;

int hal_log_open(const char *pathname)
{
	/*
	 * TODO: The pathname variable will be used
	 * to set the serial port of the Arduino
	 */

	if (!status_enabled) {
		Serial.begin(SERIAL_DATA_RATE);

		status_enabled = true;
		Serial.print("Serial communication enabled (");
	} else {
		Serial.print("Serial communication already enabled (");
	}
	Serial.print(SERIAL_DATA_RATE);
	Serial.println(")");

	return 0;
}

void logger(const char *file, const char *function, long line,
				const char *args, const char *category)
{
	Serial.print(category);
	Serial.print(file);
	Serial.print("::");
	Serial.print(function);
	Serial.print("(");
	Serial.print(line);
	Serial.print("): ");
	Serial.println(args);
}

void hal_log_close(void)
{
	if (status_enabled) {
		Serial.print("Serial communication disabled (");
		Serial.print(SERIAL_DATA_RATE);
		Serial.println(")");
		Serial.end();
		status_enabled = false;
	}

}
