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
#include "hal/avr_log.h"
#else
#include "hal/linux_log.h"
#include <errno.h>
#include <unistd.h>
#endif

#include "hal/nrf24.h"
#include "hal/nrf24_ll.h"
#include "hal/comm.h"
#include "hal/time.h"
#include "hal/config.h"
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

#define WINDOW_BCAST		5				/* ms */
#define INTERVAL_BCAST		6				/* ms */
#define BURST_BCAST		(WINDOW_BCAST*1000)/10		/* 500 us */

static uint8_t raw_timeout = 10;

/*Retransmission start time and channel offset*/
static uint8_t rt_stamp = 0;
static uint8_t rt_offset = 0;

#define SET_BIT(val, idx)	((val) |= 1 << (idx))
#define CLR_BIT(val, idx)	((val) &= ~(1 << (idx)))
#define CHK_BIT(val, idx)      ((val) & (1 << (idx)))

/* Global to know if listen function was called */
static uint8_t listen = 0;
static int sockfd_counter = 0;

/*
 * Amount of sockets/clients using the pipes
 * pipe0 <-> Index 0: reserved to broadcasting
 * pipe1..5 <-> Index 1..5: reserved to regular clients
 */
static uint8_t pipe_refs[] = { 0, 0, 0, 0, 0, 0};

/*
 * Bitmask to track assigned pipes.
 * Pipes are resources shared between nRf24 'sockets'. Currently
 * only one pipe will be assigned to a unique socket. However, this
 * approach can be extended in the future allowing more than one client
 * 'socket' per pipe at different time slot.
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

/* Structure to save peers context */
struct nrf24_raw {
	struct nrf24_mac mac;	/* Peer MAC address */
	uint8_t pipe; /* 1 to 5 */
	int sockfd;
	uint8_t buffer_rx[DATA_SIZE];
	size_t len_rx;
	uint8_t buffer_tx[DATA_SIZE];
	size_t len_tx;

	uint8_t seqnumber_tx;
	uint8_t seqnumber_rx;
	size_t offset_rx;
	unsigned long keepalive_anchor; /* Last packet received */
	uint8_t keepalive; /* zero: disabled or positive: window attempts */
};

#ifndef ARDUINO	/* If master then 5 peers */
static struct nrf24_raw peers[5] = {
	{.sockfd = -1, .pipe = 0, .len_rx = 0, .seqnumber_tx = 0,
				.seqnumber_rx = 0, .offset_rx = 0},
	{.sockfd = -1, .pipe = 0, .len_rx = 0, .seqnumber_tx = 0,
				.seqnumber_rx = 0, .offset_rx = 0},
	{.sockfd = -1, .pipe = 0, .len_rx = 0, .seqnumber_tx = 0,
				.seqnumber_rx = 0, .offset_rx = 0},
	{.sockfd = -1, .pipe = 0, .len_rx = 0, .seqnumber_tx = 0,
				.seqnumber_rx = 0, .offset_rx = 0},
	{.sockfd = -1, .pipe = 0, .len_rx = 0, .seqnumber_tx = 0,
				.seqnumber_rx = 0, .offset_rx = 0}
};
#else	/* If slave then 1 peer  and 1 Broadcast channel */
static struct nrf24_raw peers[2] = {
	{.sockfd = -1, .pipe = 0, .len_rx = 0, .seqnumber_tx = 0,
				.seqnumber_rx = 0, .offset_rx = 0},
	{.sockfd = -1, .pipe = 0, .len_rx = 0, .seqnumber_tx = 0,
				.seqnumber_rx = 0, .offset_rx = 0},
};
#endif

/* ARRAY SIZE */
#define MAX_PEERS	((int) (sizeof(peers) \
				 / sizeof(struct nrf24_raw)))

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
static int channel_mgmt = 76;
static int channel_raw = 22;

enum {
	START_MGMT,
	MGMT,
	START_RAW,
	RAW
};

enum {
	PRESENCE,
	BURST_WINDOW,
	STANDBY,
	TIMEOUT_INTERVAL
};

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

static int pipe_raw_alloc(void)
{
	uint8_t i;
	int index = sizeof(pipe_refs) - 1;
	int min = pipe_refs[index];

	/* Search the last occupied pipe */
	for (i = sizeof(pipe_refs) - 1; i > 0; i--) {
		if (pipe_refs[i] < min) {
			min = pipe_refs[i];
			index = i;
		}
	}

	/* At the moment, it is restricted to one client per pipe */
	if (min > 0)
		return -EUSERS;

	pipe_refs[index]++;

	return index;
}

