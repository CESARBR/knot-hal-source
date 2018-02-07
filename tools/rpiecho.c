/*
 * Copyright (c) 2017, CESAR.
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

#include "spi_bus.h"
#include "nrf24l01.h"
#include "nrf24l01_io.h"
#include "hal/nrf24.h"
#include "hal/time.h"

#define MESSAGE "This is a test message"
#define MESSAGE_SIZE sizeof(MESSAGE)

#define NRF24_ADDR_WIDTHS		5
#define PIPE				1 // 0 broadcast, 1 to 5 data
#define PIPE_MAX			6
#define CH_BROADCAST			76
#define CH_RAW				22
#define DEV				"/dev/spidev0.0"

static char *opt_mode = "server";
static bool aack, server; //auto-ack and server/client flags
static int8_t pipe, rx_len, tx_len, tx_status, tx_pipe;
static int32_t tx_stamp, attempt;
static uint8_t count, spi_fd;
static uint8_t pipe_addr[PIPE_MAX][NRF24_ADDR_WIDTHS] = {
	{ 0x8D, 0xD9, 0xBE, 0x96, 0xDE },
	{ 0x01, 0xBE, 0xEF, 0xDE, 0x96 },
	{ 0x02, 0xBE, 0xEF, 0xDE, 0x96 },
	{ 0x03, 0xBE, 0xEF, 0xDE, 0x96 },
	{ 0x04, 0xBE, 0xEF, 0xDE, 0x96 },
	{ 0x05, 0xBE, 0xEF, 0xDE, 0x96 }
};

struct s_msg {
	uint8_t count;
	char msg[NRF24_MTU-1];
} __attribute__ ((packed));

#define MSG_SIZE (sizeof(((struct s_msg *)NULL)->count) + MESSAGE_SIZE)

union {
	char buffer[NRF24_MTU];
	struct s_msg data;
} rx;

union {
	char buffer[NRF24_MTU];
	struct s_msg data;
} tx;

static GOptionEntry options[] = {
	{ "mode", 'm', 0, G_OPTION_ARG_STRING, &opt_mode,
		"mode", "Operation mode: server or client" },
	{ "ack", 'a', 0, G_OPTION_ARG_INT, &aack,
		"ack", "Connection channel: broadcast or data(auto-ack)" },
	{ NULL },
};

/*
 * First run the tool "./rpiecho -m server" to
 * enter in server mode and then, in another rpi,
 * run "./rpiecho -m client" to enter in client mode.
 */
static void setup_radio(uint8_t spi_fd)
{
	/* Raw Setup */
	if (aack) {
		nrf24l01_open_pipe(spi_fd, PIPE, pipe_addr[0]);
		nrf24l01_set_channel(spi_fd, CH_RAW, aack);
		if (server) {
			printf("Data Server Listening\n");
			for (pipe = 0; pipe < PIPE_MAX; ++pipe)
				nrf24l01_open_pipe(spi_fd, pipe,
						pipe_addr[pipe]);
			nrf24l01_set_prx(spi_fd);
		} else {
			printf("Data Client Transmitting\n");
		}
	/* Broadcast Setup */
	} else {
		nrf24l01_open_pipe(spi_fd, 0, pipe_addr[0]);
		nrf24l01_set_channel(spi_fd, CH_BROADCAST, 0);

		if (server) {
			printf("Broadcast Server Listening\n");
			nrf24l01_set_prx(spi_fd);
		} else {
			printf("Broadcast Client Transmitting\n");
		}
	}
}

static void print_buffer(bool tx, uint8_t pipe, uint8_t len,
					uint8_t count, char *msg)
{
	if (tx)
		printf("TX");
	else
		printf("RX");
	printf("%d[%d]:", pipe, len);
	printf("(%d)", count);
	printf("%s\n", msg);
}

static uint8_t timeout_stamp(void)
{
	uint8_t timeout;

	if (aack) {
		if (server)
			timeout = 5;
		else
			timeout = 11;
	} else {
		if (server)
			timeout = 3;
		else
			timeout = 5;
	}
	return timeout;
}

