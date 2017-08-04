/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "sx127x_hal.h"

/*
 * Pinmap for Raspberry PI 3
 * nss  -> slave select pin
 * rxtx -> rx-tx pin to control the antenna switch
 * rst  -> reset pin
 * dio  -> digital i/o for interruption
 */
const struct lmic_pinmap pins = {
	.nss = 25,
	.rxtx = 24,
	.rst = 17,
	.dio = {21, 4, 24},
};

void hal_init(void)
{

}

void hal_pin_nss(uint8_t val)
{

}

void hal_pin_rxtx(uint8_t val)
{

}

void hal_pin_rst(uint8_t val)
{

}

uint8_t hal_spi(uint8_t outval)
{
	return 0;
}

void hal_disableIRQs(void)
{

}

void hal_enableIRQs(void)
{

}

void hal_sleep(void)
{

}

uint32_t hal_ticks(void)
{
	return 0;
}

void hal_waitUntil(uint32_t time)
{

}

uint8_t hal_checkTimer(uint32_t targettime)
{
	return 0;
}

void hal_failed(void)
{

}
