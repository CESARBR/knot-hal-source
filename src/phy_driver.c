/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <phy_driver.h>
#include <knot_proto_net.h>
#ifdef ARDUINO
#include <Arduino.h>
#endif

#define EMPTY_DRIVER_NAME	"Empty driver"

static phy_driver driver_empty;

int empty_probe()
{
	return KNOT_NET_ERROR;
}

void empty_remove()
{

}

int empty_socket()
{
	return KNOT_SOCKET_FD_INVALID;
}

void empty_close(int socket)
{

}

int empty_connect(int socket, uint8_t to_addr)
{
	return KNOT_NET_ERROR;
}

int empty_generic_function(int socket)
{
	return KNOT_NET_ERROR;
}

size_t empty_recv (int sockfd, void *buffer, size_t len)
{
	return 0;
}

size_t empty_send (int sockfd, const void *buffer, size_t len)
{
	return 0;
}

phy_driver driver_empty = {
	.name = EMPTY_DRIVER_NAME,
	.valid = 0,
	.probe = empty_probe,
	.remove = empty_remove,
	.socket = empty_socket,
	.close = empty_close,
	.listen = empty_generic_function,
	.accept = empty_generic_function,
	.connect = empty_connect,
	.available = empty_generic_function,
	.recv = empty_recv,
	.send = empty_send
};

#ifdef HAVE_SERIAL
extern phy_driver driver_serial;
#endif

#ifdef HAVE_NRF24
extern phy_driver driver_nrf24;
#endif

#ifdef HAVE_NRF51
extern phy_driver driver_nrf51;
#endif

#ifdef HAVE_NRF905
extern phy_driver driver_nrf905;
#endif

#ifdef HAVE_RFM69
extern phy_driver driver_rfm69;
#endif

#ifdef HAVE_ETH
extern phy_driver driver_eth;
#endif

#ifdef HAVE_ESP8266
extern phy_driver driver_esp8266;
#endif

#ifdef HAVE_ASK
extern phy_driver driver_ask;
#endif

phy_driver *drivers[] = {
#ifdef HAVE_SERIAL
	&driver_serial,
#else
	&driver_empty,
#endif

#ifdef HAVE_NRF24
	&driver_nrf24,
#else
	&driver_empty,
#endif

#ifdef HAVE_NRF51
	&driver_nrf51,
#else
	&driver_empty,
#endif

#ifdef HAVE_NRF905
	&driver_nrf905,
#else
	&driver_empty,
#endif

#ifdef HAVE_RFM69
	&driver_rfm69,
#else
	&driver_empty,
#endif

#ifdef HAVE_ETH
	&driver_eth,
#else
	&driver_empty,
#endif

#ifdef HAVE_ESP8266
	&driver_esp8266,
#else
	&driver_empty,
#endif

#ifdef HAVE_ASK
	&driver_ask,
#else
	&driver_empty,
#endif
	NULL
};