/* pipe0 to pipe5 */
static void pipe_ref(uint8_t pipe)
{
	if (pipe < sizeof(pipe_refs))
		pipe_refs[pipe]++;
}

/* pipe0 to pipe5 */
static void pipe_unref(uint8_t pipe)
{
	if (pipe < sizeof(pipe_refs))
		pipe_refs[pipe]--;
}

static struct nrf24_raw *peer_alloc(int pipe)
{
	struct nrf24_raw *peer;
	int i;

	for (i = 0; i < MAX_PEERS; i++) {
		peer = &peers[i];
		if (peer->sockfd == -1) {
			peer->sockfd = ++sockfd_counter;
			peer->pipe = pipe;

			return peer;
		}
	}

	return NULL;
}

static struct nrf24_raw *peer_get(int sockfd)
{
	int i;

	for (i = 0; i < MAX_PEERS; i++) {
		if (sockfd == peers[i].sockfd)
			return &peers[i];
	}

	return NULL;
}

static int peer_unref(struct nrf24_raw *peer)
{
	uint8_t pipe;

	/* Release pipe */
	pipe = peer->pipe;
	if (pipe)
		pipe_unref(pipe);

	/* Reset to default values */
	peer->sockfd = -1;
	peer->pipe = 0;
	peer->len_rx = 0;
	peer->seqnumber_tx = 0;
	peer->seqnumber_rx = 0;
	peer->offset_rx = 0;
	peer->keepalive = 0;

	return 0;
}

static int write_keepalive(int spi_fd, const struct nrf24_raw *peer,
				int keepalive_op, struct nrf24_mac dst,
				struct nrf24_mac src)
{
	/* Assemble keep alive packet */
	struct nrf24_io_pack p;
	struct nrf24_ll_data_pdu *opdu =
		(struct nrf24_ll_data_pdu *) p.payload;
	struct nrf24_ll_crtl_pdu *llctrl =
		(struct nrf24_ll_crtl_pdu *) opdu->payload;
	struct nrf24_ll_keepalive *llkeepalive =
		(struct nrf24_ll_keepalive *) llctrl->payload;
	int err, len;

	opdu->lid = NRF24_PDU_LID_CONTROL;
	p.pipe = peer->pipe;
	/* Keep alive opcode - Request or Response */
	llctrl->opcode = keepalive_op;
	/* src and dst address to keepalive */
	llkeepalive->dst_addr.address.uint64 = dst.address.uint64;
	llkeepalive->src_addr.address.uint64 = src.address.uint64;
	/* Sends keep alive packet */
	len = sizeof(*opdu) + sizeof(*llctrl) + sizeof(*llkeepalive);

	err = phy_write(spi_fd, &p, len);
	if (err < 0)
		return err;

	return 0;
}

static int check_keepalive(int spi_fd, struct nrf24_raw *peer)
{
	uint32_t time_ms = hal_time_ms();

	/* Check if timeout occurred */
	if (hal_timeout(time_ms, peer->keepalive_anchor,
						NRF24_KEEPALIVE_TIMEOUT_MS) > 0)
		return -ETIMEDOUT;

	/* Disabled? (Acceptor is always 0) */
	if (peer->keepalive == 0)
		return 0;

	if (hal_timeout(time_ms, peer->keepalive_anchor,
		peer->keepalive * NRF24_KEEPALIVE_SEND_MS) <= 0)
		return 0;

	peer->keepalive++;

	/* Sends keepalive packet */
	return write_keepalive(spi_fd, peer,
			      NRF24_LL_CRTL_OP_KEEPALIVE_REQ,
			      peer->mac, mac_local);
}

