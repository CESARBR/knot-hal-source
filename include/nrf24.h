/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __HAL_NRF24_H__
#define __HAL_NRF24_H__

#ifdef __cplusplus
extern "C" {
#endif

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

/* Converts nrf24_mac address to string */
int nrf24_mac2str(const struct nrf24_mac *, char *);

/* Converts string to nrf24_mac address */
int nrf24_str2mac(const char *, struct nrf24_mac *);

/* Sent after detecting activity on data channel: pipe1 to pipe5*/
#define MGMT_EVT_NRF24_CONNECTED		0x0201 /* PHY connected */
struct mgmt_evt_nrf24_connected {
	struct nrf24_mac src;	/* Source address */
	struct nrf24_mac dst;	/* Destination address */

	uint8_t channel;	/* nRF24 channel: nRF24 spec page 23 */
	uint8_t aa[5];		/* Access Address: nRF24 spec page 25 */
} __attribute__ ((packed));

/* Sent after timeout or user initiated disconnection */
#define MGMT_EVT_NRF24_DISCONNECTED		0x0202 /* PHY connected */
struct mgmt_evt_nrf24_disconnected {
	struct nrf24_mac src;	/* Source address */
	struct nrf24_mac dst;	/* Destination address */
} __attribute__ ((packed));

#define MGMT_EVT_NRF24_BCAST_BEACON		0x0203 /* Non-connectable */
struct mgmt_evt_nrf24_bcast_beacon {
	uint8_t len;		 /* Payload length */
	uint8_t payload[0];
} __attribute__ ((packed));

#define MGMT_EVT_NRF24_BCAST_SETUP		0x0204 /* Connectable & on setup mode */
struct mgmt_evt_nrf24_bcast_setup {
	struct nrf24_mac src;
	/* TODO: Add device/service infos? */
} __attribute__ ((packed));

#define MGMT_EVT_NRF24_BCAST_PRESENCE		0x0205 /* Connectable */
struct mgmt_evt_nrf24_bcast_presence {
	struct nrf24_mac src;
	/* TODO: Add device/service infos? */
} __attribute__ ((packed));

struct mgmt_nrf24_header {
	uint16_t opcode;	/* Command/Response/Event opcode */
	uint8_t index;		/* Multi adapter: index */
	uint8_t payload[0];	/* Command/Response/Event specific data */
} __attribute__ ((packed));

#ifdef __cplusplus
}
#endif

#endif /* __HAL_NRF24_H__ */
