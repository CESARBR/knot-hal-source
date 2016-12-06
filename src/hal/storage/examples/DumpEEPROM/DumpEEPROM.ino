/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "KNoTThing.h"
#include "include/printf_serial.h"
#include <EEPROM.h>

char str[50], *pstr;
uint16_t address = 0, index = 0;
byte value;

void setup()
{
	printf_serial_init();
	printf("EEPROM(%d) Dumping...\n", EEPROM.length());
}

void loop()
{
	value = EEPROM.read(address);

	if ((address % 32) == 0) {
		if (address != 0) {
			printf(" - ");
			for(pstr=str; index != 0; --index, ++pstr)
				printf("%c", *pstr < ' ' || *pstr > 0x7f ? '.' : *pstr);
			printf("\n");
		}

		if (address == EEPROM.length()) {
			printf("[END]\n");
			while(true);
		}

		printf("[%04d]:", address);
		index = 0;
	}

	printf(" %02X", value);
	str[index++] = value;

	address++;
}
