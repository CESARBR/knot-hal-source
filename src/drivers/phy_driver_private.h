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
 * @probe: function to initialize the driver
 * @remove: function to close the driver and free its resources
 * @socket: function to create a new socket FD
 * @listen: function to enable incoming connections
 * @accept: function that waits for a new connection and returns a 'pollable' FD
 * @connect: function to connect to a server socket
 *
 * This 'driver' intends to be an abstraction for Radio technologies or
 * proxy for other services using TCP or any socket based communication.
 */

struct phy_driver {
	const char *name;
	int (*probe) (const char *spi, uint8_t tx_pwr);
	void (*remove) (void);

	int (*open) (const char *pathname);
	void (*close) (int sockfd);
	int (*listen) (int srv_sockfd, uint8_t channel);
	int (*accept) (int srv_sockfd);
	int (*connect) (int cli_sockfd, uint8_t to_addr);
	ssize_t (*recv) (int sockfd, void *buffer, size_t len);
	ssize_t (*send) (int sockfd, const void *buffer, size_t len);
};

extern struct phy_driver nrf24l01;
