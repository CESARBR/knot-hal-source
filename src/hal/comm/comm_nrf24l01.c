/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef ARDUINO
#include "hal/avr_errno.h"
#include "hal/avr_unistd.h"
#else
#include "hal/linux_log.h"
#include <errno.h>
#include <unistd.h>
#endif

#include "hal/nrf24.h"
#include "hal/comm.h"
#include "hal/time.h"
#include "hal/config.h"
#include "nrf24l01.h"
#include "nrf24l01_ll.h"
#include "phy_driver.h"
#include "phy_driver_nrf24.h"


/*
 * Transmission time (ms) values for each pipe. Note that every next
 * pipe there's a increase on the trans and retrans time. For more
 * on how these values were calculated, please refer to  nrf24l01 specs
 * or check out this very commit message.
 */
#define PIPE1_WINDOW 15
#define PIPE2_WINDOW 18
#define PIPE3_WINDOW 22
#define PIPE4_WINDOW 26
#define PIPE5_WINDOW 30


#define _MIN(a, b)		((a) < (b) ? (a) : (b))
#define DATA_SIZE 128
#define MGMT_SIZE 32
#define MGMT_TIMEOUT 10

#define WINDOW_BCAST		5		/* ms */
#define INTERVAL_BCAST		60		/* ms */
#define BURST_BCAST		WINDOW_BCAST	/* 1:1 */

static uint8_t raw_timeout = 10;

/*Retransmission start time and channel offset*/
static uint8_t rt_stamp = 0;
static uint8_t rt_offset = 0;

#define SET_BIT(val, idx)	((val) |= 1 << (idx))
#define CLR_BIT(val, idx)	((val) &= ~(1 << (idx)))
#define CHK_BIT(val, idx)      ((val) & (1 << (idx)))

/* Global to know if listen function was called */
static uint8_t listen = 0;

/*
 * Bitmask to track assigned pipes.
 *
 * 0000 0001: pipe0
 * 0000 0010: pipe1
 * 0000 0100: pipe2
 * 0000 1000: pipe3
 * 0001 0000: pipe4
 * 0010 0000: pipe5
 */
#define PIPE_RAW_BITMASK	0b00111110 /* Map of RAW Pipes */

static uint8_t pipe_bitmask = 0b00000001; /* Default: scanning/broadcasting */

static struct nrf24_mac mac_local = {.address.uint64 = 0 };

/* Structure to save broadcast context */
struct nrf24_mgmt {
	int8_t pipe;
	uint8_t buffer_rx[MGMT_SIZE];
	size_t len_rx;
	uint8_t buffer_tx[MGMT_SIZE];
	size_t len_tx;
};

static struct nrf24_mgmt mgmt = {.pipe = -1, .len_rx = 0, .len_tx = 0};

/* Structure to save peers context */
struct nrf24_data {
	int8_t pipe;
	uint8_t buffer_rx[DATA_SIZE];
	size_t len_rx;
	uint8_t buffer_tx[DATA_SIZE];
	size_t len_tx;
	uint8_t seqnumber_tx;
	uint8_t seqnumber_rx;
	size_t offset_rx;
	unsigned long keepalive_anchor; /* Last packet received */
	uint8_t keepalive; /* zero: disabled or positive: window attempts */
	struct nrf24_mac mac;
};

#ifndef ARDUINO	/* If master then 5 peers */
static struct nrf24_data peers[5] = {
	{.pipe = -1, .len_rx = 0, .seqnumber_tx = 0,
		.seqnumber_rx = 0, .offset_rx = 0},
	{.pipe = -1, .len_rx = 0, .seqnumber_tx = 0,
		.seqnumber_rx = 0, .offset_rx = 0},
	{.pipe = -1, .len_rx = 0, .seqnumber_tx = 0,
		.seqnumber_rx = 0, .offset_rx = 0},
	{.pipe = -1, .len_rx = 0, .seqnumber_tx = 0,
		.seqnumber_rx = 0, .offset_rx = 0},
	{.pipe = -1, .len_rx = 0, .seqnumber_tx = 0,
		.seqnumber_rx = 0, .offset_rx = 0}
};
#else	/* If slave then 1 peer */
static struct nrf24_data peers[1] = {
	{.pipe = -1, .len_rx = 0, .seqnumber_tx = 0,
		.seqnumber_rx = 0, .offset_rx = 0},
};
#endif

/* ARRAY SIZE */
#define CONNECTION_COUNTER	((int) (sizeof(peers) \
				 / sizeof(peers[0])))

/*
 * TODO: Get this values from config file
 * Access Address for each pipe
 */
static uint8_t aa_pipe0[5] = {0x8D, 0xD9, 0xBE, 0x96, 0xDE};

/* Global to save driver index */
static int driverIndex = -1;

/*
 * Channel to management and raw data
 *
 * nRF24 channels has been selected to avoid common interference sources:
 * Wi-Fi channels (1, 6 and 11) and Bluetooth adversiting channels (37,
 * 38 and 39). Suggested nRF24 channels are channels 22 (2422 MHz), 50
 * (2450 MHz), 74 (2474 MHz), 76 (2476 MHz) and 97 (2497 MHz).
 */
