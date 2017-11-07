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
#include "hal/time.h"

#define MESSAGE "This is a test message"
#define MESSAGE_SIZE sizeof(MESSAGE)

#define NRF24_MTU			32
#define CH_BROADCAST		76
#define MAX_RT				50
#define DEV					"/dev/spidev0.0"

static char *opt_mode = "server";
static int32_t tx_stamp;
static uint8_t rx_buffer[NRF24_MTU], tx_buffer[NRF24_MTU];
static uint8_t spi_fd;
static uint8_t broadcast_addr[5] = {0x8D, 0xD9, 0xBE, 0x96, 0xDE};
static int8_t rx_len;

static GOptionEntry options[] = {
	{ "mode", 'm', 0, G_OPTION_ARG_STRING, &opt_mode,
				"mode", "Operation mode: server or client"},
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

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	tx_stamp = 0;
	rx_len = 0;
	spi_fd = io_setup(DEV);
	nrf24l01_init(DEV, NRF24_PWR_0DBM);
	nrf24l01_set_standby(spi_fd);
	nrf24l01_open_pipe(spi_fd, 0, broadcast_addr);
	nrf24l01_set_channel(spi_fd, CH_BROADCAST, 0);
	if (strcmp(opt_mode, "server") == 0)
		nrf24l01_set_prx(spi_fd);


	if (strcmp(opt_mode, "server") == 0) {
		/*Server-side Code*/
		printf("Echo Server Listening\n");
		while (1) {
			if (rx_len != 0 && (hal_time_ms() - tx_stamp) > 3) {
				memcpy(tx_buffer, rx_buffer, rx_len);
				nrf24l01_set_ptx(spi_fd, 0);
				if (nrf24l01_ptx_data(spi_fd, tx_buffer,
								rx_len) == 0)
					nrf24l01_ptx_wait_datasent(spi_fd);
				nrf24l01_set_prx(spi_fd);
				tx_stamp = hal_time_ms();
			}

			if (nrf24l01_prx_pipe_available(spi_fd) == 0) {
				rx_len = nrf24l01_prx_data(spi_fd,
					rx_buffer, NRF24_MTU);
				if (rx_len != 0) {
					printf("RX[%d]:'%s'\n",
						rx_len, rx_buffer);
					memcpy(rx_buffer, MESSAGE,
								MESSAGE_SIZE-1);
				}
			}
		}

	} else {
		/*Client-side Code*/
		printf("Echo Client Broadcasting\n");
		while (1) {
			if ((hal_time_ms() - tx_stamp) > 5) {
				memcpy(tx_buffer, MESSAGE, MESSAGE_SIZE);
				nrf24l01_set_ptx(spi_fd, 0);
				if (nrf24l01_ptx_data(spi_fd,
						tx_buffer, MESSAGE_SIZE) == 0)
					nrf24l01_ptx_wait_datasent(spi_fd);
				tx_stamp = hal_time_ms();
				nrf24l01_set_prx(spi_fd);
			}

			if (nrf24l01_prx_pipe_available(spi_fd) == 0)
				rx_len = nrf24l01_prx_data(spi_fd,
					rx_buffer, NRF24_MTU);
			if (rx_len != 0) {
				printf("TX[%d]:'%s'; RX[%d]:'%s'\n",
					(int)MESSAGE_SIZE, MESSAGE,
							rx_len, rx_buffer);
				printf("Broadcasting...\n");
			}
		}
	}

	return 1;
}
