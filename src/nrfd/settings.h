/*
 * Copyright (c) 2018, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _settings {
	const char *config_path;
	const char *nodes_path;

	const char *host;
	unsigned int port;

	const char *spi;
	int channel;
	int dbm;

	int detach;
} settings;

int get_settings(int argc, char *argv[], settings *settings);

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_H__ */
