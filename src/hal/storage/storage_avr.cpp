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

// Address where each data is stored at the end of EEPROM
#define ADDR_UUID		0
#define ADDR_TOKEN		40
#define ADDR_SIZE_CONFIG	76
#define ADDR_CONFIG		78

// Identifier of data type to be stored
#define DATA_UUID		0
#define DATA_TOKEN		1
#define DATA_CONFIG		2

//TODO: Fix parameter order (data, len , addr)

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
		if ((addr + i) > (E2END + 1))
			break;

		eeprom_write_byte(addr + i, value[i]);
	}

	return i;
}


int hal_storage_write_end(void *value, uint16_t len, uint8_t data)
{
	/* Position where the data will be stored */
	int dst;

	/*
	 * Calculate different addresses to store the
	 * value according to the parameter passed.
	 */
	switch (data) {
	case DATA_UUID:
		dst = EE2END - len - ADDR_UUID;
		break;
	case DATA_TOKEN:
		dst = EE2END - len - ADDR_TOKEN;
		break;
	case DATA_CONFIG:
		dst = EE2END - len - ADDR_CONFIG;
		/* Store the size of the config to know where it end in the EEPROM */
		hal_storage_write(EE2END - ADDR_SIZE_CONFIG, len, 2);
		break;
	default:
		return -1;
	}

	/* Store all the block in the calculated position */
	eeprom_write_block(value, dst, len);

	return len;
}