static int run_server(void)
{
	/* Timeout threshold for listening/echoing state */
	uint8_t timeout = timeout_stamp();

	while (1) {
		/* Echoing */
		if (tx_len != 0 && (hal_time_ms() - tx_stamp) >
							 timeout) {
			memcpy(rx.buffer, tx.buffer, tx_len);
			nrf24l01_set_ptx(spi_fd, tx_pipe);
			if (nrf24l01_ptx_data(spi_fd, rx.buffer,
						tx_len) == 0) {
				if (nrf24l01_ptx_wait_datasent
						(spi_fd) == 0) {
					print_buffer(1, tx_pipe, tx_len,
						tx.data.count,
						tx.data.msg);
					tx_len = 0;
				}
			} else {
				printf("** TX FIFO FULL **\n");
			}
			nrf24l01_set_prx(spi_fd);
			tx_stamp = hal_time_ms();
		}
		/* Listening */
		pipe = nrf24l01_prx_pipe_available(spi_fd);
		if (pipe != NRF24_NO_PIPE || (pipe == 0 &&
						 !aack)) {
			rx_len = nrf24l01_prx_data(spi_fd, rx.buffer,
							NRF24_MTU);
			if (rx_len != 0 && (pipe < PIPE_MAX)) {
				print_buffer(0, pipe, rx_len,
					rx.data.count, rx.data.msg);
				memcpy(tx.buffer, rx.buffer, rx_len);
				tx_len = rx_len;
				tx_pipe = pipe;
				tx_stamp = hal_time_ms();
			}
		}
	}
	return 0;
}
static int run_client(void)
{
	/* Timeout threshold for listening/echoing state */
	uint8_t timeout = timeout_stamp();

	while (1) {
		/* Broadcasting */
		if ((hal_time_ms() - tx_stamp) > timeout) {
			memcpy(tx.data.msg, MESSAGE, MESSAGE_SIZE);
			tx.data.count = count;
			nrf24l01_set_ptx(spi_fd, tx_pipe);
			tx_status = nrf24l01_ptx_data(spi_fd,
						tx.buffer, MSG_SIZE);
			if (tx_status == 0) {
				tx_status =
				nrf24l01_ptx_wait_datasent(spi_fd);
				if (tx_status == 0) {
					print_buffer(1, PIPE,
					MESSAGE_SIZE, count, MESSAGE);
					attempt = 0;
					++count;
				} else
					++attempt;
			} else
				printf("** TX FIFO FULL **\n");
			nrf24l01_set_prx(spi_fd);
			tx_stamp = hal_time_ms();
		}
		/* Listening */
		pipe = nrf24l01_prx_pipe_available(spi_fd);
		if (pipe != NRF24_NO_PIPE) {
			rx_len = nrf24l01_prx_data(spi_fd, rx.buffer,
							NRF24_MTU);
			if (rx_len != 0) {
				print_buffer(0, pipe, rx_len,
					rx.data.count, rx.data.msg);
			}
		}
	}
	/* To-Do: Return possible errors */
	return 0;
}

int main(int argc, char *argv[])
{

	/* Options parsing */
	GOptionContext *context;
	GError *gerr = NULL;
	int retval;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	/* Reset config */
	tx_stamp = 0;
	attempt = 0;
	rx_len = 0;
	tx_len = 0;
	count = 0;

	/* Initialize Radio */
	spi_fd = io_setup(DEV);
	nrf24l01_init(DEV, NRF24_PWR_0DBM);
	nrf24l01_set_standby(spi_fd);

	if (strcmp(opt_mode, "server") == 0)
		server = true; /* Server Mode */
	else
		server = false; /* Client Mode */

	setup_radio(spi_fd);

	if (aack)
		tx_pipe = PIPE;
	else
		tx_pipe = 0;

	if (server)
		retval = run_server();
	else
		retval = run_client();

	/* To-Do: Check for possible errors */

	return retval;
}
