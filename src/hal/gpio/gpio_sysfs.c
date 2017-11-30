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
#include <sys/ioctl.h>
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


static struct gpio_map gpio_pin[HIGHEST_GPIO];

static int gpio_export(int pin)
{
	char buffer[3], path[35];
	ssize_t bytes_written;
	int fd, fd_un, err = 0;

	/* GPIO already exported */
	snprintf(path, 35, "/sys/class/gpio/gpio%2d", pin);
	if (access(path, F_OK) == 0)
		return 0;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd == -1)
		/* Failed to open export for writing! */
		return -errno;

	fd_un = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd_un == -1) {
		/* Failed to open unexport for writing! */
		close(fd);
		return -errno;
	}
	gpio_pin[pin-1].fd_unexport = fd_un;

	bytes_written = snprintf(buffer, 3, "%2d", pin);

	if (write(fd, buffer, bytes_written) == -1)
		err = errno;

	close(fd);

	return -err;
}

static int gpio_unexport(int pin)
{
	char buffer[3];
	ssize_t bytes_written;
	int err = 0;

	bytes_written = snprintf(buffer, 3, "%2d", pin);

	if (write(gpio_pin[pin-1].fd_unexport, buffer, bytes_written) == -1)
		err = errno;

	close(gpio_pin[pin-1].fd_unexport);
	close(gpio_pin[pin-1].fd_value);

	gpio_pin[pin-1].fd_unexport = -1;
	gpio_pin[pin-1].fd_value = -1;

	return -err;
}

static int gpio_direction(int pin, int dir)
{
	char path[35];
	int fd, err = 0, delay_ms;
	/*
	 * Rigth after the GPIO is exported the system have a small
	 * latency before it grants permission to write.
	 */
	snprintf(path, 35, "/sys/class/gpio/gpio%2d/direction", pin);
	for (delay_ms = 0; delay_ms < 200; delay_ms += 10) {
		if (access(path, W_OK) == 0)
			break;
		usleep(delay_ms * 1000);
	}

	snprintf(path, 35, "/sys/class/gpio/gpio%2d/direction", pin);

	fd = open(path, O_WRONLY);
	if (fd == -1)
		/* Failed to open gpio direction for writing! */
		return -errno;

	if (write(fd, dir == HAL_GPIO_INPUT ? "in" : "out",
		dir == HAL_GPIO_INPUT ? 2 : 3) == -1)
		/* Failed to set direction! */
		err = errno;

	return -err;
}

static int gpio_read(int pin)
{
	char path[30];
	char value_str[3];
	int fd;

	snprintf(path, 30, "/sys/class/gpio/gpio%2d/value", pin);

	if (gpio_pin[pin-1].fd_value < 0){
		fd = open(path, O_RDWR);
		if (fd == -1)
			/* Failed to open gpio value for reading! */
			return -errno;
		gpio_pin[pin-1].fd_value = fd;
	}
	/*
	 * This reading operation returns
	 * 3 characters wich can be:
	 * '0', '\n' and '\0' or
	 * '1', '\n' and '\0'
	 */
	if (read(gpio_pin[pin-1].fd_value, value_str, 3) == -1) {
		/* Failed to read value! */
		close(gpio_pin[pin-1].fd_value);
		gpio_pin[pin-1].fd_value = -1;
		return -errno;
	}

	if (value_str[0] == '0' || value_str[0] == '1')
		return value_str[0] - '0';
	return -EAGAIN;
}

static int gpio_write(int pin, int value)
{
	char path[30];
	int fd, err = 0;

	if (gpio_pin[pin-1].fd_value < 0){
		snprintf(path, 30, "/sys/class/gpio/gpio%2d/value", pin);
		fd = open(path, O_RDWR);
		if (fd == -1)
			/* Failed to open gpio value for writing! */
			return -errno;
		gpio_pin[pin-1].fd_value = fd;
	}

	if (write(gpio_pin[pin-1].fd_value, value == HAL_GPIO_LOW ? "0" : "1", 1) != 1)
		/* Failed to write value! */
		err = errno;

	return -err;
}