static struct channel channel_mgmt = {.value = 76, .ack = false};
static struct channel channel_raw = {.value = 22, .ack = true};

enum {
	START_MGMT,
	MGMT,
	START_RAW,
	RAW
};

static int running_state = START_MGMT;

enum {
	PRESENCE,
	BURST_WINDOW,
	STANDBY,
	TIMEOUT_INTERVAL
};

static uint8_t presence_connect_state = PRESENCE;
static uint8_t previous_state = TIMEOUT_INTERVAL;

#ifdef ARDUINO
#define DBG_RECV(mac1, mac2, pdu, len)  DBG()
#define DBG_SEND(mac1, mac2, pdu, len)  DBG()

static void DBG(void)
{
}

#else

#define DBG_RECV(mac1, mac2, pdu, len)  DBG('<', mac1, mac2, pdu, len)
#define DBG_SEND(mac1, mac2, pdu, len)  DBG('>', mac1, mac2, pdu, len)

static void DBG(char dir, const struct nrf24_mac *mac,
			const struct nrf24_mac *peer_mac,
			const uint8_t *pdu, size_t len)
{
	char buffer[128];
	char *ptr = buffer + 50;
	size_t i;

	memset(buffer, 0, sizeof(buffer));

	nrf24_mac2str(mac, buffer);

	buffer[23] = ' ';
	buffer[24] = dir;
	buffer[25] = ' ';

	nrf24_mac2str(peer_mac, buffer + 26);
	buffer[49] = ' ';

	for (i = 0; i < len && ((i * 2) < sizeof(buffer)); i++) {
		snprintf(ptr, 3, "%02x", pdu[i]);
		ptr += 2;
	}

	hal_log_dbg("%s", buffer);
}
#endif

/*
 * Calculates data-channel time as a sum of each allocated pipe tr time.
 * This already accounts for startup time, time on air and other delays.
 * For more on this, please refer to nrf24l01 specs or to this commit's
 * log message.
 */
static uint8_t new_raw_time()
{
	uint8_t new_time = 0;

	if CHK_BIT(pipe_bitmask, 1)
		new_time+= PIPE1_WINDOW;
	if CHK_BIT(pipe_bitmask, 2)
		new_time+= PIPE2_WINDOW;
	if CHK_BIT(pipe_bitmask, 3)
		new_time+= PIPE3_WINDOW;
	if CHK_BIT(pipe_bitmask, 4)
		new_time+= PIPE4_WINDOW;
	if CHK_BIT(pipe_bitmask, 5)
		new_time+= PIPE5_WINDOW;

	rt_offset = new_time/3;
	if (rt_offset > 20)
		rt_offset -= 5;

	return new_time;
}

static inline int alloc_pipe(void)
{
	uint8_t i;

	for (i = 0; i < CONNECTION_COUNTER; i++) {
		if (peers[i].pipe == -1) {
			/* Peers initialization */
			memset(&peers[i], 0, sizeof(peers[i]));
			/* One peer for pipe*/
			peers[i].pipe = i+1;
			SET_BIT(pipe_bitmask, peers[i].pipe);
			return peers[i].pipe;
		}
	}

	/* No free pipe */
	return -1;
}

static int write_disconnect(int spi_fd, int sockfd, struct nrf24_mac dst,
				struct nrf24_mac src)
{
	struct nrf24_io_pack p;
	struct nrf24_ll_data_pdu *opdu =
		(struct nrf24_ll_data_pdu *) p.payload;
	struct nrf24_ll_crtl_pdu *llctrl =
		(struct nrf24_ll_crtl_pdu *) opdu->payload;
	struct nrf24_ll_disconnect *lldc =
		(struct nrf24_ll_disconnect *) llctrl->payload;
	int err, len;

	opdu->lid = NRF24_PDU_LID_CONTROL;
	p.pipe = sockfd;
	llctrl->opcode = NRF24_LL_CRTL_OP_DISCONNECT;
	lldc->dst_addr.address.uint64 = dst.address.uint64;
	lldc->src_addr.address.uint64 = src.address.uint64;
	len = sizeof(struct nrf24_ll_data_pdu)
		+ sizeof(struct nrf24_ll_crtl_pdu)
		+ sizeof(struct nrf24_ll_disconnect);

	DBG_SEND(&mac_local, &peers[sockfd - 1].mac,
				(const uint8_t *) opdu, len);

	err = phy_write(spi_fd, &p, len);
	if (err < 0)
		return err;

	return 0;
}

static int write_keepalive(int spi_fd, int sockfd, int keepalive_op,
				struct nrf24_mac dst, struct nrf24_mac src)
{
	int err, len;
	/* Assemble keep alive packet */
	struct nrf24_io_pack p;
	struct nrf24_ll_data_pdu *opdu =
		(struct nrf24_ll_data_pdu *) p.payload;
	struct nrf24_ll_crtl_pdu *llctrl =
		(struct nrf24_ll_crtl_pdu *) opdu->payload;
	struct nrf24_ll_keepalive *llkeepalive =
		(struct nrf24_ll_keepalive *) llctrl->payload;

