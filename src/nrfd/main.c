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
#include <sys/types.h>
#include <sys/stat.h>

#include <glib.h>
#include <sys/inotify.h>
#include <json-c/json.h>

#include "nrf24l01_io.h"
#include "include/nrf24.h"
#include "manager.h"

#define BUF_LEN (sizeof(struct inotify_event))
#define CHANNEL_DEFAULT NRF24_CH_MIN
#define MAX_NODES 5

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

/* Struct with the mac address of the known peers */
static struct nrf24_mac known_peers[MAX_NODES];

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

static int parse_nodes(json_object *jobj)
{
	int array_len;
	int i;
	json_object *obj_keys, *obj_nodes, *obj_tmp;

	if (!json_object_object_get_ex(jobj, "keys", &obj_keys))
		goto failure;

	array_len = json_object_array_length(obj_keys);
	if (array_len > MAX_NODES) {
		printf("Invalid numbers of nodes in input archive");
		goto failure;
	}
	for (i = 0; i < array_len; i++) {
		obj_nodes = json_object_array_get_idx(obj_keys, i);
		if (!json_object_object_get_ex(obj_nodes, "mac", &obj_tmp))
			goto failure;

		/* Parse mac address string into struct nrf24_mac known_peers */
		if (nrf24_str2mac(json_object_get_string(obj_tmp),
						known_peers + i) < 0)
			goto failure;
	}

	return 0;

failure:
	return -EINVAL;
}

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
	json_object *jobj;

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
	/* Load nodes' info from json file */
	jobj = json_object_from_file(opt_nodes);
	if (!jobj)
		return EXIT_FAILURE;
	/* Parse info loaded and writes it to known_peers */
	err = parse_nodes(jobj);
	/* Free mem used to parse json */
	json_object_put(jobj);
	if (err < 0)
		return EXIT_FAILURE;

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
							opt_dbm, known_peers);
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
	inotify_keys_fd = inotify_init();

	wd = inotify_add_watch(inotifyFD, opt_cfg, IN_MODIFY);
	if (wd == -1) {
		printf("Error adding watch on: %s\n", opt_cfg);
		close(inotifyFD);
		close(inotify_keys_fd);
		manager_stop();
		return EXIT_FAILURE;
	}

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
