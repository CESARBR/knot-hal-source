/*
 * Copyright (c) 2018, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

struct settings {
	const char *config_path;
	const char *nodes_path;

	const char *host;
	unsigned int port;

	const char *spi;
	int channel;
	int dbm;

	int detach;
} settings;

int settings_parse(int argc, char *argv[], struct settings *settings);
