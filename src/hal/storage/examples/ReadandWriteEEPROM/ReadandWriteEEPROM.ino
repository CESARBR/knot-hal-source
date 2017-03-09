/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "KNoTThing.h"
#include "include/storage.h"

#include <string.h>
#include <EEPROM.h>

#define UUID_SIZE		36
#define TOKEN_SIZE		40
#define MAC_SIZE		8
#define SCHEMA_FLAG_SIZE	1
#define PRIVATE_KEY_SIZE	32
#define PUBLIC_KEY_SIZE	64
#define FOREIGN_KEY_SIZE	64
#define TYPE_PRIVATE_KEY	0
#define TYPE_PRIVATE_KEY_OUT	1
#define TYPE_PUBLIC_KEY	2
#define TYPE_PUBLIC_KEY_OUT	3
#define TYPE_FOREIGN_KEY	4
#define TYPE_FOREIGN_KEY_OUT	5

#define CONFIG_SIZE 		sizeof(uint16_t)

#define EEPROM_SIZE		(E2END + 1)
#define ADDR_UUID		(EEPROM_SIZE - UUID_SIZE)
#define ADDR_TOKEN		(ADDR_UUID - TOKEN_SIZE)
#define ADDR_MAC		(ADDR_TOKEN - MAC_SIZE)
#define ADDR_SCHEMA_FLAG	(ADDR_MAC - SCHEMA_FLAG_SIZE)
#define ADDR_PRIVATE_KEY	(ADDR_SCHEMA_FLAG - PRIVATE_KEY_SIZE)
#define ADDR_PUBLIC_KEY	(ADDR_PRIVATE_KEY - PUBLIC_KEY_SIZE)
#define ADDR_FOREIGN_KEY	(ADDR_PUBLIC_KEY - FOREIGN_KEY_SIZE)
#define ADDR_OFFSET_CONFIG	(ADDR_FOREIGN_KEY - CONFIG_SIZE)

/* Sample values */
static char value_UUID[] = "361ff48d-c534-4ac6-a7b7-70e648a80000";
static char value_UUID_out[UUID_SIZE+1];
static char value_TOKEN[] = "ad798840028f9055e061256f3d59a150ddee045d";
static char value_TOKEN_out[TOKEN_SIZE+1];
static uint64_t mac = 0x1122334455667788;
static uint8_t value_PRIVATE_KEY[PRIVATE_KEY_SIZE] = {
	0xA8, 0x78, 0x51, 0x3A, 0x5D, 0xDA, 0x9C, 0x33, 0x35, 0xB4, 0x05, 0x06,
	 0xE1, 0x1E, 0x88, 0xD9, 0x52, 0xBF, 0x4F, 0x98, 0xF3, 0xB3, 0x0E, 0xB8,
	  0x64, 0x35, 0x0F, 0x9D, 0xB7, 0x35, 0x0C, 0xAE
};
static uint8_t value_PRIVATE_KEY_out[PRIVATE_KEY_SIZE];
static uint8_t value_PUBLIC_KEY[PUBLIC_KEY_SIZE] = {
	0x1F, 0x37, 0x66, 0xC9, 0x8A, 0xDB, 0x2D, 0x0C, 0xAF, 0x8C, 0x02, 0xE4,
	 0x47, 0x05, 0x9C, 0xB0, 0xFE, 0x6F, 0x1A, 0x40, 0x3D, 0x57, 0x70, 0x0D,
	  0x9B, 0x3F, 0x34, 0xCD, 0xD4, 0xCF, 0xDC, 0x8C, 0x36, 0x6D, 0x48,
	   0x58, 0xAD, 0x64, 0xE5, 0xE4, 0xAE, 0x9A, 0xC1, 0x49, 0xEF, 0xD9,
	    0x94, 0x75, 0xF8, 0x24, 0x7B, 0x9A, 0xCF, 0x18, 0x53, 0xF9, 0x41,
	     0xB0, 0xC5, 0x5A, 0x86, 0xB9, 0x60, 0xF4
};
static uint8_t value_PUBLIC_KEY_out[PUBLIC_KEY_SIZE];
static uint8_t value_FOREIGN_KEY[FOREIGN_KEY_SIZE] = {
	0x06, 0xE6, 0x9C, 0xFE, 0x72, 0x2A, 0xAC, 0x71, 0x55, 0xA3, 0x4A, 0x9A,
	 0xD1, 0x6B, 0x21, 0xA7, 0x51, 0xCC, 0xE8, 0xA0, 0x5C, 0x65, 0xC2, 0xA8,
	  0x77, 0xDD, 0x9D, 0x4C, 0x3D, 0xDC, 0x7A, 0x03, 0x84, 0x7E, 0x37,
	   0xC3, 0x08, 0x70, 0x17, 0xDB, 0xCD, 0xF1, 0x31, 0xD0, 0x79, 0xEF,
	    0x2E, 0xB0, 0xF2, 0x09, 0xBA, 0xDF, 0x57, 0xE8, 0xA5, 0x3D, 0x47,
	     0xE1, 0x1D, 0x42, 0x1B, 0x32, 0x3F, 0x96
};
static uint8_t value_FOREIGN_KEY_out[FOREIGN_KEY_SIZE];

