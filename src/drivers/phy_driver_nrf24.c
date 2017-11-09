/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef ARDUINO
#include "hal/avr_errno.h"
#include "hal/avr_unistd.h"
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "hal/nrf24.h"
#include "nrf24l01_io.h"
#include "nrf24l01.h"
#include "phy_driver_private.h"
#include "phy_driver_nrf24.h"

static ssize_t nrf24l01_write(int spi_fd, const void *buffer, size_t len)
{
	int err;
	struct nrf24_io_pack *p = (struct nrf24_io_pack *) buffer;

	/* Sets Radio TX mode, enabling Acknowledgment */
	nrf24l01_set_ptx(spi_fd, p->pipe);

	/* Data Transmission */
	err = nrf24l01_ptx_data(spi_fd, p->payload, len);
	if (err == 0)
		/* Wait for ACK */
		err = nrf24l01_ptx_wait_datasent(spi_fd);

	/*
	 * The radio does not receive and send at the same time
	 * It's a good practice to put the radio in RX mode
	 * and only switch to TX mode when transmitting data.
	 */

	nrf24l01_set_prx(spi_fd);

	/*
	 * On success, the number of bytes written is returned
	 * Otherwise, -1 is returned.
	 */

	if (err < 0 || err == 1)
		return -1;

	return len;
}

static ssize_t nrf24l01_read(int spi_fd, void *buffer, size_t len)
{
	ssize_t length = 0;
	struct nrf24_io_pack *p = (struct nrf24_io_pack *) buffer;
	uint8_t pipe;

	/* If the pipe available */
	pipe = nrf24l01_prx_pipe_available(spi_fd);
	if ((p->pipe == NRF24_NO_PIPE && pipe != NRF24_NO_PIPE) ||
			(p->pipe != NRF24_NO_PIPE && pipe == p->pipe)) {
		p->pipe = pipe;
		/* Copy data to buffer */
		length = nrf24l01_prx_data(spi_fd, p->payload, len);
	}

	/*
	 * On success, the number of bytes read is returned
	 * Otherwise, 0 is returned.
	 */
	return length;
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

	/* Init the radio with tx power 0dbm. Returns SPI fd */
	return nrf24l01_init(pathname, NRF24_PWR_0DBM);
}

static void nrf24l01_close(int spi_fd)
{
	nrf24l01_deinit(spi_fd);
}

static int nrf24l01_ioctl(int spi_fd, int cmd, void *arg)
{
	union {
		struct addr_pipe addr;
		struct channel ch;
	} param;
	int err = -1;

	/* Set standby to set registers */
	nrf24l01_set_standby(spi_fd);

	switch (cmd) {
	/* Command to set address pipe */
	case NRF24_CMD_SET_PIPE:
		memcpy(&param.addr, arg, sizeof(param.addr));
		err = nrf24l01_open_pipe(spi_fd, param.addr.pipe, param.addr.aa);
		break;
	case NRF24_CMD_RESET_PIPE:
		err = nrf24l01_close_pipe(spi_fd, *((int *) arg));
		break;
	/* Command to set channel pipe */
	case NRF24_CMD_SET_CHANNEL:
		memcpy(&param.ch, arg, sizeof(param.ch));
		err = nrf24l01_set_channel(spi_fd, param.ch.value, param.ch.ack);
		break;
	case NRF24_CMD_SET_STANDBY:
		break;
	default:
		break;
	}

	if (cmd != NRF24_CMD_SET_STANDBY)
		nrf24l01_set_prx(spi_fd);

	return err;
}

struct phy_driver nrf24l01 = {
	.name = "NRF0",
#ifndef ARDUINO
	.pathname = "/dev/spidev0.0",
#else
	.pathname = NULL,
#endif
	.open = nrf24l01_open,
	.read = nrf24l01_read,
	.write = nrf24l01_write,
	.ioctl = nrf24l01_ioctl,
	.close = nrf24l01_close,
	.ref_open = 0,
	.fd = -1
};
