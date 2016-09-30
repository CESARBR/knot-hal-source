/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <glib.h>

#include "include/storage.h"

#define ADDR_LEN  7

static const char *storage_file = "/etc/knot/knot_data_storage.bin";
static const char *storage_group = "KNOT_STORAGE";

static GKeyFile *storage_open_file(void)
{
	GKeyFile *gfile = g_key_file_new();

	if (!g_key_file_load_from_file(gfile, storage_file,
					G_KEY_FILE_KEEP_COMMENTS, NULL)) {
		g_key_file_free(gfile);
		return NULL;
	}

	return gfile;
}

int hal_storage_read(uint16_t addr, uint8_t *value, uint16_t len)
{
	GKeyFile *gfile;
	int retval = -EIO;
	char *str;
	char saddr[ADDR_LEN];

	gfile = storage_open_file();
	if (gfile == NULL)
		return -EIO;

	/* The address must be converted to string, to be assigned
	 * to key of the value to be read */
	snprintf(saddr, ADDR_LEN, "0x%04X", addr);

	str = g_key_file_get_string(gfile, storage_group, saddr, NULL);

	g_key_file_free(gfile);

	if (str) {
		retval = snprintf((char *) value, len, "%s", str);
		g_free(str);
	}

	return retval;
}

int hal_storage_write(uint16_t addr, const uint8_t *value, uint16_t len)
{
	GKeyFile *gfile;
	char saddr[ADDR_LEN];

	gfile = storage_open_file();
	if (gfile == NULL)
		return -EIO;

	/* The address must be converted to string, to be assigned
	 * to key of the value to be stored */
	snprintf(saddr, ADDR_LEN, "0x%04X", addr);

	g_key_file_set_string(gfile, storage_group, saddr, (char *) value);

	if (!g_key_file_save_to_file(gfile, storage_file, NULL)) {
		g_key_file_free(gfile);
		return -EIO;
	}

	g_key_file_free(gfile);

	return len;
}