static ssize_t read_channel_broadcast(const uint8_t *payload, size_t plen,
						uint8_t *buffer, size_t blen)
{
	struct nrf24_ll_mgmt_pdu *ipdu = (struct nrf24_ll_mgmt_pdu *) payload;
	struct nrf24_ll_mgmt_connect *llc;
	ssize_t olen;

	switch (ipdu->type) {
	/* If is a presente type */
	case NRF24_PDU_TYPE_PRESENCE:
		/*
		 * If broadcasting: Ignore presence from other devices.
		 * TODO: Find a better approach to manage this scenario.
		 */
		if (listen)
			return -EAGAIN;

		if (plen < (ssize_t) (sizeof(struct nrf24_ll_mgmt_pdu) +
					sizeof(struct nrf24_ll_presence)))
			return -EINVAL;

		if (plen > blen)
			return -EPROTO;

		/* Presence structure */
		memcpy(buffer, payload, plen);
		olen = plen;

		break;
	/* If is a connect request type */
	case NRF24_PDU_TYPE_CONNECT_REQ:

		if (plen != (sizeof(struct nrf24_ll_mgmt_pdu) +
			     sizeof(struct nrf24_ll_mgmt_connect)))
			return -EINVAL;

		/* Link layer connect structure */
		llc = (struct nrf24_ll_mgmt_connect *) ipdu->payload;

		/* Ignore if it is not addressed to local address */
		if (mac_local.address.uint64 != llc->dst_addr.address.uint64)
			return -EAGAIN;

		memcpy(buffer, payload, plen);
		olen = plen;

		break;
	default:
		return -EINVAL;
	}

	/* Returns the amount of bytes read */
	return olen;
}

static int write_raw(int spi_fd, struct nrf24_raw *peer)
{
	int err;
	struct nrf24_io_pack p;

	/* If nothing to do */
	if (peer->len_tx == 0)
		return -EAGAIN;

	/* Set pipe to be sent */
	p.pipe = 0;
	memcpy(p.payload, peer->buffer_tx, peer->len_tx);

	err = phy_write(spi_fd, &p, peer->len_tx);
	if (err < 0)
		return err;

	/* Reset len_tx */
	peer->len_tx = 0;

	return err;

}

static int write_raw_data(int spi_fd, struct nrf24_raw *peer)
{
	struct nrf24_io_pack p;
	struct nrf24_ll_data_pdu *opdu = (void *) p.payload;
	size_t plen, left;
	int err;

	/* If has nothing to send, returns EBUSY */
	if (peer->len_tx == 0)
		return -EAGAIN;

	/* If len is larger than the maximum message size */
	if (peer->len_tx > DATA_SIZE)
		return -EINVAL;

	/* Set pipe to be sent */
	p.pipe = peer->pipe;

	/* Amount of bytes to be sent */
	left = peer->len_tx;

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
		opdu->nseq = peer->seqnumber_tx;

		/* Offset = len - left */
		memcpy(opdu->payload, peer->buffer_tx + (peer->len_tx - left),
									plen);

		/* Send packet */
		err = phy_write(spi_fd, &p, plen + DATA_HDR_SIZE);
		/*
		 * If write error then reset tx len
		 * and sequence number
		 */
		if (err < 0) {
			peer->len_tx = 0;
			peer->seqnumber_tx = 0;
			return err;
		}

		left -= plen;
		peer->seqnumber_tx++;
	}

	err = peer->len_tx;

	/* Resets controls */
	peer->len_tx = 0;
	peer->seqnumber_tx = 0;

	return err;
}

