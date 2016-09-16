/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <avr/eeprom.h>
#include "storage.h"

int hal_storage_read(uint16_t addr, uint8_t *value, uint16_t len)
{
	int i;

	/* E2END represents the last EEPROM address */
	for (i = 0; i < len; i++) {
		if((addr + i) > (E2END + 1))
			break;

		value[i] = eeprom_read_byte(addr + i);
	}

	return i;
}

int hal_storage_write(uint16_t addr, const uint8_t *value, uint16_t len)
{
	int i;

	/* E2END represents the last EEPROM address */
	for (i = 0; i < len; i++) {
		if ((addr + i) > (E2END + 1)) {
			break;

		eeprom_write_byte(addr + i, value[i]);
	}

	return i;
}
