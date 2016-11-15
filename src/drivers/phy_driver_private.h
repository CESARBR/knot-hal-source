/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifdef ARDUINO
#include <Arduino.h>
#endif

 /**
 * struct phy_driver - driver abstraction for the physical layer
 * @name: driver name
 * @pathname: driver path
 * @open: function to initialize the driver
 * @close: function to close the driver and free its resources
 * @ioctl: function to device-specific input/output operations
 * @ref_open: reference driver open
 * @fd: driver fd
 *
 * This 'driver' intends to be an abstraction for Radio technologies or
 * proxy for other services using TCP or any socket based communication.
 */

struct phy_driver {
	const char *name;
	const char *pathname;

	int (*open) (const char *pathname);
	void (*close) (int sockfd);
	ssize_t (*recv) (int sockfd, void *buffer, size_t len);
	ssize_t (*send) (int sockfd, const void *buffer, size_t len);
	int (*ioctl) (int sockfd, int cmd, void *arg);
	int ref_open;
	int fd;
};

extern struct phy_driver nrf24l01;
