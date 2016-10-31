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
#include <glib.h>
#include <stdbool.h>

#include "spi.h"
#include "nrf24l01.h"
#include "nrf24l01_io.h"
#include "include/time.h"

#define MAXRETRIES 20
#define PIPE0		0

static char *opt_mode = "server";
static uint8_t addr[5] = {0x6B, 0x96, 0xB6, 0xC1, 0xE7};
uint8_t bufferRx[32];

static GMainLoop *main_loop;

static int spi_fd =  -1;

static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);

}

static void printBuffer(uint8_t *buffer, ssize_t len)
{
	int index;

	for (index = 0; index < len; index++) {
		if (index % 8 == 0)
			printf("\n");

		printf("0x%02x ", buffer[index]);
	}
	printf("\n");

}

static void timeout_destroy(gpointer user_data)
{
	g_main_loop_quit(main_loop);

}

static gboolean timeout_watch_client(gpointer user_data)
{

	/*
	 * This function gets called frequently to verify
	 * if there is SPI/nRF24L01 data to be read or
	 * written. Later GPIO sys interface can be used
	 * to avoid busy loop.
	 */

	static uint32_t counter = 0;
	static int err = 0;
	int retries;
	unsigned long start;

	uint8_t buffer[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1A, 0x1B, 0x00, 0x00, 0x00, 0x00
	};

	counter++;

	buffer[28] = counter >> 24;
	buffer[29] = counter >> 16;
	buffer[30] = counter >> 8;
	buffer[31] = counter;

	printf("\nSend Message\n");

	printBuffer(buffer, 32);
	memcpy(bufferRx, buffer, sizeof(buffer));

	retries = 0;

	do {
		/* Set pipe 0 to tx enabling ACK */
		nrf24l01_set_ptx(spi_fd, PIPE0);
		/* Sends the data */
		nrf24l01_ptx_data(spi_fd, &buffer, sizeof(buffer));
		/* Waits for ACK - returns 0 on succes */
		err = nrf24l01_ptx_wait_datasent(spi_fd);

		if (err != 0)
			memcpy(buffer, bufferRx, sizeof(bufferRx));

		if (retries > MAXRETRIES) {
			printf("Could not send the message\n");
			return FALSE;
		}

		retries++;
	/* Loop until receive ACK */
	} while (err != 0);

	/* Put radio in rx mode */
	nrf24l01_set_prx(spi_fd, addr);

	printf("Waiting for response...\n");

	start = hal_time_ms();
	/*
	 * While not data available in pipe 0
	 * or not receive the messge in 5 second
	 */
	while (nrf24l01_prx_pipe_available(spi_fd) != PIPE0 &&
			!(hal_timeout(hal_time_ms(), start, 5000)))
		;

	/* If data available in pipe 0 */
	if (nrf24l01_prx_pipe_available(spi_fd) == PIPE0) {
		/* Reads the data */
		nrf24l01_prx_data(spi_fd, &buffer, sizeof(buffer));

		/* Compares the message */
		if (memcmp(buffer, bufferRx, sizeof(bufferRx)) == 0)
			printf("Correct Message Received\n");
		else {
			printf("Different Message Received\n");
			return FALSE;
		}
	} else {
		printf("Timeout\n");
		return FALSE;
	}

	return TRUE;
}

static gboolean timeout_watch_server(gpointer user_data)
{

	static int err = 0;
	static uint8_t buffer[32];

	int retries;
	uint16_t rxlen;

	/*
	 * The server receives the message from
	 * client and send back the message received.
	 */

	/* If data available in pipe 0 */
	if (nrf24l01_prx_pipe_available(spi_fd) == PIPE0) {
		printf("\nMessage Received\n");
		rxlen = nrf24l01_prx_data(spi_fd, &buffer, sizeof(buffer));
		printBuffer(buffer, rxlen);
	} else /* If no data available */
		return TRUE;

	printf("Send Message back...\n");

	memcpy(bufferRx, buffer, sizeof(buffer));

	retries = 0;

	do {
		/* Open pipe 0 enabling ACK */
		nrf24l01_set_ptx(spi_fd, PIPE0);
		/* Sends the data */
		nrf24l01_ptx_data(spi_fd, &buffer, sizeof(buffer));
		/* Waits for ACK - returns 0 on succes */
		err = nrf24l01_ptx_wait_datasent(spi_fd);

		/* If error, copy buffer again */
		if (err != 0)
			memcpy(buffer, bufferRx, sizeof(bufferRx));

		if (retries > MAXRETRIES) {
			printf("Could not send the message\n");
			return FALSE;
		}

		retries++;

	/* Loop until receive the ACK */
	} while (err != 0);

	/* Put radio in rx mode */
	nrf24l01_set_prx(spi_fd, addr);
	printf("Message sent\nWaiting message...\n");


	return TRUE;
}

static void radio_stop(void)
{
	/* Deinit the radio */
	nrf24l01_deinit(spi_fd);
}

static int radio_init(void)
{
	printf("Radio init\n");
	/* Init the nrf24l01+ */
	spi_fd = nrf24l01_init("/dev/spidev0.0", NRF24_PWR_0DBM);
	/* Set the operation channel - Channel default = 10 */
	nrf24l01_set_channel(spi_fd, NRF24_CHANNEL_DEFAULT);
	nrf24l01_set_standby(spi_fd);
	/* Open pipe zero */
	nrf24l01_open_pipe(spi_fd, PIPE0, addr, true);
	/* Put the radio in RX mode to start receive packets */
	nrf24l01_set_prx(spi_fd, addr);

	return 0;
}

static GOptionEntry options[] = {
	{ "mode", 'm', 0, G_OPTION_ARG_STRING, &opt_mode,
					"mode", "Operation mode: server or client" },
	{ NULL },
};

/*
 * First run the tool "./rpiecho -m server" to
 * enter in server mode and then, in another rpi,
 * run "./rpiecho -m client" to enter in client mode.
 */

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	int err;

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

	g_option_context_free(context);

	main_loop = g_main_loop_new(NULL, FALSE);

	printf("RPi nRF24L01 Radio test tool %s mode\n", opt_mode);
	err = radio_init();
	if (err < 0) {
		g_main_loop_unref(main_loop);
		return EXIT_FAILURE;
	}

	if (strcmp(opt_mode, "client") == 0)
		g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, 1,
			timeout_watch_client, NULL, timeout_destroy);
	else {
		printf("Waiting message...\n");
		g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, 1,
			timeout_watch_server, NULL, timeout_destroy);
	}

	g_main_loop_run(main_loop);

	radio_stop();
	g_main_loop_unref(main_loop);

	return EXIT_SUCCESS;
}
