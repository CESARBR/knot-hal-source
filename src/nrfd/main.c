/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>

#include "hal/linux_log.h"
#include "manager.h"

#define CHANNEL_DEFAULT NRF24_CH_MIN

static GMainLoop *main_loop;

static const char *opt_cfg = "/etc/knot/gatewayConfig.json";
static const char *opt_host = NULL;
static unsigned int opt_port = 8081;
static const char *opt_spi = "/dev/spidev0.0";
static int opt_channel = -1;
static int opt_dbm = -255;
static const char *opt_nodes = "/etc/knot/keys.json";
static gboolean opt_detach = TRUE;

static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);
}

/*
 * OPTIONAL: describe the valid values ranges
 * for tx and channel
 */
static GOptionEntry options[] = {

	{ "config", 'c', 0, G_OPTION_ARG_STRING, &opt_cfg,
					"configuration file path", NULL },
	{ "host", 'h', 0, G_OPTION_ARG_STRING, &opt_host,
					"host", "Host to forward KNoT" },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &opt_port,
					"port", "Remote port" },
	{ "spi", 'i', 0, G_OPTION_ARG_STRING, &opt_spi,
					"spi", "SPI device path" },
	{ "nodes", 'f', 0, G_OPTION_ARG_STRING, &opt_nodes,
					"nodes", "Known nodes file path" },
	{ "channel", 'c', 0, G_OPTION_ARG_INT, &opt_channel,
					"channel", "Broadcast channel" },
	{ "tx", 't', 0, G_OPTION_ARG_INT, &opt_dbm,
					"tx_power",
		"TX power: transmition signal strength in dBm" },
	{ "nodetach", 'n', G_OPTION_FLAG_REVERSE,
					G_OPTION_ARG_NONE, &opt_detach,
					"Logging in foreground" },
	{ NULL },
};

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	struct stat sb;
	int err, retval = 0;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		g_printerr("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	g_option_context_free(context);

	if (stat(opt_cfg, &sb) == -1) {
		err = errno;
		g_printerr("%s: %s(%d)\n", opt_cfg, strerror(err), err);
		return EXIT_FAILURE;
	}

	if ((sb.st_mode & S_IFMT) != S_IFREG) {
		g_printerr("%s is not a regular file!\n", opt_cfg);
		return EXIT_FAILURE;
	}

	if (!opt_nodes) {
		g_printerr("Missing KNOT known nodes file!\n");
		return EXIT_FAILURE;
	}

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	main_loop = g_main_loop_new(NULL, FALSE);

	hal_log_init("nrfd", opt_detach);
	hal_log_info("KNOT HAL nrfd");

	if (opt_host)
		hal_log_error("Development mode: %s:%u", opt_host, opt_port);

	err = manager_start(opt_cfg, opt_host, opt_port, opt_spi, opt_channel,
							opt_dbm, opt_nodes);
	if (err < 0) {
		hal_log_error("manager_start(): %s(%d)", strerror(-err), -err);
		g_main_loop_unref(main_loop);
		hal_log_close();
		return EXIT_FAILURE;
	}

	/* Set user id to nobody */
	if (setuid(65534) != 0) {
		err = errno;
		hal_log_error("Set uid to nobody failed. %s(%d). Exiting...",
							strerror(err), err);
		manager_stop();
		hal_log_close();
		return EXIT_FAILURE;
	}

	if (opt_detach) {
		if (daemon(0, 0)) {
			hal_log_error("Can't start daemon!");
			retval = EXIT_FAILURE;
			goto done;
		}
	}

	g_main_loop_run(main_loop);

done:
	manager_stop();

	hal_log_error("exiting ...");
	hal_log_close();

	g_main_loop_unref(main_loop);

	return retval;
}
