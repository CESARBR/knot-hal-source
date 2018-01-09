/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

/* Common to all devices */
#define NRF24_AA_MGMT_CHANNEL		0xDE96BED98D /* Access Address used for management */

/*
 * Recommended access address assignment for data channel
 * distributed by the master: B4B3B2B1B0
 * B4, B3 and B2 defined below, remaining 2 octets (B1, B0)
 * may follow the following recommended assignment:
 * B1: GW identification
 * B0: User identification (GW should assign)
 */

/*
 * PDUs transmitted over managementchannel: pipe0
 * 'Management' channel is reserved to presence announcement
 * (or beaconing) and initial connection establishment.
 */
#define NRF24_PDU_TYPE_BEACON		0x00	/* Beaconing/Broadcast */
#define NRF24_PDU_TYPE_PRESENCE		0x01	/* Connectable mode */
#define NRF24_PDU_TYPE_SETUP		0x02	/* Setup mode */
#define NRF24_PDU_TYPE_CONNECT_REQ	0x03	/* Master to slave */
/* 04 to 07 are reserved to future use */

struct nrf24_ll_mgmt_pdu {
	uint8_t type:4;
	uint8_t rfu:4;
	uint8_t payload[0];		/* pack beacon of mgmt frames */
} __attribute__ ((packed));

/*
 * Used at ll_mgmt_channel_pdu.payload
 * by slave to announce his presence.
 * The packet contains the name and its mac address.
 */
struct nrf24_ll_presence {
	struct nrf24_mac mac;	/* Source address */
	uint8_t name[0];		/* Slave name */
} __attribute__ ((packed));


/*
 * Used at ll_mgmt_channel_pdu.payload
 * Master assigns the channel and access address of the slaves.
 * TODO: event should be added to notify disconnection to upper layer
 */
struct nrf24_ll_mgmt_connect {
	struct nrf24_mac src_addr;	/* Source address */
	struct nrf24_mac dst_addr;	/* Destination address */
	uint8_t channel;	/* nRF24 channel: nRF24 spec page 25 */
	uint8_t aa[5];		/* Access Address: nRF24 spec page 28 */
	uint64_t rfu;		/* Reserved for future use */
} __attribute__ ((packed));

/*
 * TODO: payload of mgmt frames may have class of device or appearance,
 * operating behaviour (intervals), supported services, service specific
 * data or URL.
 *
 * Protocol multiplexer should be later exposed to the user informing
 * which protocol is being transfered over a given 'pm'.
 */

/*
 * PDUs transmitted over data channel: pipe{1,2,3,4,5}
 * 'Data' channel is reserved to link control and user traffic.
 */

/* Logical channel ids */
#define NRF24_PDU_LID_DATA_FRAG		0x00 /* Data: Beginning or fragment */
#define NRF24_PDU_LID_DATA_END		0x01 /* Data: End of fragment or complete */
#define NRF24_PDU_LID_CONTROL		0x03 /* Control */

struct nrf24_ll_data_pdu {
	uint8_t lid:2;	/* 00 (data frag), 01 (data complete), 11: (control) */
	uint8_t nseq:6;	/* Fragment sequence number */
	uint8_t rfu;  /* Reserved for future use */
	uint8_t payload[0];
} __attribute__ ((packed));

#define DATA_HDR_SIZE 			sizeof(struct nrf24_ll_data_pdu)
#define NRF24_PW_MSG_SIZE		(NRF24_MTU - DATA_HDR_SIZE)
/* 6 bits (64 packets) are reserved to sequence number*/
#define NRF24_MAX_MSG_SIZE		(64 * NRF24_PW_MSG_SIZE)

struct nrf24_ll_crtl_pdu {
	uint8_t opcode;
	uint8_t payload[0];
} __attribute__ ((packed));

#define NRF24_KEEPALIVE_SEND_MS		750

#define NRF24_KEEPALIVE_TIMEOUT_MS	(10 * NRF24_KEEPALIVE_SEND_MS)

/*
 * Keep alive should be sent every 2560ms. Timeout happens
 * after 25600ms (10 keep alive without response).
 */
#define NRF24_LL_CRTL_OP_KEEPALIVE_REQ	0x01
#define NRF24_LL_CRTL_OP_KEEPALIVE_RSP	0x02

struct nrf24_ll_keepalive {
	struct nrf24_mac src_addr;	/* Source address */
	struct nrf24_mac dst_addr;	/* Destination address */
} __attribute__ ((packed));


/* Sent automatically from both sides after establishing connection */
#define NRF24_LL_CRTL_OP_VERSION_IND	0x03
struct nrf24_ll_version_ind {
	uint8_t major;
	uint8_t minor;
} __attribute__ ((packed));

/*Slave to master */
#define NRF24_LL_CRTL_OP_DISCONNECT	0x04
struct nrf24_ll_disconnect {
	struct nrf24_mac src_addr;	/* Source address */
	struct nrf24_mac dst_addr;	/* Destination address */
} __attribute__ ((packed));