	opdu->lid = NRF24_PDU_LID_CONTROL;
	p.pipe = sockfd;
	/* Keep alive opcode - Request or Response */
	llctrl->opcode = keepalive_op;
	/* src and dst address to keepalive */
	llkeepalive->dst_addr.address.uint64 = dst.address.uint64;
	llkeepalive->src_addr.address.uint64 = src.address.uint64;
	/* Sends keep alive packet */
	len = sizeof(*opdu) + sizeof(*llctrl) + sizeof(*llkeepalive);

	DBG_SEND(&mac_local, &peers[sockfd - 1].mac,
					(const uint8_t *) opdu, len);

	err = phy_write(spi_fd, &p, len);
	if (err < 0)
		return err;

	return 0;
}

static int check_keepalive(int spi_fd, int sockfd)
{
	uint32_t time_ms = hal_time_ms();

	/* Check if timeout occurred */
	if (hal_timeout(time_ms, peers[sockfd-1].keepalive_anchor,
						NRF24_KEEPALIVE_TIMEOUT_MS) > 0)
		return -ETIMEDOUT;

	/* Disabled? (Acceptor is always 0) */
	if (peers[sockfd-1].keepalive == 0)
		return 0;

	if (hal_timeout(time_ms, peers[sockfd-1].keepalive_anchor,
		peers[sockfd-1].keepalive * NRF24_KEEPALIVE_SEND_MS) <= 0)
		return 0;

	peers[sockfd-1].keepalive++;

	/* Sends keepalive packet */
	return write_keepalive(spi_fd, sockfd,
			      NRF24_LL_CRTL_OP_KEEPALIVE_REQ,
			      peers[sockfd-1].mac, mac_local);
}

static int write_mgmt(int spi_fd)
{
	int err;
	struct nrf24_io_pack p;

	/* If nothing to do */
	if (mgmt.len_tx == 0)
		return -EAGAIN;

	/* Set pipe to be sent */
	p.pipe = 0;
	/* Copy buffer_tx to payload */
	memcpy(p.payload, mgmt.buffer_tx, mgmt.len_tx);

	err = phy_write(spi_fd, &p, mgmt.len_tx);
	if (err < 0)
		return err;

	/* Reset len_tx */
	mgmt.len_tx = 0;

	return err;
}

static int read_mgmt(int spi_fd)
{
	struct nrf24_io_pack p;
	struct nrf24_ll_mgmt_pdu *ipdu = (struct nrf24_ll_mgmt_pdu *)p.payload;
	struct mgmt_evt_nrf24_bcast_presence *mgmtev_bcast;
	struct mgmt_evt_nrf24_connected *mgmtev_cn;
	struct mgmt_nrf24_header *mgmtev_hdr;
	struct nrf24_ll_mgmt_connect *llc;
	struct nrf24_ll_presence *llp;
	ssize_t ilen;

	/* Read from management pipe */
	p.pipe = 0;
	p.payload[0] = 0;
	/* Read data */
	ilen = phy_read(spi_fd, &p, NRF24_MTU);
	if (ilen <= 0)
		return -EAGAIN;

	/* If already has something in rx buffer then return BUSY */
	if (mgmt.len_rx != 0)
		return -EBUSY;

	/* Event header structure */
	mgmtev_hdr = (struct mgmt_nrf24_header *) mgmt.buffer_rx;

	switch (ipdu->type) {
	/* If is a presente type */
	case NRF24_PDU_TYPE_PRESENCE:

		/*
		 * If broadcasting: Ignore presence from other devices.
		 * TODO: Find a better approach to manage this scenario.
		 */
		if (listen)
			return -EAGAIN;

		if (ilen < (ssize_t) (sizeof(struct nrf24_ll_mgmt_pdu) +
					sizeof(struct nrf24_ll_presence)))
			return -EINVAL;

		/* Event presence structure */
		mgmtev_bcast = (struct mgmt_evt_nrf24_bcast_presence *)mgmtev_hdr->payload;
		/* Presence structure */
		llp = (struct nrf24_ll_presence *) ipdu->payload;

		/* Header type is a broadcast presence */
		mgmtev_hdr->opcode = MGMT_EVT_NRF24_BCAST_PRESENCE;
		mgmtev_hdr->index = 0;
		/* Copy source address */
		mgmtev_bcast->mac.address.uint64 = llp->mac.address.uint64;
		/*
		 * The packet structure contains the
		 * mgmt_pdu header, the MAC address
		 * and the slave name. The name length
		 * is equal to input length (ilen) minus
		 * header length and minus MAC address length.
		 */
		memcpy(mgmtev_bcast->name, llp->name,
				ilen - sizeof(*llp) - sizeof(*ipdu));

		/*
		 * The rx buffer length is equal to the
		 * event header length + presence packet length.
		 * Presence packet len = (input len - mgmt_pdu header len)
		 */
		mgmt.len_rx = ilen - sizeof(*ipdu) + sizeof(*mgmtev_hdr);

		break;
	/* If is a connect request type */
	case NRF24_PDU_TYPE_CONNECT_REQ:

		if (ilen != (sizeof(struct nrf24_ll_mgmt_pdu) +
			     sizeof(struct nrf24_ll_mgmt_connect)))
			return -EINVAL;

		/* Event connect structure */
		mgmtev_cn = (struct mgmt_evt_nrf24_connected *)mgmtev_hdr->payload;
		/* Link layer connect structure */
		llc = (struct nrf24_ll_mgmt_connect *) ipdu->payload;

		/* Header type is a connect request type */
		mgmtev_hdr->opcode = MGMT_EVT_NRF24_CONNECTED;
		mgmtev_hdr->index = 0;
		/* Copy src and dst address*/
		mgmtev_cn->src.address.uint64 = llc->src_addr.address.uint64;
		mgmtev_cn->dst.address.uint64 = llc->dst_addr.address.uint64;
		/* Copy channel */
		mgmtev_cn->channel = llc->channel;
		/* Copy access address */
		memcpy(mgmtev_cn->aa, llc->aa, sizeof(mgmtev_cn->aa));

		mgmt.len_rx = sizeof(*mgmtev_hdr) + sizeof(*mgmtev_cn);

		DBG_RECV(&llc->src_addr, &llc->dst_addr, (const uint8_t *) ipdu, ilen);

		break;
	default:
		return -EINVAL;
	}

	/* Returns the amount of bytes read */
	return ilen;
}