static union {
	uint64_t dw;
	struct {
		uint32_t low,
		high;
	} w;
} mac_out;

typedef struct {
	uint8_t		sensor_id;
	knot_config	values;
} data_config_store;

static data_config_store conf[2];
static data_config_store conf_out[2];

static void DumpEEPROM(void)
{
	char str[50], *pstr;
	uint16_t address, index;
	byte value;

	address = 0;
	index = 0;

	while (true)
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
			address = 0;
			return;
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
}

static void testComparisons(void)
{
	/* Comparisons */
	/* UUID */
	if((memcmp(value_UUID, value_UUID_out, sizeof(value_UUID))) == 0)
		Serial.println("UUID: \t\tOk");
	else
		Serial.println("Problems reading UUID");

	/* TOKEN */
	if((memcmp(value_TOKEN, value_TOKEN_out, sizeof(value_TOKEN))) == 0)
		Serial.println("TOKEN: \t\tOk");
	else
		Serial.println("Problems reading TOKEN");

	/* MAC */
	if (mac == mac_out.dw)
		Serial.println("MAC: \t\tOk");
	else
		Serial.println("Problems reading MAC");

	/* PRIVATE KEY */
	if ((memcmp(value_PRIVATE_KEY, value_PRIVATE_KEY_out,
		 sizeof(value_PRIVATE_KEY))) == 0)
		Serial.println("PRIVATE KEY: \t\tOk");
	else
		Serial.println("Problems reading PRIVATE KEY");

	/* PUBLIC KEY */
	if ((memcmp(value_PUBLIC_KEY, value_PUBLIC_KEY_out,
		 sizeof(value_PUBLIC_KEY))) == 0)
		Serial.println("PUBLIC KEY: \t\tOk");
	else
		Serial.println("Problems reading PUBLIC KEY");

	/* FOREIGN KEY */
	if ((memcmp(value_FOREIGN_KEY, value_FOREIGN_KEY_out,
		 sizeof(value_FOREIGN_KEY))) == 0)
		Serial.println("FOREIGN KEY: \t\tOk");
	else
		Serial.println("Problems reading FOREIGN KEY");

	/* Config */
	if((memcmp(conf, conf_out, sizeof(conf))) == 0)
		Serial.println("Config: \tOk\n");
	else
		Serial.println("Problems reading Config\n");

	Serial.print("EEPROM(");
	Serial.print(EEPROM.length());
	Serial.println(") Dumping...");

 	DumpEEPROM();
}


