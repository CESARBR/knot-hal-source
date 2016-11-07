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
#include "avr_errno.h"
#include "avr_unistd.h"
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

inline int phy_open(const char *pathname)
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

inline int phy_close(int sockfd)
{
	return -ENOSYS;
}

inline ssize_t phy_read(int sockfd, void *buffer, size_t len)
{
	return -ENOSYS;
}

inline ssize_t phy_write(int sockfd, const void *buffer, size_t len)
{

	return -ENOSYS;

}

inline int phy_ioctl(int sockfd, int cmd, void *args)
{

	return -ENOSYS;
}
