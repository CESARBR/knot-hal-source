/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "comm_private.h"
#include "nrf24l01.h"

static int nrf24l01_probe(void)
{
	return nrf24l01_init();
}

static void nrf24l01_remove(void)
{

}

static int nrf24l01_open(const char *pathname)
{
	/*
	 * Considering 16-bits to address adapter and logical
	 * channels. The most significant 4-bits will be used to
	 * address the local adapter, the remaining (12-bits) will
	 * used to address logical channels(clients/pipes).
	 * eg: For nRF24 '0' will be mapped to pipe0.
	 * TODO: Implement addressing
	 */

	return 0;
}

static ssize_t nrf24l01_recv(int sockfd, void *buffer, size_t len)
{
	ssize_t length = -1;

	/* TODO: check if the data received is fragmented or not */

	/* If the pipe available */
	if (nrf24l01_prx_pipe_available() == sockfd)
		/* Copy data to buffer */
		length = nrf24l01_prx_data(buffer, len);

	/*
	 * On success, the number of bytes read is returned
	 * Otherwise, -1 is returned.
	 */
	return length;
}

static ssize_t nrf24l01_send(int sockfd, const void *buffer, size_t len)
{
	int err;

	/* TODO: break the buffer in parts if the len > NRF24_MTU*/
	if (len > NRF24_PAYLOAD_SIZE)
		return -EINVAL;

	/* Puts the radio in TX mode - sockfd address */
	nrf24l01_set_ptx(sockfd);
	/* Transmits the data disabling Acknowledgment */
	err = nrf24l01_ptx_data((void *)buffer, len, false);

	if (err == NRF24_TX_FIFO_FULL)
		return -EAGAIN;
	/*
	 * The radio do not receive and send at the same time
	 * It's a good practice to put the radio in RX mode
	 * and only switch to TX mode when transmitting data.
	 */

	nrf24l01_set_prx();

	/*
	 * On success, the number of bytes written is returned
	 * Otherwise, -1 is returned.
	 */
	return len;
}

struct phy_driver nrf24l01 = {
	.name = "nRF24L01",
	.probe = nrf24l01_probe,
	.remove = nrf24l01_remove,
	.open = nrf24l01_open,
	.recv = nrf24l01_recv,
	.send = nrf24l01_send
};