static int write_raw(int spi_fd, int sockfd)
{
	int err;
	struct nrf24_io_pack p;
	struct nrf24_ll_data_pdu *opdu = (void *)p.payload;
	size_t plen, left;

	/* If has nothing to send, returns EBUSY */
	if (peers[sockfd-1].len_tx == 0)
		return -EAGAIN;

	/* If len is larger than the maximum message size */
	if (peers[sockfd-1].len_tx > DATA_SIZE)
		return -EINVAL;

	/* Set pipe to be sent */
	p.pipe = sockfd;
	/* Amount of bytes to be sent */
	left = peers[sockfd-1].len_tx;

	while (left) {

		/* Delay to avoid sending all packets too fast */
		hal_delay_us(512);
		/*
		 * If left is larger than the NRF24_PW_MSG_SIZE,
		 * payload length = NRF24_PW_MSG_SIZE,
		 * if not, payload length = left
		 */
		plen = _MIN(left, NRF24_PW_MSG_SIZE);

		/*
		 * If left is larger than the NRF24_PW_MSG_SIZE,
		 * it means that the packet is fragmented,
		 * if not, it means that it is the last packet.
		 */
		opdu->lid = (left > NRF24_PW_MSG_SIZE) ?
			NRF24_PDU_LID_DATA_FRAG : NRF24_PDU_LID_DATA_END;

		/* Packet sequence number */
		opdu->nseq = peers[sockfd-1].seqnumber_tx;

		/* Offset = len - left */
		memcpy(opdu->payload, peers[sockfd-1].buffer_tx +
			(peers[sockfd-1].len_tx - left), plen);

		DBG_SEND(&mac_local, &peers[sockfd - 1].mac,
				 (const uint8_t *) opdu, plen + DATA_HDR_SIZE);

		/* Send packet */
		err = phy_write(spi_fd, &p, plen + DATA_HDR_SIZE);
		/*
		 * If write error then reset tx len
		 * and sequence number
		 */
		if (err < 0) {
			peers[sockfd-1].len_tx = 0;
			peers[sockfd-1].seqnumber_tx = 0;
			return err;
		}

		left -= plen;
		peers[sockfd-1].seqnumber_tx++;
	}

	err = peers[sockfd-1].len_tx;

	/* Resets controls */
	peers[sockfd-1].len_tx = 0;
	peers[sockfd-1].seqnumber_tx = 0;

	return err;
}

