/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "hal/gpio_sysfs.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define HIGHEST_GPIO 28

/* Bit Operation */
#define SET_BIT(data, idx)		((data) |= 1 << (idx))
#define CLR_BIT(data, idx)		((data) &= ~(1 << (idx)))
#define CHK_BIT(data, idx)		((data) & (1 << (idx)))

/* 0 = not initialized, 1 = initialized */
#define BIT_INITIALIZED	0
/* 0 = input, 1 = output */
#define BIT_DIRECTION	1


static uint8_t gpio_map[HIGHEST_GPIO];

static int gpio_export(int pin)
{
	char buffer[3];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd == -1)
		/* Failed to open export for writing! */
		return -EAGAIN;

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
	if (fd == -1)
		/* Failed to open unexport for writing! */
		return -EAGAIN;

	bytes_written = snprintf(buffer, 3, "%2d", pin);

	write(fd, buffer, bytes_written);
	close(fd);

	return 0;
}

static int gpio_direction(int pin, int dir)
{
	char path[35];
	int fd, ret = 0;

	snprintf(path, 35, "/sys/class/gpio/gpio%2d/direction", pin);
	fd = open(path, O_WRONLY);
	if (fd == -1)
		/* Failed to open gpio direction for writing! */
		return -EAGAIN;

	if (write(fd, dir == HAL_GPIO_INPUT ? "in" : "out",
		dir == HAL_GPIO_INPUT ? 2 : 3) == -1)
		/* Failed to set direction! */
		ret = -EAGAIN;

	close(fd);
	return ret;
}

static int gpio_read(int pin)
{
	char path[30];
	char value_str[3];
	int fd;

	snprintf(path, 30, "/sys/class/gpio/gpio%2d/value", pin);
	fd = open(path, O_RDONLY);
	if (fd == -1)
		/* Failed to open gpio value for reading! */
		return -EAGAIN;

	/*
	 * This reading operation returns
	 * 3 characters wich can be:
	 * '0', '\n' and '\0' or
	 * '1', '\n' and '\0'
	 */
	if (read(fd, value_str, 3) == -1) {
		/* Failed to read value! */
		close(fd);
		return -EAGAIN;
	}

	close(fd);

	if (value_str[0] == '0' || value_str[0] == '1')
		return value_str[0] - '0';
	return -EAGAIN;
}

static int gpio_write(int pin, int value)
{
	char path[30];
	int fd, ret = 0;

	snprintf(path, 30, "/sys/class/gpio/gpio%2d/value", pin);
	fd = open(path, O_WRONLY);
	if (fd == -1)
		/* Failed to open gpio value for writing! */
		return -EAGAIN;

	if (write(fd, value == HAL_GPIO_LOW ? "0" : "1", 1) != 1)
		/* Failed to write value! */
		ret = -EAGAIN;

	close(fd);
	return ret;
}

int hal_gpio_setup(void)
{
	char *ret;

	ret = memset(gpio_map, 0, sizeof(gpio_map));
	return (ret == NULL) ? -EAGAIN : 0;
}

void hal_gpio_unmap(void)
{
	int i;

	for (i = 0; i < HIGHEST_GPIO; ++i)
		if (CHK_BIT(gpio_map[i], BIT_INITIALIZED))
			gpio_unexport(i);

	memset(gpio_map, 0, sizeof(gpio_map));
}

int hal_gpio_pin_mode(uint8_t gpio, uint8_t mode)
{
	if (gpio > HIGHEST_GPIO)
		/* Cannot initialize gpio: maximum gpio exceeded */
		return -1;

	if (!CHK_BIT(gpio_map[gpio-1], BIT_INITIALIZED)) {
		if (gpio_export(gpio) != 0)
			return -1;
		SET_BIT(gpio_map[gpio-1], BIT_INITIALIZED);
	}

	if (gpio_direction(gpio, mode) != 0)
		return -1;

	if (mode == HAL_GPIO_INPUT)
		CLR_BIT(gpio_map[gpio-1], BIT_DIRECTION);
	else
		SET_BIT(gpio_map[gpio-1], BIT_DIRECTION);

	return 0;
}

void hal_gpio_digital_write(uint8_t gpio, uint8_t value)
{
	if (CHK_BIT(gpio_map[gpio-1], BIT_DIRECTION)
		&& CHK_BIT(gpio_map[gpio-1], BIT_INITIALIZED))
		gpio_write(gpio, value);
	else{
		/* Changing mode and writing */
		hal_gpio_pin_mode(gpio, HAL_GPIO_OUTPUT);
		hal_gpio_digital_write(gpio, value);
	}
}

int hal_gpio_digital_read(uint8_t gpio)
{
	int ret = 0;

	if (!CHK_BIT(gpio_map[gpio-1], BIT_DIRECTION)
		&& CHK_BIT(gpio_map[gpio-1], BIT_INITIALIZED)) {
		ret = gpio_read(gpio);
		if (ret < 0)
			return ret;
		return (ret == 0 ? HAL_GPIO_LOW : HAL_GPIO_HIGH);
	}

	/* Changing mode and reading */
	hal_gpio_pin_mode(gpio, HAL_GPIO_INPUT);
	return hal_gpio_digital_read(gpio);
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
