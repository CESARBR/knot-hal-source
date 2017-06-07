/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifdef ARDUINO
#include <avr_errno.h>
#include <avr_unistd.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

#include <stdint.h>

/* FIXME: Remove this header */
#include "hal/nrf24.h"

#include "hal/comm.h"

int hal_comm_init(const char *pathname, const void *params)
{
	return -ENOSYS;
}

int hal_comm_deinit(void)
{
	return -ENOSYS;
}

int hal_comm_socket(int domain, int protocol)
{
	return -ENOSYS;
}

int hal_comm_close(int sockfd)
{
	return -ENOSYS;
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
	/* It is not applied to serial ports */

	return -ENOSYS;
}

int hal_comm_accept(int sockfd, void *addr)
{
	/* It is not applied to serial ports */

	return -ENOSYS;
}

int hal_comm_connect(int sockfd, uint64_t *addr)
{
	/* It is not applied to serial ports */

	return -ENOSYS;
}
