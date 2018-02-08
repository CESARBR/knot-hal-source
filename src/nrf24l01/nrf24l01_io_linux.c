/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <unistd.h>

#include "hal/gpio_sysfs.h"
#include "nrf24l01_io.h"
#include "spi_bus.h"

#define CE	25
#define IRQ	24

/* Time delay in microseconds (us) */
#define	TPECE2CSN		4

void delay_us(float us)
{
	usleep(us);
}

void enable(void)
{
	hal_gpio_digital_write(CE, HAL_GPIO_HIGH);
	usleep(TPECE2CSN);
}

void disable(void)
{
	hal_gpio_digital_write(CE, HAL_GPIO_LOW);
}

int io_setup(const char *dev)
{
	int err;

	hal_gpio_setup();

	err = hal_gpio_pin_mode(CE, HAL_GPIO_OUTPUT);
	if (err < 0)
		return err;

	err = hal_gpio_pin_mode(IRQ, HAL_GPIO_INPUT);
	if (err < 0){
		hal_gpio_unmap();
		return err;
	}

	disable();
	return spi_bus_init(dev);
}

void io_reset(int spi_fd)
{
	disable();
	hal_gpio_unmap();
	spi_bus_deinit(spi_fd);
}
