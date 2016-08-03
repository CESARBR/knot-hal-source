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
