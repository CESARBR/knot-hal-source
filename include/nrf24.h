/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

/* Sent after detecting activity on data channel: pipe1 to pipe5*/
struct nrf24_evt_connected {
	uint64_t src_addr;	/* Source address */
	uint64_t dst_addr;	/* Destination address */

	uint8_t channel;	/* nRF24 channel: nRF24 spec page 23 */
	uint8_t aa[5];		/* Access Address: nRF24 spec page 25 */
} __attribute__ ((packed));

/* Sent after timeout or user initiated disconnection */
struct nrf24_evt_disconnected {
	uint64_t src_addr;	/* Source address */
	uint64_t dst_addr;	/* Destination address */
} __attribute__ ((packed));

/* Event indicating connectability or simply broadcast(beacon) */
struct nrf24_evt_broadcast {
	uint8_t type;		/* Beacon/Setup/Presence */
	uint8_t len;		/* Payload length */
	uint8_t payload[0];
};
