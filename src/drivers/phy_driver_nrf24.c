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

#include "phy_driver_private.h"
#include "nrf24l01_ll.h"

static ssize_t nrf24l01_write(int sockfd, const void *buffer, size_t len)
{
	return -ENOSYS;

}

static ssize_t nrf24l01_read(int sockfd, void *buffer, size_t len)
{
	return -ENOSYS;

}

static int nrf24l01_open(const char *pathname)
{
	return -ENOSYS;

}

static void nrf24l01_close(int sockfd)
{

}

static int nrf24l01_ioctl(int sockfd, int cmd, void *arg)
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
