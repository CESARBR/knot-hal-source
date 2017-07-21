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

static int gpio_direction(int pin, int dir)
{
	char path[35];
	int fd, ret = 0;

	snprintf(path, 35, "/sys/class/gpio/gpio%2d/direction", pin);
	fd = open(path, O_WRONLY);
	if (fd == -1) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return -EAGAIN;
	}

	if (write(fd, dir == HAL_GPIO_INPUT ? "in" : "out",
		dir == HAL_GPIO_INPUT ? 2 : 3) == -1) {
		fprintf(stderr, "Failed to set direction!\n");
		ret = -EAGAIN;
	}

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
	if (fd == -1) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return -EAGAIN;
	}

	/*
	 * This reading operation returns
	 * 3 characters wich can be:
	 * '0', '\n' and '\0' or
	 * '1', '\n' and '\0'
	 */
	if (read(fd, value_str, 3) == -1) {
		fprintf(stderr, "Failed to read value!\n");
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
	if (fd == -1) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return -EAGAIN;
	}

	if (write(fd, value == HAL_GPIO_LOW ? "0" : "1", 1) != 1) {
		fprintf(stderr, "Failed to write value!\n");
		ret = -EAGAIN;
	}

	close(fd);
	return ret;
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