static ssize_t read_channel_data(struct nrf24_raw *peer,
					const uint8_t *payload, size_t plen)
{
	const struct nrf24_ll_data_pdu *ipdu = (void *) payload;
	const struct nrf24_ll_crtl_pdu *llctrl;
	const struct nrf24_ll_keepalive *llkeepalive;

	/* TODO: avoiding passing peer */

	/* Check if is data or Control */
	switch (ipdu->lid) {

		/* If is Control */
	case NRF24_PDU_LID_CONTROL:
		llctrl = (struct nrf24_ll_crtl_pdu *)ipdu->payload;
		llkeepalive = (struct nrf24_ll_keepalive *) llctrl->payload;

		if (llctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_REQ &&
		    llkeepalive->src_addr.address.uint64 ==
		    peer->mac.address.uint64 &&
		    llkeepalive->dst_addr.address.uint64 ==
		    mac_local.address.uint64) {
			write_keepalive(driverIndex, peer,
					NRF24_LL_CRTL_OP_KEEPALIVE_RSP,
					peer->mac, mac_local);

		} else if (llctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_RSP) {
			/* Initiator: reset the counter */
			peer->keepalive = 1;
		}

		/* If packet is disconnect request */
		else if (llctrl->opcode == NRF24_LL_CRTL_OP_DISCONNECT)  {
			/* TODO: Manage disconnect request */
		}

		break;
		/* If is Data */
	case NRF24_PDU_LID_DATA_FRAG:
	case NRF24_PDU_LID_DATA_END:
		if (peer->len_rx != 0)
			break; /* Discard packet */

		/* Incoming data: reset keepalive counter */
		peer->keepalive = 1;

		/* Reset offset if sequence number is zero */
		if (ipdu->nseq == 0) {
			peer->offset_rx = 0;
			peer->seqnumber_rx = 0;
		}

		/* If sequence number error */
		if (peer->seqnumber_rx < ipdu->nseq)
			break;
		/*
		 * TODO: disconnect, data error!?!?!?
		 * Illegal byte sequence
		 */

		if (peer->seqnumber_rx > ipdu->nseq)
			break; /* Discard packet duplicated */

		/* FIXME: review */
		/* Payloag length = input length - header size */
		plen = plen - DATA_HDR_SIZE;

		if (ipdu->lid == NRF24_PDU_LID_DATA_FRAG &&
		    plen < NRF24_PW_MSG_SIZE)
			break;
		/*
		 * TODO: disconnect, data error!?!?!?
		 * Not a data message
		 */

		/* Reads no more than DATA_SIZE bytes */
		if (peer->offset_rx + plen > DATA_SIZE)
			plen = DATA_SIZE - peer->offset_rx;

		memcpy(peer->buffer_rx + peer->offset_rx,
		       ipdu->payload, plen);
		peer->offset_rx += plen;
		peer->seqnumber_rx++;

		/* If is DATA_END then put in rx buffer */
		if (ipdu->lid == NRF24_PDU_LID_DATA_END) {
			/* Sets packet length read */
			peer->len_rx =
				peer->offset_rx;

			/*
			 * If the complete msg is received,
			 * resets the controls
			 */
			peer->seqnumber_rx = 0;
			peer->offset_rx = 0;
		}
		break;
	}

	return 0;
}

