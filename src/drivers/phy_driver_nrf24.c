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

#ifdef ARDUINO
#include "avr_errno.h"
#include "avr_unistd.h"
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "nrf24l01.h"
#include "phy_driver_private.h"
#include "phy_driver_nrf24.h"

uint8_t broadcast_addr[5] = {0x8D, 0xD9, 0xBE, 0x96, 0xDE};

static ssize_t nrf24l01_write(int spi_fd, const void *buffer, size_t len)
{
	int err;
	struct nrf24_io_pack *p = (struct nrf24_io_pack *) buffer;

	/* Puts the radio in TX mode  enabling Acknowledgment */
	nrf24l01_set_ptx(spi_fd, p->pipe);

	/* Transmits the data */
	nrf24l01_ptx_data(spi_fd, (void *)p->payload, len);

	/* Waits for ACK */
	err = nrf24l01_ptx_wait_datasent(spi_fd);

	if (err < 0)
		return err;
	/*
	 * The radio do not receive and send at the same time
	 * It's a good practice to put the radio in RX mode
	 * and only switch to TX mode when transmitting data.
	 */

	nrf24l01_set_prx(spi_fd, broadcast_addr);

	/*
	 * On success, the number of bytes written is returned
	 * Otherwise, -1 is returned.
	 */

	return len;
}

static ssize_t nrf24l01_read(int spi_fd, void *buffer, size_t len)
{
	return -ENOSYS;

}

static int nrf24l01_open(const char *pathname)
{
	return -ENOSYS;

}

static void nrf24l01_close(int spi_fd)
{

}

static int nrf24l01_ioctl(int spi_fd, int cmd, void *arg)
{
	return -ENOSYS;
}

struct phy_driver nrf24l01 = {
	.name = "NRF0",
	.pathname = "/dev/spidev0.0",
	.open = nrf24l01_open,
	.recv = nrf24l01_read,
	.send = nrf24l01_write,
	.ioctl = nrf24l01_ioctl,
	.close = nrf24l01_close,
	.ref_open = 0,
	.fd = -1
};
