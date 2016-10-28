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

#include <glib.h>
#include <sys/inotify.h>

#include "nrf24l01_io.h"
#include "manager.h"

#define BUF_LEN (sizeof(struct inotify_event))
#define CHANNEL_DEFAULT NRF24_CH_MIN

static GMainLoop *main_loop;

/* temporary default configuration file path */
static const char *opt_cfg = "gatewayConfig.json";
static const char *opt_host = NULL;
static unsigned int opt_port = 9000;
static const char *opt_spi = "/dev/spidev0.0";
static uint8_t opt_channel = CHANNEL_DEFAULT;
static int opt_channel_aux = CHANNEL_DEFAULT;
static uint8_t opt_tx = NRF24_PWR_0DBM;
static int opt_tx_aux = 0;	/* 0 dBm */

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
	{ "channel", 'c', 0, G_OPTION_ARG_INT, &opt_channel_aux,
					"channel", "Broadcast channel" },
	{ "tx", 't', 0, G_OPTION_ARG_INT, &opt_tx_aux,
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

static uint8_t set_tx_input(int tx_pwr)
{
	switch (tx_pwr) {

	case 0:
		return NRF24_PWR_0DBM;

	case -6:
		return NRF24_PWR_6DBM;

	case -12:
		return NRF24_PWR_12DBM;

	case -18:
		return NRF24_PWR_18DBM;
	}

	return NRF24_PWR_0DBM;
}

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	GIOChannel *inotify_io;
	int err;
	int inotifyFD, wd;
	guint watch_id;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	/*
	 * Validate tx_pwr
	 * write Power Amplifier(PA) control in
	 * TX mode: 0 <= v <= 3 valid values
	 * and respectively to: -18dBm,
	 * -12dBm, -6 dBm and 0dBm
	 */
	opt_tx = set_tx_input(opt_tx_aux);

	/*
	 * Validate channel values
	 * channel range 0 - 125 valid values
	 */
	if (opt_channel_aux > 125 || opt_channel_aux < 0) {
		printf("Invalid channel value: %d\n", opt_channel_aux);
		return EXIT_FAILURE;
	}
	opt_channel = opt_channel_aux;

	g_option_context_free(context);

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	main_loop = g_main_loop_new(NULL, FALSE);

	printf("KNOT HAL phynrfd\n");
	if (opt_host)
		printf("Development mode: %s:%u\n", opt_host, opt_port);
	else
		printf("Native SPI mode\n");

	err = manager_start(opt_host, opt_port, opt_spi);
	if (err < 0) {
		g_main_loop_unref(main_loop);
		return EXIT_FAILURE;
	}

	/* starting inotify */
	inotifyFD = inotify_init();
	/*
	 * The path to file gatewayConfig.json with radio parameters will be
	 * received through command line in the future, this is just a temporary
	 * path as example.
	 */
	wd = inotify_add_watch(inotifyFD, "gatewayConfig.json", IN_MODIFY);
	if (wd == -1) {
		printf("Error adding watch on: gatewayConfig.json\n");
		close(inotifyFD);
		manager_stop();
		return EXIT_FAILURE;
	}

	/*Setting gio channel to watch inotify fd*/
	inotify_io = g_io_channel_unix_new(inotifyFD);
	watch_id = g_io_add_watch(inotify_io, G_IO_IN, inotify_cb, NULL);
	g_io_channel_set_close_on_unref(inotify_io, TRUE);

	g_main_loop_run(main_loop);

	g_source_remove(watch_id);
	g_io_channel_unref(inotify_io);
	 /*removing from the watch list.*/
	inotify_rm_watch(inotifyFD, wd);
	/*closing the INOTIFY instance*/
	close(inotifyFD);

	manager_stop();

	g_main_loop_unref(main_loop);

	return 0;
}
