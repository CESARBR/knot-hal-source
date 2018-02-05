/*
 * Copyright (c) 2018, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "settings.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>

static const char *config_path = "/etc/knot/gatewayConfig.json";
static const char *nodes_path = "/etc/knot/keys.json";
static const char *host = NULL;
static unsigned int port = 8081;
static const char *spi = "/dev/spidev0.0";
static int channel = -1;
static int dbm = -255;
static gboolean detach = TRUE;

/*
 * OPTIONAL: describe the valid values ranges
 * for tx and channel
 */

static GOptionEntry options_spec[] = {

	{ "config", 'c', 0, G_OPTION_ARG_STRING, &config_path,
					"configuration file path", NULL },
	{ "nodes", 'f', 0, G_OPTION_ARG_STRING, &nodes_path,
					"nodes", "Known nodes file path" },
	{ "host", 'h', 0, G_OPTION_ARG_STRING, &host,
					"host", "Host to forward KNoT" },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &port,
					"port", "Remote port" },
	{ "spi", 'i', 0, G_OPTION_ARG_STRING, &spi,
					"spi", "SPI device path" },
	{ "channel", 'C', 0, G_OPTION_ARG_INT, &channel,
					"channel", "Broadcast channel" },
	{ "tx", 't', 0, G_OPTION_ARG_INT, &dbm,
					"tx_power",
		"TX power: transmition signal strength in dBm" },
	{ "nodetach", 'n', G_OPTION_FLAG_REVERSE,
					G_OPTION_ARG_NONE, &detach,
					"Logging in foreground" },
	{ NULL },
};

static int parse_args(int argc, char* argv[], settings *settings)
{
	GOptionContext *context;
	GError *gerr = NULL;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options_spec, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		g_printerr("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	g_option_context_free(context);

	settings->config_path = config_path;
	settings->nodes_path = nodes_path;
	settings->host = host;
	settings->port = port;
	settings->spi = spi;
	settings->channel = channel;
	settings->dbm = dbm;
	settings->detach = detach;

	return EXIT_SUCCESS;
}

static int is_valid_config_file(const char *config_path)
{
	struct stat sb;

	if (stat(config_path, &sb) == -1) {
		g_printerr("%s: %s(%d)\n", config_path, strerror(errno), errno);
		return EXIT_FAILURE;
	}

	if ((sb.st_mode & S_IFMT) != S_IFREG) {
		g_printerr("%s is not a regular file!\n", config_path);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int is_valid_nodes_file(const char *nodes_path)
{
	if (!nodes_path) {
		g_printerr("Missing KNOT known nodes file!\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int get_settings(int argc, char *argv[], settings *settings)
{
	if (!parse_args(argc, argv, settings))
		return EXIT_FAILURE;

	return is_valid_config_file(settings->config_path)
		&& is_valid_nodes_file(settings->nodes_path);
}
