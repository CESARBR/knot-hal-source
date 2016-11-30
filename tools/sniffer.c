/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <glib.h>

#include "phy_driver.h"
#include "nrf24l01.h"
#include "nrf24l01_ll.h"
#include "phy_driver_nrf24.h"

static char *opt_mode = "mgmt";

static GMainLoop *main_loop;

int cli_fd;

static int channel_mgmt = 20;
static int channel_raw = 10;

/* Access Address for each pipe */
static uint8_t aa_pipes[2][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x35, 0x96, 0xB6, 0xC1, 0x6B},
};

static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);
}


static void listen_raw(void)
{

}

static void listen_mgmt(void)
{

}

static GOptionEntry options[] = {
	{ "mode", 'm', 0, G_OPTION_ARG_STRING, &opt_mode,
					"mode", "Operation mode: server or client" },
	{ NULL },
};


int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	struct addr_pipe adrrp;

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	cli_fd = phy_open("NRF0");
	if (cli_fd < 0) {
		printf("error open");
		return EXIT_FAILURE;
	}

	if (cli_fd < 0) {
		g_main_loop_unref(main_loop);
		return -1;
	}

	g_option_context_free(context);

	main_loop = g_main_loop_new(NULL, FALSE);

	printf("Sniffer nrfd knot %s\n", opt_mode);

	/* Sniffer in broadcast channel*/
	if (strcmp(opt_mode, "mgmt") == 0){
		printf("listen mgmt\n");
		/* Set Channel */
		phy_ioctl(cli_fd, NRF24_CMD_SET_CHANNEL, &channel_mgmt);
		adrrp.pipe = 0;
		adrrp.ack = false;
		memcpy(adrrp.aa, aa_pipes[0], sizeof(aa_pipes[0]));
		phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);
		listen_mgmt();

	}else{
		/*Sniffer in data channel*/
		printf("listen raw\n");
		/* Set Channel */
		phy_ioctl(cli_fd, NRF24_CMD_SET_CHANNEL, &channel_raw);
		/* Open pipe zero */
		adrrp.pipe = 0;
		adrrp.ack = false;
		memcpy(adrrp.aa, aa_pipes[0], sizeof(aa_pipes[0]));
		phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);
		/* Open pipe 1 */
		adrrp.pipe = 1;
		adrrp.ack = false;
		memcpy(adrrp.aa, aa_pipes[1], sizeof(aa_pipes[1]));
		phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);
		listen_raw();
	}

	g_main_loop_run(main_loop);


	g_main_loop_unref(main_loop);

	return 0;
}
