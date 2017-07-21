/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "hal/gpio_sysfs.h"

static int gpio_export(int pin)
{
	char buffer[3];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd == -1) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return -EAGAIN;
	}

	bytes_written = snprintf(buffer, 3, "%2d", pin);

	write(fd, buffer, bytes_written);
	close(fd);

	return 0;
}

static int gpio_unexport(int pin)
{
	char buffer[3];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd == -1) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return -EAGAIN;
	}

	bytes_written = snprintf(buffer, 3, "%2d", pin);

	write(fd, buffer, bytes_written);
	close(fd);

	return 0;
}

int hal_gpio_setup(void)
{
	return 0;
}

void hal_gpio_unmap(void)
{

}

int hal_gpio_pin_mode(uint8_t gpio, uint8_t mode)
{
	return 0;
}

void hal_gpio_digital_write(uint8_t gpio, uint8_t value)
{

}

int hal_gpio_digital_read(uint8_t gpio)
{
	return 0;
}

int hal_gpio_analog_read(uint8_t gpio)
{
	return 0;
}

void hal_gpio_analog_reference(uint8_t mode)
{

}

void hal_gpio_analog_write(uint8_t gpio, int value)
{

}
