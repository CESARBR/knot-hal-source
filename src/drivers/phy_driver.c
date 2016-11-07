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

inline int phy_open(const char *pathname)
{
	return -ENOSYS;
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
