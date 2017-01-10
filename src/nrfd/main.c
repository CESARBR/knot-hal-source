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
#include <sys/inotify.h>

#include "manager.h"

#define BUF_LEN (sizeof(struct inotify_event))
#define CHANNEL_DEFAULT NRF24_CH_MIN

static GMainLoop *main_loop;

static const char *opt_cfg = "/etc/knot/gatewayConfig.json";
static const char *opt_host = NULL;
static unsigned int opt_port = 9000;
static const char *opt_spi = "/dev/spidev0.0";
static int opt_channel = -1;
static int opt_dbm = -255;
static const char *opt_nodes = "/etc/knot/keys.json";

static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);
}

/*
 * OPTIONAL: describe the valid values ranges
 * for tx and channel
 */
static GOptionEntry options[] = {

	{ "config", 'f', 0, G_OPTION_ARG_STRING, &opt_cfg,
					"configuration file path", NULL },
	{ "host", 'h', 0, G_OPTION_ARG_STRING, &opt_host,
					"host", "Host exposing nRF24L01 SPI" },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &opt_port,
					"port", "Remote port" },
	{ "spi", 'i', 0, G_OPTION_ARG_STRING, &opt_spi,
					"spi", "SPI device path" },
	{ "nodes", 'n', 0, G_OPTION_ARG_STRING, &opt_nodes,
					"nodes", "Known nodes file path" },
	{ "channel", 'c', 0, G_OPTION_ARG_INT, &opt_channel,
					"channel", "Broadcast channel" },
	{ "tx", 't', 0, G_OPTION_ARG_INT, &opt_dbm,
					"tx_power",
		"TX power: transmition signal strength in dBm" },
	{ NULL },
};

static gboolean inotify_cb(GIOChannel *gio, GIOCondition condition,
								gpointer data)
{
	int inotifyFD = g_io_channel_unix_get_fd(gio);
	char buf[BUF_LEN];
	ssize_t numRead;
	const struct inotify_event *event;

	numRead = read(inotifyFD, buf, BUF_LEN);
	if (numRead == -1)
		return FALSE;

	/*Process the event returned from read()*/
	event = (struct inotify_event *) buf;
	if (event->mask & IN_MODIFY)
		g_main_loop_quit(main_loop);

	return TRUE;
}

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	GIOChannel *inotify_io;
	GIOChannel *inotify_keys_io;
	struct stat sb;
	int err;
	int inotifyFD, wd;
	int inotify_keys_fd, wd_keys;
	guint watch_id;
	guint watch_keys_id;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	g_option_context_free(context);

	if (stat(opt_cfg, &sb) == -1) {
		err = errno;
		printf("%s: %s(%d)\n", opt_cfg, strerror(err), err);
		return EXIT_FAILURE;
	}

	if ((sb.st_mode & S_IFMT) != S_IFREG) {
		printf("%s is not a regular file!\n", opt_cfg);
		return EXIT_FAILURE;
	}

	if (!opt_nodes) {
		printf("Missing KNOT known nodes file!\n");
		return EXIT_FAILURE;
	}

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	main_loop = g_main_loop_new(NULL, FALSE);

	printf("KNOT HAL phynrfd\n");
	if (opt_host)
		printf("Development mode: %s:%u\n", opt_host, opt_port);
	else
		printf("Native SPI mode\n");

	err = manager_start(opt_cfg, opt_host, opt_port, opt_spi, opt_channel,
							opt_dbm, opt_nodes);
	if (err < 0) {
		g_main_loop_unref(main_loop);
		return EXIT_FAILURE;
	}

	/* Set user id to nobody */
	if (setuid(65534) != 0) {
		err = errno;
		printf("Set uid to nobody failed. %s(%d). Exiting...\n",
							strerror(err), err);
		manager_stop();
		return EXIT_FAILURE;
	}

	/* Starting inotify */
	inotifyFD = inotify_init();
	wd = inotify_add_watch(inotifyFD, opt_cfg, IN_MODIFY);
	if (wd == -1) {
		printf("Error adding watch on: %s\n", opt_cfg);
		close(inotifyFD);
		manager_stop();
		return EXIT_FAILURE;
	}
	/* Starting inotify for known peers file*/
	inotify_keys_fd = inotify_init();
	wd_keys = inotify_add_watch(inotify_keys_fd, opt_nodes, IN_MODIFY);
	if (wd_keys == -1) {
		printf("Error adding watch on: %s\n", opt_nodes);
		inotify_rm_watch(inotifyFD, wd);
		close(inotifyFD);
		close(inotify_keys_fd);
		manager_stop();
		return EXIT_FAILURE;
	}

	/* Setting gio channel to watch inotify fd*/
	inotify_io = g_io_channel_unix_new(inotifyFD);
	inotify_keys_io = g_io_channel_unix_new(inotify_keys_fd);
	watch_id = g_io_add_watch(inotify_io, G_IO_IN, inotify_cb, NULL);
	watch_keys_id = g_io_add_watch(inotify_keys_io, G_IO_IN, inotify_cb,
									 NULL);
	g_io_channel_set_close_on_unref(inotify_io, TRUE);
	g_io_channel_set_close_on_unref(inotify_keys_io, TRUE);
	g_main_loop_run(main_loop);

	g_source_remove(watch_id);
	g_source_remove(watch_keys_id);
	g_io_channel_unref(inotify_io);
	g_io_channel_unref(inotify_keys_io);
	/* Removing from the watch list.*/
	inotify_rm_watch(inotifyFD, wd);
	inotify_rm_watch(inotify_keys_fd, wd_keys);
	/* Closing the INOTIFY instance */
	close(inotifyFD);
	close(inotify_keys_fd);

	manager_stop();

	g_main_loop_unref(main_loop);

	return 0;
}
