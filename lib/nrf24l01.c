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

#include "hal.h"
#include "phy_driver.h"

static int nrf24l01_probe(void)
{
	return 0;
}

static void nrf24l01_remove(void)
{

}

static struct phy_driver nrf24l01 = {
	.name = "nRF24L01",
	.domain = PF_NRF24L01,
	.probe = nrf24l01_probe,
	.remove = nrf24l01_remove
};