static int read_raw(int spi_fd, int sockfd)
{
	struct nrf24_io_pack p;
	const struct nrf24_ll_data_pdu *ipdu = (void *) p.payload;
	struct mgmt_nrf24_header *mgmtev_hdr;
	struct mgmt_evt_nrf24_disconnected *mgmtev_dc;
	struct nrf24_ll_disconnect *lldc;
	struct nrf24_ll_keepalive *llkeepalive;
	struct nrf24_ll_crtl_pdu *llctrl;
	size_t plen;
	ssize_t ilen;

	p.pipe = sockfd;
	p.payload[0] = 0;
	/*
	 * Reads the data while to exist,
	 * on success, the number of bytes read is returned
	 */
	while ((ilen = phy_read(spi_fd, &p, NRF24_MTU)) > 0) {

		/* Initiator/acceptor: reset anchor */
		DBG_RECV(&mac_local, &peers[sockfd - 1].mac, (const uint8_t *) ipdu, ilen);

		peers[sockfd-1].keepalive_anchor = hal_time_ms();

		/* Check if is data or Control */
		switch (ipdu->lid) {

		/* If is Control */
		case NRF24_PDU_LID_CONTROL:
			llctrl = (struct nrf24_ll_crtl_pdu *)ipdu->payload;
			llkeepalive = (struct nrf24_ll_keepalive *)
							llctrl->payload;
			lldc = (struct nrf24_ll_disconnect *) llctrl->payload;

			if (llctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_REQ &&
				llkeepalive->src_addr.address.uint64 ==
				peers[sockfd-1].mac.address.uint64 &&
				llkeepalive->dst_addr.address.uint64 ==
				mac_local.address.uint64) {
				write_keepalive(spi_fd, sockfd,
					NRF24_LL_CRTL_OP_KEEPALIVE_RSP,
					peers[sockfd-1].mac,
					mac_local);

			} else if (llctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_RSP) {
				/* Initiator: reset the counter */
				peers[sockfd-1].keepalive = 1;
			}

			/* If packet is disconnect request */
			else if (llctrl->opcode == NRF24_LL_CRTL_OP_DISCONNECT &&
							mgmt.len_rx == 0) {
				mgmtev_hdr = (struct mgmt_nrf24_header *)
								mgmt.buffer_rx;
				mgmtev_dc = (struct mgmt_evt_nrf24_disconnected *)
							mgmtev_hdr->payload;

				mgmtev_hdr->opcode = MGMT_EVT_NRF24_DISCONNECTED;
				mgmtev_dc->mac.address.uint64 =
					lldc->src_addr.address.uint64;
				mgmt.len_rx = sizeof(*mgmtev_hdr) +
							sizeof(*mgmtev_dc);
			}

			break;
		/* If is Data */
		case NRF24_PDU_LID_DATA_FRAG:
		case NRF24_PDU_LID_DATA_END:
			if (peers[sockfd-1].len_rx != 0)
				break; /* Discard packet */

			/* Incoming data: reset keepalive counter */
			peers[sockfd-1].keepalive = 1;

			/* Reset offset if sequence number is zero */
			if (ipdu->nseq == 0) {
				peers[sockfd-1].offset_rx = 0;
				peers[sockfd-1].seqnumber_rx = 0;
			}

			/* If sequence number error */
			if (peers[sockfd-1].seqnumber_rx < ipdu->nseq)
				break;
				/*
				 * TODO: disconnect, data error!?!?!?
				 * Illegal byte sequence
				 */

			if (peers[sockfd-1].seqnumber_rx > ipdu->nseq)
				break; /* Discard packet duplicated */

			/* Payloag length = input length - header size */
			plen = ilen - DATA_HDR_SIZE;

			if (ipdu->lid == NRF24_PDU_LID_DATA_FRAG &&
				plen < NRF24_PW_MSG_SIZE)
				break;
				/*
				 * TODO: disconnect, data error!?!?!?
				 * Not a data message
				 */

			/* Reads no more than DATA_SIZE bytes */
			if (peers[sockfd-1].offset_rx + plen > DATA_SIZE)
				plen = DATA_SIZE - peers[sockfd-1].offset_rx;

			memcpy(peers[sockfd-1].buffer_rx +
				peers[sockfd-1].offset_rx, ipdu->payload, plen);
			peers[sockfd-1].offset_rx += plen;
			peers[sockfd-1].seqnumber_rx++;

			/* If is DATA_END then put in rx buffer */
			if (ipdu->lid == NRF24_PDU_LID_DATA_END) {
				/* Sets packet length read */
				peers[sockfd-1].len_rx =
					peers[sockfd-1].offset_rx;

				/*
				 * If the complete msg is received,
				 * resets the controls
				 */
				peers[sockfd-1].seqnumber_rx = 0;
				peers[sockfd-1].offset_rx = 0;
			}
			break;
		}
	}

	return 0;
}

/*
 * This functions send presence packets during
 * windows_bcast time and go to standy by mode during
 * (interval_bcast - windows_bcast) time
 */