static int read_raw(int spi_fd, struct nrf24_raw *peer)
{
	struct nrf24_io_pack p;
	ssize_t ilen, len_rx;

	p.pipe = peer->pipe;
	/*
	 * Reads the data while to exist,
	 * on success, the number of bytes read is returned
	 */
	while ((ilen = phy_read(spi_fd, &p, NRF24_MTU)) > 0) {
		if (p.pipe != 0) {
			/* RAW data (including control) channel */
			peer->keepalive_anchor = hal_time_ms();
			len_rx = read_channel_data(peer, p.payload, ilen);
		} else {
			/* Broadcasting channel */
			len_rx = read_channel_broadcast(p.payload, ilen,
							peer->buffer_rx,
							sizeof(peer->buffer_rx));
		}

		if (len_rx > 0)
			peer->len_rx = len_rx;
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
	static unsigned long start;
	/* Start timeout */
	static uint8_t state = PRESENCE;
	static uint8_t previous_state = TIMEOUT_INTERVAL;

	switch (state) {
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

		phy_write(spi_fd, &p, len);

		/* Init time */
		if (previous_state == TIMEOUT_INTERVAL)
			start = hal_time_ms();

		state = BURST_WINDOW;
		break;
	case BURST_WINDOW:

		if (hal_timeout(hal_time_ms(), start, WINDOW_BCAST) > 0)
			state = STANDBY;
		else if (hal_timeout(hal_time_us(), start*1000, BURST_BCAST) > 0)
			state = PRESENCE;

		previous_state = BURST_WINDOW;
		break;
	case STANDBY:
		phy_ioctl(spi_fd, NRF24_CMD_SET_STANDBY, NULL);
		state = TIMEOUT_INTERVAL;
		break;
	case TIMEOUT_INTERVAL:
		if (hal_timeout(hal_time_ms(), start, INTERVAL_BCAST) > 0)
			state = PRESENCE;

		previous_state = TIMEOUT_INTERVAL;
		break;
	}
}

static void running(void)
{
	static int state = START_MGMT;
	static unsigned long start;

	switch (state) {
	case START_MGMT:
		/* Set channel to management channel */
		phy_ioctl(driverIndex, NRF24_CMD_SET_CHANNEL, &channel_mgmt);
		/* Start timeout */
		start = hal_time_ms();
		/* Go to next state */
		state = MGMT;
		break;
	case MGMT:

		read_raw(driverIndex, &peers[0]);
		write_raw(driverIndex, &peers[0]);

		/* Broadcasting/acceptor */
		if (listen)
			presence_connect(driverIndex);

		/* TODO: Switch to RAW if connected to at least one peer */
		if (hal_timeout(hal_time_ms(), start, MGMT_TIMEOUT) > 0)
			state = START_RAW;

		break;

	case START_RAW:
		/* Set channel to data channel */
		phy_ioctl(driverIndex, NRF24_CMD_SET_CHANNEL, &channel_raw);
		/* Start timeout */
		start = hal_time_ms();

		/* Go to next state */
		state = RAW;
		break;
	case RAW:

		/* TODO: Switch to MGMT if not connected to all possible peers */
#if 0
		/* Checks for RAW timeout and RTs offset time */
		if (hal_timeout(hal_time_ms(), start, raw_timeout) > 0 &&
		    hal_timeout(hal_time_ms(), rt_stamp, (rt_offset)) > 0)
			state = START_MGMT;
#else
		if (hal_timeout(hal_time_ms(), start, raw_timeout))
			state = START_MGMT;

#endif

		read_raw(driverIndex, &peers[1]);
		write_raw_data(driverIndex, &peers[1]);
		rt_stamp = hal_time_ms();

		/*
		 * If keepalive is enabled
		 * Check if timeout occurred and generates
		 * disconnect event
		 */

		if (check_keepalive(driverIndex, &peers[1]) == -ETIMEDOUT) {
			/* TODO: Send disconnect packet to slave */
			peer_unref(&peers[1]);
			raw_timeout = new_raw_time();
		}

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

	/* Clear all peers */
	for (i = 0; i < MAX_PEERS; i++)
		peers[i].pipe = 0;

	pipe_bitmask = 0b00000001;
	/* Close driver */
	err = phy_close(driverIndex);
	if (err < 0)
		return err;

	/* Dereferencing driverIndex */
	driverIndex = -1;

	return err;
}

int hal_comm_socket(int domain, int protocol)
{
	struct addr_pipe ap;
	struct nrf24_raw *sock;
	uint8_t pipe = 0;

	/* TODO: domain and protocol are not used */

	/* If domain is not NRF24 */
	if (domain != HAL_COMM_PF_NRF24)
		return -EPERM;

	/* If not initialized */
	if (driverIndex == -1)
		return -EPERM;	/* Operation not permitted */

	if (protocol != HAL_COMM_PROTO_RAW)
		return -EINVAL;

	/* Assign pipe0 */
	pipe_ref(pipe);
	sock = peer_alloc(pipe);
	if (!sock)
		return -EBUSY;

	ap.ack = false;
	ap.pipe  = pipe;
	memcpy(ap.aa, aa_pipe0, sizeof(ap.aa));

	phy_ioctl(driverIndex, NRF24_CMD_SET_PIPE, &ap);

	return sock->sockfd;
}

int hal_comm_close(int sockfd)
{
	struct nrf24_raw *peer;
	uint8_t pipe;

	if (driverIndex == -1)
		return -EPERM;

	peer = peer_get(sockfd);
	if (!peer)
		return -ENOTCONN;

	pipe = peer->pipe;

	if (pipe) {
		pipe_unref(pipe);
		phy_ioctl(driverIndex, NRF24_CMD_RESET_PIPE, &pipe);
	}

	peer_unref(peer);

	raw_timeout = new_raw_time();

	return 0;
}

ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{
	struct nrf24_raw *peer;
	size_t length = 0;

	/* Run background procedures */
	running();

	peer = peer_get(sockfd);
	if (!peer)
		return -ENOTCONN;

	if (peer->len_rx != 0) {
		/*
		 * If the amount of bytes available
		 * to be read is greather than count
		 * then read count bytes
		 */
		length = peer->len_rx > count ?
				 count : peer->len_rx;
		/* Copy rx buffer */
		memcpy(buffer, peer->buffer_rx, length);
		/* Reset rx len */
		peer->len_rx = 0;
	} else
		return -EAGAIN;

	/* Returns the amount of bytes read */
	return length;
}


ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{
	struct nrf24_raw *peer;

	/* Run background procedures */
	running();

	if (count > DATA_SIZE)
		return -EINVAL;

	peer = peer_get(sockfd);
	if (!peer)
		return -ENOTCONN;

	/* If already has something to write then returns busy */
	if (peer->len_tx != 0)
		return -EBUSY;

	/* Copy data to be write in tx buffer */
	memcpy(peer->buffer_tx, buffer, count);
	peer->len_tx = count;

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
	const struct nrf24_ll_mgmt_pdu *ipdu;
	const struct nrf24_ll_mgmt_connect *llc;
	struct nrf24_raw *peer, *peer_new;
	struct addr_pipe p_addr;
	int pipe;

	/* Run background procedures */
	running();

	peer = peer_get(sockfd);
	if (!peer)
		return -ENOTCONN;

	if (peer->len_rx == 0)
		return -EAGAIN;

	/* TODO: missing PDU type verification */
	ipdu = (struct nrf24_ll_mgmt_pdu *) peer->buffer_rx;
	llc = (struct nrf24_ll_mgmt_connect *) ipdu->payload;

	/* Mark as read */
	peer->len_rx = 0;

#ifdef ARDUINO
	hal_log_str("Accept()");
#endif

	pipe = pipe_raw_alloc();
	/* If not pipe available */
	if (pipe < 0)
		return pipe; /* Returns too many users */

	/*
	 * Creating a new socket to assign to the nRF24 peer
	 * FIXME: it should inherit sockfd. At the moment
	 * sockfd is not being used.
	 */
	peer_new = peer_alloc(pipe);
	if (!peer_new) {
		pipe_unref(pipe);
		return -EUSERS;
	}

#ifdef ARDUINO
	hal_log_int(peer_new->sockfd);
	hal_log_int(pipe);
#endif

	peer_new->pipe = pipe;

	/* If accept then stop listen */
	listen = 0;

	/* Set aa in pipe */
	p_addr.pipe = pipe;
	memcpy(p_addr.aa, llc->aa, sizeof(p_addr.aa));
	p_addr.ack = true;

	/* Open pipe */
	phy_ioctl(driverIndex, NRF24_CMD_SET_PIPE, &p_addr);

	/* Resize data channel time */
	raw_timeout = new_raw_time();

	/* Disable keep alive request */
	peer_new->keepalive = 0;

	/* Start timeout */
	peer_new->keepalive_anchor = hal_time_ms();

	/* Source address for keepalive message */
	peer_new->mac.address.uint64 = llc->src_addr.address.uint64;

	/* Copy peer address */
	mac->address.uint64 = llc->src_addr.address.uint64;

	/* Return pipe */
	return peer_new->sockfd;
}

int hal_comm_connect(int sockfd, uint64_t *addr)
{
	struct nrf24_ll_mgmt_pdu *opdu;
	struct nrf24_ll_mgmt_connect *payload;
	struct addr_pipe ap;
	struct nrf24_raw *peer;
	struct nrf24_raw *peer_raw = &peers[0];
	size_t len;
	int pipe;

	/* Run background procedures */
	running();

	/* If already has something to write then returns busy */
	if (peer_raw->len_tx != 0)
		return -EINPROGRESS;

	peer = peer_get(sockfd);
	if (!peer)
		return -ENOTCONN;

	memcpy(ap.aa, &mac_local.address.b[3], 4);

	/* Assign a new pipe (not zero) to manage the new peer */
	if (peer->pipe == 0) {
		pipe = pipe_raw_alloc();
		/* If not pipe available */
		if (pipe < 0)
			return pipe;

		peer->pipe = pipe;
		ap.pipe = pipe;
		ap.ack = true;
		ap.aa[0] = pipe;

		phy_ioctl(driverIndex, NRF24_CMD_SET_PIPE, &ap);
	} else {
		ap.aa[0] = peer->pipe;
	}

	/*
	 * New pipe is now allocated. However connect request is
	 * sent over broadcast channel
	 */
	opdu = (struct nrf24_ll_mgmt_pdu *) peer_raw->buffer_tx;
	opdu->type = NRF24_PDU_TYPE_CONNECT_REQ;

	payload = (struct nrf24_ll_mgmt_connect *) opdu->payload;
	payload->src_addr = mac_local;
	payload->dst_addr.address.uint64 = *addr;
	payload->channel = channel_raw;

	memcpy(payload->aa, ap.aa, sizeof(payload->aa));

	/* Source address for keepalive message */
	peer->mac.address.uint64 = *addr;

	len = sizeof(struct nrf24_ll_mgmt_connect);
	len += sizeof(struct nrf24_ll_mgmt_pdu);

	/* Start timeout */
	peer->keepalive_anchor = hal_time_ms();
	/* Enable keep alive: 5 attempts until timeout */
	peer->keepalive = 1;
	peer_raw->len_tx = len;

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
