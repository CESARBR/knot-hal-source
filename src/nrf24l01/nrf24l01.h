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
// Auto Retransmit Count => 15 attempt
#define NRF24L01_ARC	15
// Auto Retransmit Delay => 4 ms
#define NRF24L01_ARD	ARD_40000US
		// Delay factor
#define NRF24L01_ARD_FACTOR_US	ARD_FACTOR_US
/// Address width is 5 bytes
#define NRF24L01_ADDR_WIDTHS		5

#define NRF24L01_PIPE_MIN	0// pipe min
#define NRF24L01_PIPE_MAX	5// pipe max
#define NRF24L01_NO_PIPE	RX_FIFO_EMPTY// invalid pipe

#define NRF24L01_PAYLOAD_SIZE				32

#define NRF24L01_PIPE0_ADDR		0
#define NRF24L01_PIPE1_ADDR		1
#define NRF24L01_PIPE2_ADDR		2
#define NRF24L01_PIPE3_ADDR		3
#define NRF24L01_PIPE4_ADDR		4
#define NRF24L01_PIPE5_ADDR		5
#define NRF24L01_PIPE_ADDR_MAX		NRF24L01_PIPE5_ADDR

#define _CONSTRAIN(x, l, h)	((x) < (l) ? (l) : ((x) > (h) ? (h) : (x)))
#define _MIN(a, b)	((a) < (b) ? (a) : (b))

int8_t nrf24l01_init(void);
int8_t nrf24l01_set_channel(uint8_t ch);
int8_t nrf24l01_open_pipe(uint8_t pipe, uint8_t pipe_addr);
int8_t nrf24l01_set_ptx(uint8_t pipe_addr);
int8_t nrf24l01_ptx_data(void *pdata, uint16_t len, bool ack);
int8_t nrf24l01_ptx_wait_datasent(void);
int8_t nrf24l01_set_prx(void);
int8_t nrf24l01_prx_pipe_available(void);
int8_t nrf24l01_prx_data(void *pdata, uint16_t len);
int8_t nrf24l01_set_standby(void);
