/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdlib.h>
#include <inttypes.h>

#include "sx127x_hal.h"
#include "hal/gpio_sysfs.h"
#include "hal/time.h"
#include "spi_bus.h"

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

//GPIO--------------------------------------------------------------------------
//Function to setup and inicialize all pins used by transciver sx127x;
static void init_gpio(void)
{
	/* This function sets uo the enviorment for accessing the gpio */
	hal_gpio_setup();

	/*
	 * This following set of functions sets up the direction of each gpio
	 * pin whether it is input or output
	 */
	hal_gpio_pin_mode(pins.nss, HAL_GPIO_OUTPUT);
	hal_gpio_pin_mode(pins.rxtx, HAL_GPIO_OUTPUT);
	hal_gpio_pin_mode(pins.rst, HAL_GPIO_OUTPUT);
	hal_gpio_pin_mode(pins.dio[0], HAL_GPIO_INPUT);
	hal_gpio_pin_mode(pins.dio[1], HAL_GPIO_INPUT);
	hal_gpio_pin_mode(pins.dio[2], HAL_GPIO_INPUT);
}

void hal_pin_nss(uint8_t val)
{
	hal_gpio_digital_write(pins.nss, val);
}

void hal_pin_rxtx(uint8_t val)
{
	hal_gpio_digital_write(pins.rxtx, val);
}

void hal_pin_rst(uint8_t val)
{
	if (val == 0 || val == 1) {	//RST pin to LOW or HIGH
		hal_gpio_pin_mode(pins.rst, HAL_GPIO_OUTPUT);
		hal_gpio_digital_write(pins.rst, val);
	} else {			//Configure rst pin floating
		hal_gpio_pin_mode(pins.rst, HAL_GPIO_INPUT);
	}
}

void hal_pins_unmap(void)
{
	hal_gpio_unmap();
}


// SPI--------------------------------------------------------------------------
static int fd_spi;

static void init_spi(void)
{
	fd_spi = spi_bus_init("/dev/spidev0.0");
}

uint8_t hal_spi(uint8_t outval)
{
	int err;

	err =  spi_bus_transfer(fd_spi, NULL, 0, &outval, 1);
	if (err < 0)
		return err;
	else
		return outval;

}


//ISR---------------------------------------------------------------------------
int init_gpio_fd(void)
{
	return hal_gpio_get_fd(pins.dio[0], HAL_GPIO_RISING);
}

void hal_disableIRQs(void)
{

}

void hal_enableIRQs(void)
{

}

void hal_sleep(void)
{
	//not implemented
}


//TIME--------------------------------------------------------------------------
uint32_t hal_ticks(void)
{
	return hal_time_us();
}

void hal_wait_until(uint32_t time)
{
	uint32_t time_us = hal_ticks();

	if (time >= time_us)
		return;

		hal_delay_us(time_us - time);
}

uint8_t hal_check_timer(uint32_t targettime)
{
	//not implemented
	return 0;
}

void hal_failed(void)
{
	//not implemented
}

void hal_init(void)
{
	init_gpio();
	init_spi();
}
