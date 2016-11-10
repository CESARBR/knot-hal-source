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
#include <string.h>

#ifdef ARDUINO
#include <avr_errno.h>
#include <avr_unistd.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "include/comm.h"
#include "phy_driver.h"
#include "phy_driver_nrf24.h"

/* Array to indicate if pipe is in used */
static int8_t pipes_allocate[] = {0, 0, 0, 0, 0};

/* Access Address for each pipe */
static uint8_t aa_pipes[6][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x35, 0x96, 0xB6, 0xC1, 0x6B},
	{0x77, 0x96, 0xB6, 0xC1, 0x6B},
	{0xD3, 0x96, 0xB6, 0xC1, 0x6B},
	{0xE7, 0x96, 0xB6, 0xC1, 0x6B},
	{0xF0, 0x96, 0xB6, 0xC1, 0x6B}
};

static int driverIndex = -1;

/* Local functions */
static inline int get_free_pipe(void)
{
	uint8_t i;

	for (i = 0; i < sizeof(pipes_allocate); i++) {
		if (pipes_allocate[i] == 0) {
			/* one peer for pipe*/
			pipes_allocate[i] = 1;
			return i+1;
		}
	}

	/* No free pipe */
	return -1;
}

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
	int retval;
	struct addr_pipe ap;

	/* If domain is not NRF24 */
	if (domain != HAL_COMM_PF_NRF24)
		return -EPERM;

	/* If not initialized */
	if (driverIndex == -1)
		return -EPERM;	/* Operation not permitted */

	switch (protocol) {

	case HAL_COMM_PROTO_MGMT:
		/* If Management, disable ACK and returns 0 */
		ap.ack = false;
		retval = 0;
		break;

	case HAL_COMM_PROTO_RAW:
		/*
		 * If raw data, enable ACK
		 * and returns an available pipe
		 * from 1 to 5
		 */
		retval = get_free_pipe();
		/* If not pipe available */
		if (retval < 0)
			return -EUSERS; /* Returns too many users */

		ap.ack = true;
		break;

	default:
		return -EINVAL; /* Invalid argument */
	}

	ap.pipe = retval;
	memcpy(ap.aa, aa_pipes[retval], sizeof(aa_pipes[retval]));

	/* Open pipe */
	phy_ioctl(driverIndex, CMD_SET_PIPE, &ap);

	return retval;
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