static void presence_connect(int spi_fd)
{
	struct nrf24_io_pack p;
	struct nrf24_ll_mgmt_pdu *opdu = (void *)p.payload;
	struct nrf24_ll_presence *llp =
				(struct nrf24_ll_presence *) opdu->payload;
	size_t len, nameLen;
	int err;
	static unsigned long start;

	switch (presence_connect_state) {
	case PRESENCE:
		/* Send Presence */
		if (mac_local.address.uint64 == 0)
			break;

		p.pipe = 0;
		opdu->type = NRF24_PDU_TYPE_PRESENCE;
		/* Send the mac address and thing name */
		llp->mac.address.uint64 = mac_local.address.uint64;

		len = sizeof(*opdu) + sizeof(*llp);

		/*
		 * Checks if need to truncate the name
		 * If header length + MAC length + name length is
		 * greater than MGMT_SIZE, then only sends the remaining.
		 * If not, sends the total name length.
		 */
		nameLen = (len + sizeof(THING_NAME) > MGMT_SIZE ?
				MGMT_SIZE - len : sizeof(THING_NAME));

		memcpy(llp->name, THING_NAME, nameLen);
		/* Increments name length */
		len += nameLen;

		err = phy_write(spi_fd, &p, len);
		if (err < 0)
			break;
		/* Init time */
		if (previous_state == TIMEOUT_INTERVAL)
			start = hal_time_ms();

		presence_connect_state = BURST_WINDOW;
		break;
	case BURST_WINDOW:
		if (hal_timeout(hal_time_ms(), start, WINDOW_BCAST) > 0)
			presence_connect_state = STANDBY;
		else if (hal_timeout(hal_time_ms(), start, BURST_BCAST) > 0)
			presence_connect_state = PRESENCE;

		previous_state = BURST_WINDOW;
		break;
	case STANDBY:
		phy_ioctl(spi_fd, NRF24_CMD_SET_STANDBY, NULL);
		presence_connect_state = TIMEOUT_INTERVAL;
		break;
	case TIMEOUT_INTERVAL:
		if (hal_timeout(hal_time_ms(), start, INTERVAL_BCAST) > 0)
			presence_connect_state = PRESENCE;

		previous_state = TIMEOUT_INTERVAL;
		break;
	}
}

