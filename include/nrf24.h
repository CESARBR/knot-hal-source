/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

/*
 * nRF24 64-bits address example (string)
 * 88:77:66:55:44:33:22:11 (On left MSB of the address)
 * b0:b1:b2:b3:b4:b5:b6:b7 (mapping to nrf24_mac.b[i])
 * Address should not be converted to host order due
 * potential errors related to endianess.
 */
struct nrf24_mac {
	union {
		uint8_t b[8];
		uint64_t uint64;
	} address;
};

/* Sent after detecting activity on data channel: pipe1 to pipe5*/
struct nrf24_evt_connected {
	struct nrf24_mac src;	/* Source address */
	struct nrf24_mac dst;	/* Destination address */

	uint8_t channel;	/* nRF24 channel: nRF24 spec page 23 */
	uint8_t aa[5];		/* Access Address: nRF24 spec page 25 */
} __attribute__ ((packed));

/* Sent after timeout or user initiated disconnection */
struct nrf24_evt_disconnected {
	struct nrf24_mac src;	/* Source address */
	struct nrf24_mac dst;	/* Destination address */
} __attribute__ ((packed));

/* Event indicating connectability or simply broadcast(beacon) */
struct nrf24_evt_broadcast {
	uint8_t type;		/* Beacon/Setup/Presence */
	uint8_t len;		/* Payload length */
	uint8_t payload[0];
};
