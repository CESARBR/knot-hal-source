/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <unistd.h>

#include "phy_driver.h"
#include "hal.h"

extern struct phy_driver nrf24l01;

static struct phy_driver *drivers[] = {
	&nrf24l01,
	NULL
};

int hal_init(void)
{
	struct phy_driver *drv;
	int i;

	for (i = 0, drv = drivers[i]; drv; i++)
		drv->probe();
}

void hal_exit(void)
{
	struct phy_driver *drv;
	int i;

	for (i = 0, drv = drivers[i]; drv; i++)
		drv->remove();
}

int ksocket(int domain, int type, int protocol)
{
	struct phy_driver *drv;
	int i;
	/* PF_NRF24L01: currently supported domain */

	if (type != TYPE_SOCK_RAW)
		return -1;

	for (i = 0, drv = drivers[i]; drv; i++) {
		if (domain != drv->domain)
			continue;

		/* TODO: track created 'sockets' */
		return drv->socket(type, protocol);
	}

	return -1;
}
