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

#define NRF24_MTU					32

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

struct nrf24_config {
	struct nrf24_mac mac;
	int8_t channel;
	const char *name;
};

/* Converts nrf24_mac address to string */
int nrf24_mac2str(const struct nrf24_mac *, char *);

/* Converts string to nrf24_mac address */
int nrf24_str2mac(const char *, struct nrf24_mac *);

/* Generic response for all commands */
#define MGMT_CMD_NRF24_RSP			0x0101
struct mgmt_cmd_nrf24_rsp {
	uint16_t cmd;		/* CMD opcode related to this response */
	uint8_t status;		/* 0: success, 1: failure */
	uint16_t len;		/* Payload length */
	uint8_t payload[0];	/* Command specific returned response */
} __attribute__ ((packed));

/* Synchronous command to power UP nRF24 adapter */
#define MGMT_CMD_NRF24_POWER			0x0102
struct mgmt_cmd_nrf24_power {
	uint8_t on;		/* 1: Power ON, 0: Power OFF */
} __attribute__ ((packed));

/* Local nRF24 adapter: MAC address */
#define MGMT_CMD_NRF24_CONFIG		0x0103
struct mgmt_cmd_nrf24_config {
	struct nrf24_mac mac;	/* Local adapter nRF24 MAC address */
	uint8_t key[0];		/* FIXME: private key */
} __attribute__ ((packed));

/* Synchronous command to load/overwrite known devices/keys. */
#define MGMT_CMD_NRF24_LOAD_KEY			0x0104
struct mgmt_cmd_nrf24_load_key {
	struct nrf24_mac mac;		/* Peer */
	char key[0];			/* FIXME: */
} __attribute__ ((packed));

/* Synchronous command to enable broadcast: beaconing/presence/setup */
#define MGMT_CMD_NRF24_BCAST_ENABLE		0x0105
struct mgmt_cmd_nrf24_broadcast {
	uint8_t enable;		/* 1: enable, 0: disable */
} __attribute__ ((packed));

/*
 * Synchronous command to manage broadcast: beaconing/presence/setup
 * Interval is defined as the elapsed time between the begining of
 * windows. Consequently: window <= interval
 */
#define MGMT_CMD_NRF24_BCAST_PARAMS		0x0106
struct mgmt_cmd_nrf24_broadcast_params {
	uint8_t type;			/* 0: beacon, 1: presence, 2: setup */
	uint16_t window;		/* ms */
	uint16_t interval;		/* ms */
} __attribute__ ((packed));

/*
 * Synchronous command to set beaconing/presence/setup data included
 * in the payload field. Must follow BTLE advertising data format in
 * order to allow future radio & protocol interoperability.
 */
#define MGMT_CMD_NRF24_BCAST_DATA		0x0107
struct mgmt_cmd_nrf24_broadcast_data {
	uint8_t len;
	uint8_t payload[0];
};

/* Synchronous command to enable scanning: beaconing/presence/setup */
#define MGMT_CMD_NRF24_SCAN_ENABLE		0x0108
struct mgmt_cmd_nrf24_scan {
	uint8_t enable;		/* 1: enable, 0: disable */
} __attribute__ ((packed));

/*
 * Synchronous command to manage scanning: beaconing/presence/setup
 * Interval is defined as the elapsed time between the begining of
 * windows. Consequently: window <= interval
 */
#define MGMT_CMD_NRF24_SCAN_PARAMS		0x0109
struct mgmt_cmd_nrf24_scan_params {
	uint16_t window;		/* ms */
	uint16_t interval;		/* ms */
	uint8_t type;			/* 0xFF: All, 0: beacon only, 1: presence
					   only, 2: setup only */
	uint8_t filter;			/* 0: report all, 1: skip dupplicated */
} __attribute__ ((packed));

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
	struct nrf24_mac mac;	/* Address to disconnect */
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
	struct nrf24_mac mac;
	uint8_t name[0];
	/* TODO: Add device/service infos? */
} __attribute__ ((packed));

/*
 * New device has been added: keys are generated in the lower layer.
 * Userspace should store it and use management command to load the
 * keys after radio starts.
 */
#define MGMT_EVT_NRF24_KEY			0x0206 /* New key */
struct mgmt_evt_nrf24_key {
	struct nrf24_mac src;
	uint8_t key[0];			/* FIXME: size? */
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
