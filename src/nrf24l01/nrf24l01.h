/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "nrf24l01_io.h"

// Output power in max power
#define NRF24L01_POWER		PWR_0DBM
// Data rate in Mbps
#define NRF24L01_DATA_RATE		DR_1MBPS
// Channel = 2400GHz + CHANNEL_DEF [MHz], max 2.525GHz
#define NRF24L01_CHANNEL_DEFAULT	CH_MIN
/// Address width is 5 bytes
#define NRF24L01_ADDR_WIDTHS		5

#define NRF24L01_PIPE0_ADDR		0

#define _CONSTRAIN(x, l, h)	((x) < (l) ? (l) : ((x) > (h) ? (h) : (x)))

int8_t nrf24l01_init(void);
int8_t nrf24l01_set_channel(uint8_t ch);
