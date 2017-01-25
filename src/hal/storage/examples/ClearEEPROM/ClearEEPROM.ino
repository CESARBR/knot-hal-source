/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "KNoTThing.h"
#include "include/avr_log.h"
#include <EEPROM.h>

void setup()
{
	hal_log_open("serial0");
	hal_log_info("Setting the EEPROM addresses to zero...", 1);

	for (int i = 0 ; i < EEPROM.length() ; i++) {
		EEPROM.write(i, 0);
	}

	hal_log_info("Completed", 1);
}

void loop() {}