static void testWrite_function(void)
{
	Serial.println("Unit Test Write EEPROM Functions");

	/* Write Functions */
	hal_storage_write_end(HAL_STORAGE_ID_UUID, (void *) value_UUID, sizeof(value_UUID));
	hal_storage_write_end(HAL_STORAGE_ID_TOKEN, (void *) value_TOKEN, sizeof(value_TOKEN));
	hal_storage_write_end(HAL_STORAGE_ID_MAC, &mac, sizeof(mac));
	hal_storage_write_end(HAL_STORAGE_ID_PRIVATE_KEY,
		 (void *) value_PRIVATE_KEY, sizeof(value_PRIVATE_KEY));
	hal_storage_write_end(HAL_STORAGE_ID_PUBLIC_KEY,
		 (void *) value_PUBLIC_KEY, sizeof(value_PUBLIC_KEY));
	hal_storage_write_end(HAL_STORAGE_ID_FOREIGN_KEY,
		 (void *) value_FOREIGN_KEY, sizeof(value_FOREIGN_KEY));
	hal_storage_write_end(HAL_STORAGE_ID_CONFIG, (void *) conf, sizeof(conf));

	/* Read Functions (natives) */
	eeprom_read_block(&value_UUID_out, (const void *) ADDR_UUID, sizeof(value_UUID_out));
	value_UUID_out[sizeof(value_UUID)] = '\0';

	eeprom_read_block(&value_TOKEN_out, (const void *) ADDR_TOKEN, sizeof(value_TOKEN_out));
	value_TOKEN_out[sizeof(value_TOKEN)] = '\0';

	eeprom_read_block(&mac_out, (const void *) ADDR_MAC, sizeof(mac_out));

	eeprom_read_block(&value_PRIVATE_KEY_out,
		 (const void *) ADDR_PRIVATE_KEY, sizeof(value_PRIVATE_KEY_out));

	eeprom_read_block(&value_PUBLIC_KEY_out,
		 (const void *) ADDR_PUBLIC_KEY, sizeof(value_PUBLIC_KEY_out));

	eeprom_read_block(&value_FOREIGN_KEY_out,
		 (const void *) ADDR_FOREIGN_KEY, sizeof(value_FOREIGN_KEY_out));

	eeprom_read_block(&conf_out, (const void *) (ADDR_OFFSET_CONFIG - sizeof(conf)), sizeof(conf));

	testComparisons();
}


static void testRead_function()
{
	Serial.println("Unit Test Read EEPROM Functions");

	/* White Functions (natives) */
	eeprom_write_block(&value_UUID, (void *) ADDR_UUID, sizeof(value_UUID));

	eeprom_write_block(&value_TOKEN, (void *) ADDR_TOKEN, sizeof(value_TOKEN));

	eeprom_write_block(&mac, (void *) ADDR_MAC, sizeof(mac));

	eeprom_write_block(&value_PRIVATE_KEY, (void *) ADDR_PRIVATE_KEY,
	 sizeof(value_PRIVATE_KEY));

	eeprom_write_block(&value_PUBLIC_KEY, (void *) ADDR_PUBLIC_KEY,
	 sizeof(value_PUBLIC_KEY));

	eeprom_write_block(&value_FOREIGN_KEY, (void *) ADDR_FOREIGN_KEY,
	 sizeof(value_FOREIGN_KEY));

	eeprom_write_word((uint16_t *) ADDR_OFFSET_CONFIG, sizeof(conf));
	eeprom_write_block(&conf, (void *) (ADDR_OFFSET_CONFIG - sizeof(conf)), sizeof(conf));

	/* Read Functions */
	hal_storage_read_end(HAL_STORAGE_ID_UUID, &value_UUID_out, sizeof(value_UUID));
	value_UUID_out[sizeof(value_UUID)] = '\0';

	hal_storage_read_end(HAL_STORAGE_ID_TOKEN, &value_TOKEN_out, sizeof(value_TOKEN));
	value_TOKEN_out[sizeof(value_TOKEN)] = '\0';

	hal_storage_read_end(HAL_STORAGE_ID_MAC, &mac_out, sizeof(mac));

	hal_storage_read_end(HAL_STORAGE_ID_PRIVATE_KEY,
		 (void *) value_PRIVATE_KEY_out, sizeof(value_PRIVATE_KEY_out));

	hal_storage_read_end(HAL_STORAGE_ID_PUBLIC_KEY,
		 (void *) value_PUBLIC_KEY_out, sizeof(value_PUBLIC_KEY_out));

	hal_storage_read_end(HAL_STORAGE_ID_FOREIGN_KEY,
		 (void *) value_FOREIGN_KEY_out, sizeof(value_FOREIGN_KEY_out));

	hal_storage_read_end(HAL_STORAGE_ID_CONFIG, (void *) conf_out, sizeof(conf_out));

	testComparisons();
}

void setup()
{
  Serial.begin(9600);

	conf[0].sensor_id = 30;
	conf[0].values.event_flags = KNOT_EVT_FLAG_CHANGE;
	conf[0].values.time_sec = 25;
	conf[0].values.lower_limit.val_i.value = 5;
	conf[0].values.upper_limit.val_i.value = 35;

	conf[1].sensor_id = 99;
	conf[1].values.event_flags = KNOT_EVT_FLAG_TIME;
	conf[1].values.time_sec = 11;
	conf[1].values.lower_limit.val_i.value = 7;
	conf[1].values.upper_limit.val_i.value = 16;

	testRead_function();
	testWrite_function();

}

void loop()
{

}
