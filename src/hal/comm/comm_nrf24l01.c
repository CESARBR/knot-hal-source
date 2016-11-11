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

#ifdef ARDUINO
#include <avr_errno.h>
#include <avr_unistd.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "include/comm.h"
#include "phy_driver.h"
#include "nrf24l01.h"

static int driverIndex = -1;

int hal_comm_init(const char *pathname)
{
	/* If driver not opened */
	if (driverIndex != -1)
		return -EPERM;

	/* Open driver and returns the driver index */
	driverIndex = phy_open(pathname);
	if (driverIndex < 0)
		return driverIndex;

	return 0;
}

int hal_comm_socket(int domain, int protocol)
{

	return -ENOSYS;
}

void hal_comm_close(int sockfd)
{

}

ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{

	return -ENOSYS;
}

ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{

	return -ENOSYS;
}

int hal_comm_listen(int sockfd)
{

	return -ENOSYS;
}

int hal_comm_accept(int sockfd, uint64_t *addr)
{

	return -ENOSYS;
}


int hal_comm_connect(int sockfd, uint64_t *addr)
{

	return -ENOSYS;
}
