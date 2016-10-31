/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

/* Output power in max power */
#define NRF24_POWER		NRF24_PWR_0DBM

/* Data rate in Mbps */
#define NRF24_DATA_RATE	NRF24_DR_1MBPS

/* Channel = 2400GHz + CHANNEL_DEF [MHz], max 2.525GHz */
#define NRF24_CHANNEL_DEFAULT	10

/* Auto Retransmit Count => 15 attempt */
#define NRF24_ARC			15

/* Auto Retransmit Delay => 4 ms */
#define NRF24_ARD		NRF24_ARD_40000US

/* Address width is 5 bytes */
#define NRF24_ADDR_WIDTHS		5

#define NRF24_PIPE_MIN		0
#define NRF24_PIPE_MAX		5

/* Invalid pipe */
#define NRF24_NO_PIPE	NRF24_RX_FIFO_EMPTY

#define NRF24_PAYLOAD_SIZE		32
#define NRF24_PIPE0_ADDR		0
#define NRF24_PIPE1_ADDR		1
#define NRF24_PIPE2_ADDR		2
#define NRF24_PIPE3_ADDR		3
#define NRF24_PIPE4_ADDR		4
#define NRF24_PIPE5_ADDR		5
#define NRF24_PIPE_ADDR_MAX		NRF24_PIPE5_ADDR

#define PIPE_BROADCAST NRF24_PIPE0_ADDR

#define _CONSTRAIN(x, l, h)	((x) < (l) ? (l) : ((x) > (h) ? (h) : (x)))
#define _MIN(a, b)		((a) < (b) ? (a) : (b))

#ifdef __cplusplus
extern "C"{
#endif

int8_t nrf24l01_init(const char *dev, uint8_t tx_pwr);
int8_t nrf24l01_deinit(int8_t spi_fd);
int8_t nrf24l01_set_channel(int8_t spi_fd, uint8_t ch);
int8_t nrf24l01_open_pipe(int8_t spi_fd, uint8_t pipe, uint8_t *pipe_addr,
					bool ack);
int8_t nrf24l01_set_ptx(int8_t spi_fd, uint8_t pipe);
int8_t nrf24l01_ptx_data(int8_t spi_fd, void *pdata, uint16_t len);
int8_t nrf24l01_ptx_wait_datasent(int8_t spi_fd);
int8_t nrf24l01_set_prx(int8_t spi_fd, uint8_t *pipe0_addr);
int8_t nrf24l01_prx_pipe_available(int8_t spi_fd);
int8_t nrf24l01_prx_data(int8_t spi_fd, void *pdata, uint16_t len);
int8_t nrf24l01_set_standby(int8_t spi_fd);

#ifdef __cplusplus
} // extern "C"
#endif
