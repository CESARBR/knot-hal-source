/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#ifdef ARDUINO
#include "hal/avr_errno.h"
#include "hal/avr_unistd.h"
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "phy_driver_private.h"
#include "phy_driver.h"

struct phy_driver *driver_ops[] = {
	&nrf24l01
};

/* ARRAY SIZE */
#define PHY_DRIVERS_COUNTER	((int) (sizeof(driver_ops) \
				 / sizeof(driver_ops[0])))

int phy_open(const char *pathname)
{
	uint8_t i;
	int err, sockfd = -1;

	/* Find driver index */
	for (i = 0; i < PHY_DRIVERS_COUNTER; ++i) {
		if (strcmp(pathname, driver_ops[i]->name) == 0)
			sockfd = i;
	}

	/* Return error if driver not found */
	if (sockfd < 0)
		return sockfd;

	/* If not open */
	if (driver_ops[sockfd]->ref_open == 0) {
		/* Open the driver - returns fd */
		err = driver_ops[sockfd]->open(driver_ops[sockfd]->pathname);
		if (err < 0)
			return err;

		driver_ops[sockfd]->fd = err;
	}

	++driver_ops[sockfd]->ref_open;

	return sockfd;
}

int phy_close(int sockfd)
{
	if (sockfd < 0 || sockfd > PHY_DRIVERS_COUNTER)
		return -EINVAL;

	/* If has open driver */
	if (driver_ops[sockfd]->ref_open != 0) {
		--driver_ops[sockfd]->ref_open;
		/* If zero then close the driver */
		if (driver_ops[sockfd]->ref_open == 0) {
			driver_ops[sockfd]->close(driver_ops[sockfd]->fd);
			driver_ops[sockfd]->fd = -1;
		}
	}

	return 0;
}

ssize_t phy_read(int sockfd, void *buffer, size_t len)
{
	return driver_ops[sockfd]->read(driver_ops[sockfd]->fd, buffer, len);
}

ssize_t phy_write(int sockfd, const void *buffer, size_t len)
{
	return driver_ops[sockfd]->write(driver_ops[sockfd]->fd, buffer, len);
}

int phy_ioctl(int sockfd, int cmd, void *arg)
{
	return driver_ops[sockfd]->ioctl(driver_ops[sockfd]->fd, cmd, arg);
}