static void running(void)
{
	struct mgmt_nrf24_header *mgmtev_hdr;
	struct mgmt_evt_nrf24_disconnected *mgmtev_dc;
	/* Index peers */
	static int sockIndex = 1;
	static unsigned long start;

	switch (running_state) {
	case START_MGMT:
		/* Set channel to management channel */
		phy_ioctl(driverIndex, NRF24_CMD_SET_CHANNEL, &channel_mgmt);
		/* Start timeout */
		start = hal_time_ms();
		/* Go to next state */
		running_state = MGMT;
		break;
	case MGMT:

		read_mgmt(driverIndex);
		write_mgmt(driverIndex);

		/* Broadcasting/acceptor */
		if (listen)
			presence_connect(driverIndex);

		/* Peers connected? */
		if (pipe_bitmask & PIPE_RAW_BITMASK) {
			if (hal_timeout(hal_time_ms(), start, MGMT_TIMEOUT) > 0)
				running_state = START_RAW;
		}
		break;

	case START_RAW:
		/* Set channel to data channel */
		phy_ioctl(driverIndex, NRF24_CMD_SET_CHANNEL, &channel_raw);
		/* Start timeout */
		start = hal_time_ms();

		/* Go to next state */
		running_state = RAW;
		break;
	case RAW:

		/* Start broadcast or scan? */
#ifdef ARDUINO
		if (!(pipe_bitmask & PIPE_RAW_BITMASK)) {
#else
		if ((pipe_bitmask & PIPE_RAW_BITMASK) != PIPE_RAW_BITMASK) {
#endif
			/*Checks for RAW timeout and RTs offset time*/
			if (hal_timeout(hal_time_ms(), start, raw_timeout) > 0 &&
			hal_timeout(hal_time_ms(), rt_stamp, (rt_offset)) > 0)
				running_state = START_MGMT;
		}
		/* Check if pipe is allocated */
		if (peers[sockIndex-1].pipe != -1) {
			read_raw(driverIndex, sockIndex);
			write_raw(driverIndex, sockIndex);
			rt_stamp = hal_time_ms();

			/*
			 * If keepalive is enabled
			 * Check if timeout occurred and generates
			 * disconnect event
			 */

			if (check_keepalive(driverIndex, sockIndex) == -ETIMEDOUT &&
				mgmt.len_rx == 0) {

				mgmtev_hdr = (struct mgmt_nrf24_header *)
								mgmt.buffer_rx;
				mgmtev_dc = (struct mgmt_evt_nrf24_disconnected *)
							mgmtev_hdr->payload;

				mgmtev_hdr->opcode = MGMT_EVT_NRF24_DISCONNECTED;

				mgmtev_dc->mac.address.uint64 =
					peers[sockIndex-1].mac.address.uint64;
				mgmt.len_rx = sizeof(*mgmtev_hdr) +
								sizeof(*mgmtev_dc);

				/* TODO: Send disconnect packet to slave */

				/* Free pipe & resize raw time */
				CLR_BIT(pipe_bitmask, peers[sockIndex - 1].pipe);
				peers[sockIndex - 1].pipe = -1;
				peers[sockIndex - 1].keepalive = 0;
				phy_ioctl(driverIndex, NRF24_CMD_RESET_PIPE,
								&sockIndex);
				raw_timeout = new_raw_time();
			}
		}

		sockIndex++;
		/* Resets sockIndex if sockIndex > CONNECTION_COUNTER */
		if (sockIndex > CONNECTION_COUNTER)
			sockIndex = 1;

		break;

	}
}

/* Global functions */
int hal_comm_init(const char *pathname, const void *params)
{

	const struct nrf24_mac *mac = (const struct nrf24_mac *)params;

	/* If driver not opened */
	if (driverIndex != -1)
		return -EPERM;

	/* Open driver and returns the driver index */
	driverIndex = phy_open(pathname);
	if (driverIndex < 0)
		return driverIndex;

	mac_local.address.uint64 = mac->address.uint64;

	return 0;
}

int hal_comm_deinit(void)
{
	int err;
	uint8_t i;

	/* If try to close driver with no driver open */
	if (driverIndex == -1)
		return -EPERM;

	/* Close driver */
	err = phy_close(driverIndex);
	if (err < 0)
		return err;

	/* Dereferencing driverIndex */
	driverIndex = -1;

	/* Clear all peers*/
	memset(peers, 0, sizeof(peers));
	for (i = 0; i < CONNECTION_COUNTER; i++) {
		peers[i].pipe = -1;
	}

	memset(&mgmt, 0, sizeof(mgmt));
	mgmt.pipe = -1;

	pipe_bitmask = 0b00000001;

	/* Reset machine states */
	running_state = START_MGMT;
	presence_connect_state = PRESENCE;
	previous_state = TIMEOUT_INTERVAL;

	return err;
}

int hal_comm_socket(int domain, int protocol)
{
	int retval;
	struct addr_pipe ap;

	/* If domain is not NRF24 */
	if (domain != HAL_COMM_PF_NRF24)
		return -EPERM;

	/* If not initialized */
	if (driverIndex == -1)
		return -EPERM;	/* Operation not permitted */

	switch (protocol) {

	case HAL_COMM_PROTO_MGMT:
		/* If Management, disable ACK and returns 0 */
		if (mgmt.pipe == 0)
			return -EUSERS; /* Returns too many users */
		retval = 0;
		mgmt.pipe = 0;

		/* Copy broadcast address */
		memcpy(ap.aa, aa_pipe0, sizeof(ap.aa));
		break;

	case HAL_COMM_PROTO_RAW:
		if (mgmt.pipe == -1) {
			/* If Management is not open*/
			mgmt.pipe = 0;
			retval = 0;
			/* Copy broadcast address */
			memcpy(ap.aa, aa_pipe0, sizeof(ap.aa));
			break;
		}
		/*
		 * If raw data, enable ACK
		 * and returns an available pipe
		 * from 1 to 5
		 */
		retval = alloc_pipe();
		/* If not pipe available */
		if (retval < 0)
			return -EUSERS; /* Returns too many users */

		/*
		 * Copy the 5 LSBs of master mac addres
		 * to access address and the last least
		 * significant byte is the pipe index.
		 */
		memcpy(ap.aa, &mac_local.address.b[3], sizeof(ap.aa));
		ap.aa[0] = (uint8_t)retval;

		break;

	default:
		return -EINVAL; /* Invalid argument */
	}

	ap.pipe = retval;

	/* Open pipe */
	phy_ioctl(driverIndex, NRF24_CMD_SET_PIPE, &ap);

	return retval;
}

int hal_comm_close(int sockfd)
{
	if (driverIndex == -1)
		return -EPERM;

	/* Pipe 0 is not closed because ACK arrives in this pipe */
	if (sockfd >= 1 && sockfd <= 5 && peers[sockfd-1].pipe != -1) {
		/* Send disconnect packet */
		if (mac_local.address.uint64 != 0)
			/* Slave side */
			write_disconnect(driverIndex, sockfd,
					peers[sockfd-1].mac, mac_local);
		/* Free pipe & & resize raw time */
		CLR_BIT(pipe_bitmask, peers[sockfd - 1].pipe);
		peers[sockfd-1].pipe = -1;
		phy_ioctl(driverIndex, NRF24_CMD_RESET_PIPE, &sockfd);
		raw_timeout = new_raw_time();
		/* Disable to send keep alive request */
		peers[sockfd-1].keepalive = 0;
	}

	return 0;
}

ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{
	size_t length = 0;

	/* Run background procedures */
	running();

	if (sockfd < 0 || sockfd > 5 || count == 0)
		return -EINVAL;

	/* If management */
	if (sockfd == 0) {
		/* If has something to read */
		if (mgmt.len_rx != 0) {
			/*
			 * If the amount of bytes available
			 * to be read is greather than count
			 * then read count bytes
			 */
			length = mgmt.len_rx > count ? count : mgmt.len_rx;
			/* Copy rx buffer */
			memcpy(buffer, mgmt.buffer_rx, length);

			/* Reset rx len */
			mgmt.len_rx = 0;
		} else /* Return -EAGAIN has nothing to be read */
			return -EAGAIN;

	} else if (peers[sockfd-1].len_rx != 0) {
		/*
		 * If the amount of bytes available
		 * to be read is greather than count
		 * then read count bytes
		 */
		length = peers[sockfd-1].len_rx > count ?
				 count : peers[sockfd-1].len_rx;
		/* Copy rx buffer */
		memcpy(buffer, peers[sockfd-1].buffer_rx, length);
		/* Reset rx len */
		peers[sockfd-1].len_rx = 0;
	} else
		return -EAGAIN;

	/* Returns the amount of bytes read */
	return length;
}


ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{

	/* Run background procedures */
	running();

	if (sockfd < 1 || sockfd > 5 || count == 0 || count > DATA_SIZE)
		return -EINVAL;

	/* If already has something to write then returns busy */
	if (peers[sockfd-1].len_tx != 0)
		return -EBUSY;

	/* Copy data to be write in tx buffer */
	memcpy(peers[sockfd-1].buffer_tx, buffer, count);
	peers[sockfd-1].len_tx = count;

	return count;
}

int hal_comm_listen(int sockfd)
{
	/* Init listen */
	listen = 1;

	/* pipe0 used for broadcasting/scanning */
	SET_BIT(pipe_bitmask, 0);

	return 0;
}

int hal_comm_accept(int sockfd, void *addr)
{
	struct nrf24_mac *mac = (struct nrf24_mac *) addr;

	/* TODO: Run background procedures */
	struct mgmt_nrf24_header *mgmtev_hdr =
				(struct mgmt_nrf24_header *) mgmt.buffer_rx;
	struct mgmt_evt_nrf24_connected *mgmtev_cn =
			(struct mgmt_evt_nrf24_connected *)mgmtev_hdr->payload;
	struct addr_pipe p_addr;
	int pipe;
	/* Run background procedures */
	running();

	if (mgmt.len_rx == 0)
		return -EAGAIN;

	/* Free management read to receive new packet */
	mgmt.len_rx = 0;

	if (mgmtev_hdr->opcode != MGMT_EVT_NRF24_CONNECTED ||
		mgmtev_cn->dst.address.uint64 != mac_local.address.uint64)
		return -EAGAIN;


	pipe = alloc_pipe();
	/* If not pipe available */
	if (pipe < 0)
		return -EUSERS; /* Returns too many users */

	/* If accept then stop listen */
	listen = 0;

	/* Set aa in pipe */
	p_addr.pipe = pipe;
	memcpy(p_addr.aa, mgmtev_cn->aa, sizeof(p_addr.aa));
	/*open pipe*/
	phy_ioctl(driverIndex, NRF24_CMD_SET_PIPE, &p_addr);
	/*Resize data channel time*/
	raw_timeout = new_raw_time();

	/* Source address for keepalive message */
	peers[pipe-1].mac.address.uint64 =
		mgmtev_cn->src.address.uint64;
	/* Disable keep alive request */
	peers[pipe-1].keepalive = 0;
	/* Start timeout */
	peers[pipe-1].keepalive_anchor = hal_time_ms();

	/* Copy peer address */
	mac->address.uint64 = mgmtev_cn->src.address.uint64;

	/* Return pipe */
	return pipe;
}


int hal_comm_connect(int sockfd, uint64_t *addr)
{
	struct nrf24_ll_mgmt_pdu *opdu =
		(struct nrf24_ll_mgmt_pdu *)mgmt.buffer_tx;
	struct nrf24_ll_mgmt_connect *payload =
				(struct nrf24_ll_mgmt_connect *) opdu->payload;
	size_t len;

	/* Run background procedures */
	running();

	/* If already has something to write then returns busy */
	if (mgmt.len_tx != 0)
		return -EBUSY;

	opdu->type = NRF24_PDU_TYPE_CONNECT_REQ;

	payload->src_addr = mac_local;
	payload->dst_addr.address.uint64 = *addr;
	payload->channel = channel_raw.value;
	/*
	 * Set in payload the addr to be set in client.
	 * sockfd contains the pipe allocated for the client
	 * aa_pipes contains the Access Address for each pipe
	 */

	/*
	 * Copy the 5 LSBs of master mac addres
	 * to access address and the last least
	 * significant byte is the socket index.
	 */

	memcpy(payload->aa, &mac_local.address.b[3],
		sizeof(payload->aa));
	payload->aa[0] = (uint8_t)sockfd;

	/* Source address for keepalive message */
	peers[sockfd-1].mac.address.uint64 = *addr;

	len = sizeof(struct nrf24_ll_mgmt_connect);
	len += sizeof(struct nrf24_ll_mgmt_pdu);

	/* Start timeout */
	peers[sockfd-1].keepalive_anchor = hal_time_ms();
	/* Enable keep alive: 5 attempts until timeout */
	peers[sockfd-1].keepalive = 1;
	mgmt.len_tx = len;

	return 0;
}

int nrf24_str2mac(const char *str, struct nrf24_mac *mac)
{
	/* Parse the input string into 8 bytes */
	int rc = sscanf(str,
		"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
		&mac->address.b[0], &mac->address.b[1], &mac->address.b[2],
		&mac->address.b[3], &mac->address.b[4], &mac->address.b[5],
		&mac->address.b[6], &mac->address.b[7]);

	return (rc != 8 ? -1 : 0);
}

int nrf24_mac2str(const struct nrf24_mac *mac, char *str)
{
	/* Write nrf24_mac.address into string buffer in hexadecimal format */
	int rc = sprintf(str,
		"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
			mac->address.b[0], mac->address.b[1], mac->address.b[2],
			mac->address.b[3], mac->address.b[4], mac->address.b[5],
			mac->address.b[6], mac->address.b[7]);

	return (rc != 23 ? -1 : 0);
}