static int gpio_edge(int pin, int edge)
{
	char path[30];
	int fd, err = 0, werr = 0;

	snprintf(path, 30, "/sys/class/gpio/gpio%2d/edge", pin);
	fd = open(path, O_WRONLY);

	if (fd == -1)
		/* Failed to open gpio value for writing! */
		return -errno;

	switch (edge) {

	case HAL_GPIO_NONE:
		werr = write(fd, "none", 4);
		break;

	case HAL_GPIO_RISING:
		werr = write(fd, "rising", 6);
		break;

	case HAL_GPIO_FALLING:
		werr = write(fd, "falling", 7);
		break;

	case HAL_GPIO_BOTH:
		werr = write(fd, "both", 4);
		break;
	}

	if (werr == -1)
		err = errno;

	close(fd);

	return -err;
}

static int get_gpio_fd(int gpio)
{
	char path[30], dummy_buffer[1024];
	int fd;
	unsigned int n;

	snprintf(path, 30, "/sys/class/gpio/gpio%2d/value", gpio);
	fd = open(path, O_RDONLY);

	if (fd == -1)
		/* Failed to open gpio value for reading! */
		return -errno;

	/* Clear the file before watching */
	ioctl(fd, FIONREAD, &n);
	while (n > 0) {
		if (n >= sizeof(dummy_buffer))
			read(fd, dummy_buffer, sizeof(dummy_buffer));
		else
			read(fd, dummy_buffer, n);
		n -= sizeof(dummy_buffer);
	}

	return fd;
}

int hal_gpio_setup(void)
{
	int i;

	for (i = 0; i < HIGHEST_GPIO; i++){
		gpio_pin[i].status = 0;
		gpio_pin[i].fd_value = -1;
		gpio_pin[i].fd_unexport = -1;
	}

	return 0;
}

void hal_gpio_unmap(void)
{
	int i;

	for (i = 0; i < HIGHEST_GPIO; ++i)
		if (CHK_BIT(gpio_pin[i].status, BIT_INITIALIZED))
			gpio_unexport(i+1);

	memset(gpio_pin, 0, sizeof(gpio_pin));
}

int hal_gpio_pin_mode(uint8_t gpio, uint8_t mode)
{
	int err;

	if (gpio > HIGHEST_GPIO)
		/* Cannot initialize gpio: maximum gpio exceeded */
		return -EINVAL;

	if (!CHK_BIT(gpio_pin[gpio-1].status, BIT_INITIALIZED)) {

		err = gpio_export(gpio);
		if (err < 0)
			return err;

		SET_BIT(gpio_pin[gpio-1].status, BIT_INITIALIZED);
	}

	err = gpio_direction(gpio, mode);
	if (err < 0)
		return err;

	if (mode == HAL_GPIO_INPUT)
		CLR_BIT(gpio_pin[gpio-1].status, BIT_DIRECTION);
	else
		SET_BIT(gpio_pin[gpio-1].status, BIT_DIRECTION);

	return 0;
}

void hal_gpio_digital_write(uint8_t gpio, uint8_t value)
{
	if (CHK_BIT(gpio_pin[gpio-1].status, BIT_DIRECTION)
		&& CHK_BIT(gpio_pin[gpio-1].status, BIT_INITIALIZED))
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

	if (!CHK_BIT(gpio_pin[gpio-1].status, BIT_DIRECTION)
		&& CHK_BIT(gpio_pin[gpio-1].status, BIT_INITIALIZED)) {
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

/*
 * Warning: According to the linux documentation you must
 * follow a set of rules when watching a GPIO pin to trigger
 * the interruption properly:
 *
 * "If you use poll(2), set the events POLLPRI and POLLERR. If you
 * use select(2), set the file descriptor in exceptfds. After
 * poll(2) returns, either lseek(2) to the beginning of the sysfs
 * file and read the new value or close the file and re-open it
 * to read the value.".
 *
 * In case you are using GLIB you must set the conditions
 * G_IO_PRI and G_IO_ERR.
 */
int hal_gpio_get_fd(uint8_t gpio, int edge)
{
	int err;

	if (!CHK_BIT(gpio_pin[gpio-1].status, BIT_INITIALIZED) ||
		CHK_BIT(gpio_pin[gpio-1].status, BIT_DIRECTION))
		return -EIO;

	err = gpio_edge(gpio, edge);
	if (err < 0)
		return err;

	return get_gpio_fd(gpio);
}
