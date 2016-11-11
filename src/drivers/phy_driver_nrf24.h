/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __PHY_DRIVER_NRF24__
#define __PHY_DRIVER_NRF24__

#define NRF24_PAYLOAD_SIZE		32

/* Used to read/write operations */
struct nrf24_io_pack {
	uint8_t pipe;
	uint8_t payload[NRF24_PAYLOAD_SIZE];
} __attribute__ ((packed));

enum nrf24_cmds {
				NRF24_CMD_SET_PIPE,
				NRF24_CMD_RESET_PIPE,
				NRF24_CMD_SET_CHANNEL,
				NRF24_CMD_GET_CHANNEL,
				NRF24_CMD_SET_ADDRESS_PIPE,
				NRF24_CMD_SET_POWER,
};

/* Used to set pipe address*/
struct addr_pipe {
	uint8_t pipe;
	bool ack;
	uint8_t aa[5];
};


#endif
