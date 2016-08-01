/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

/* FIXME: Arduino */
#include <stdint.h>
#include <stdlib.h>

#include <proto-net/knot_proto_net.h>

#include "phy_driver.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

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
#endif

#ifdef HAVE_NRF24
	&driver_nrf24,
#endif

#ifdef HAVE_NRF51
	&driver_nrf51,
#endif

#ifdef HAVE_NRF905
	&driver_nrf905,
#endif

#ifdef HAVE_RFM69
	&driver_rfm69,
#endif

#ifdef HAVE_ETH
	&driver_eth,
#endif

#ifdef HAVE_ESP8266
	&driver_esp8266,
#endif

#ifdef HAVE_ASK
	&driver_ask,
#endif
	NULL
};
