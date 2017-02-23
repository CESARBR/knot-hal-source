/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "KNoTThing.h"
#include <EEPROM.h>

char str[50], *pstr;
uint16_t address = 0, index = 0;
byte value;

void setup()
{
	Serial.begin(9600);

	Serial.print("EEPROM(");
	Serial.print(EEPROM.length());
	Serial.println(") Dumping...");
}

void loop()
{
	value = EEPROM.read(address);

	if ((address % 32) == 0) {
		if (address != 0) {
			Serial.print(" - ");
			for(pstr=str; index != 0; --index, ++pstr) {
				if(*pstr < ' ' || *pstr > 0x7f) {
					Serial.print('.');
				} else {
					Serial.print(*pstr);
				}
			}

			Serial.println();
		}

		if (address == EEPROM.length()) {
			Serial.println("[END]\n");
			while(true);
		}

		Serial.print('[');
		if (address < 9) {
			Serial.print("000");
		} else if (address < 99) {
			Serial.print("00");
		} else if (address < 999) {
			Serial.print("0");
		}
		Serial.print(address);
		Serial.print("]:");
		index = 0;
	}

	Serial.print(" ");

	if (value < 16) {
		Serial.print("0");
	}

	Serial.print(value, HEX);
	str[index++] = value;

	address++;
}
