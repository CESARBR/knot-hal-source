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

int hal_log_open(const char *pathname)
{
	/*
	 * TODO: The pathname variable will be used
	 * to set the serial port of the Arduino
	 */
	Serial.begin(9600);
	Serial.println("Serial communication enabled (9600)");

	return 0;
}

void logger(const char *file, const char *function, long line,
				const char *args, const char *category);
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
	Serial.println("Serial communication disabled (9600)");
	Serial.end();
}
