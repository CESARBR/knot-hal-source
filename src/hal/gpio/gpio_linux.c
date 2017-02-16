/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "include/gpio.h"

 static volatile unsigned	*gpio;

int hal_gpio_setup(void)
{
	int mem_fd;

	mem_fd = open("/dev/mem", O_RDWR|O_SYNC);

	if (mem_fd < 0)
		return -errno;

	gpio = (volatile unsigned *)mmap(NULL, BLOCK_SIZE,
						PROT_READ | PROT_WRITE,
						MAP_SHARED, mem_fd, GPIO_BASE);
	close(mem_fd);

	if (gpio == MAP_FAILED)
		return -errno;

	return 0;
}

void hal_gpio_unmap(void)
{
	munmap((void*)gpio, BLOCK_SIZE);
}

void hal_gpio_pin_mode(uint8_t pin, uint8_t mode)
{
	if (mode == OUTPUT) {
		/* MUST use INP_GPIO before OUT_GPIO */
		INP_GPIO(pin);
		OUT_GPIO(pin);
	} else {
		INP_GPIO(pin);
	}
}


void hal_gpio_digital_write(uint8_t pin, uint8_t value)
{
	if (value)
		GPIO_SET = 1 << pin;
	else
		GPIO_CLR = 1 << pin;
}

int hal_gpio_digital_read(uint8_t pin)
{
	if (GET_GPIO(pin))
		return 1;
	return 0;
}

int hal_gpio_analog_read(uint8_t pin)
{
	return 0;
}

void hal_gpio_analog_reference(uint8_t type)
{

}

void hal_gpio_analog_write(uint8_t pin, int value)
{

}
