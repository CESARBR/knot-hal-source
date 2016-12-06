/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <avr/eeprom.h>
#include "include/avr_errno.h"
#include "include/storage.h"

#define UUID_SIZE		36
#define TOKEN_SIZE		40
#define MAC_SIZE		8

#define CONFIG_SIZE 		sizeof(uint16_t)

// Address where each data is stored at the end of EEPROM
#define EEPROM_SIZE		(E2END + 1)
#define ADDR_UUID		(EEPROM_SIZE - UUID_SIZE)
#define ADDR_TOKEN		(ADDR_UUID - TOKEN_SIZE)
#define ADDR_MAC		(ADDR_TOKEN - MAC_SIZE)
#define ADDR_OFFSET_CONFIG	(ADDR_MAC - CONFIG_SIZE)

#define EEPROM_SIZE_FREE	ADDR_OFFSET_CONFIG

size_t hal_storage_read(uint16_t addr, uint8_t *value, size_t len)
{
	uint8_t *offset = (uint8_t *) addr;
	union {
		uint8_t *offset;
		uint16_t address;
	} config;
	size_t i;

	/* Safe guard to avoid reading 'protected' EEPROM area: config/uuid/token */
        config.address = eeprom_read_word((const uint16_t*) ADDR_OFFSET_CONFIG);

	/* E2END represents the last EEPROM address */
	for (i = 0; i < len && offset < config.offset; ++i, ++offset)
		value[i] = eeprom_read_byte((const uint8_t *) offset);

	return i;
}

size_t hal_storage_write(uint16_t addr, const uint8_t *value, size_t len)
{
	uint8_t *offset = (uint8_t *) addr;
	union {
		uint8_t *offset;
		uint16_t address;
	} config;
	size_t i;

	/* Safe guard to avoid writing 'protected' EEPROM area: config/uuid/token */
        config.address = eeprom_read_word((const uint16_t*) ADDR_OFFSET_CONFIG);

	/* E2END represents the last EEPROM address */
	for (i = 0; i < len && offset < config.offset; ++i, ++offset)
		eeprom_write_byte(offset, value[i]);

	return i;
}

ssize_t hal_storage_write_end(uint8_t id, void *value, size_t len)
{
	/* Position where the data will be stored */
	uint16_t dst;

	/* Calculate different addresses to store the
	 * value according to the parameter passed.
	 */
	switch (id) {
	case HAL_STORAGE_ID_UUID:
		if(len != UUID_SIZE)
			return -EINVAL;

		dst = ADDR_UUID;
		break;
	case HAL_STORAGE_ID_TOKEN:
		if(len != TOKEN_SIZE)
			return -EINVAL;

		dst = ADDR_TOKEN;
		break;
	case HAL_STORAGE_ID_MAC:
		if(len != MAC_SIZE)
			return -EINVAL;

		dst = ADDR_MAC;
		break;
	case HAL_STORAGE_ID_CONFIG:
		if(len > EEPROM_SIZE_FREE)
			return -EINVAL;

		dst = ADDR_OFFSET_CONFIG - len;
		/*
		 * Stores the offset of the config(beginning of config area),
		 * 2 bytes, to know where it end in the EEPROM.
		 */
		eeprom_write_word((uint16_t *) ADDR_OFFSET_CONFIG, dst);
		break;
	default:
		return -EINVAL;
	}

	/*Store all the block in the calculated position*/
	eeprom_write_block(value, (void *) dst, len);

	return len;
}

ssize_t hal_storage_read_end(uint8_t id, void *value, size_t len)
{
	/* Position where the data will be stored */
	uint16_t src;

	/* Calculate different addresses to read the
	 * value according to the parameter passed.
	 */
	switch (id) {
	case HAL_STORAGE_ID_UUID:
		if(len != UUID_SIZE)
			return -EINVAL;

		src = ADDR_UUID;
		break;

	case HAL_STORAGE_ID_TOKEN:
		if(len != TOKEN_SIZE)
			return -EINVAL;

		src = ADDR_TOKEN;
		break;

	case HAL_STORAGE_ID_MAC:
		if(len != MAC_SIZE)
			return -EINVAL;

		src = ADDR_MAC;
		break;

	case HAL_STORAGE_ID_CONFIG:
		/*
		 * Read the offset of the config(beginning of config area),
		 * 2 bytes, to know where it end in the EEPROM.
		 */

		src = eeprom_read_word((const uint16_t*) ADDR_OFFSET_CONFIG);
		if (len > (ADDR_OFFSET_CONFIG - src))
			return -EINVAL;

		break;

	default:
		return -EINVAL;
	}

	/* Read all the block in the calculated position */
	eeprom_read_block(value, (const void *) src, len);

	return len;
}
