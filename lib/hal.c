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

int hal_init(void)
{
	phy_driver *drv;
	int i;

	for (i = 0, drv = drivers[i]; drv; i++)
		drv->probe();
}

void hal_exit(void)
{
	phy_driver *drv;
	int i;

	for (i = 0, drv = drivers[i]; drv; i++)
		drv->remove();
}
