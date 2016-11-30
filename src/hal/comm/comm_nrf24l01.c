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
#include "include/avr_errno.h"
#include "include/avr_unistd.h"
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "include/comm.h"
#include "include/nrf24.h"
#include "include/time.h"
#include "phy_driver.h"
#include "phy_driver_nrf24.h"
#include "nrf24l01_ll.h"

#define _MIN(a, b)		((a) < (b) ? (a) : (b))
#define DATA_SIZE 128

/* Global to know if listen function was called */
static uint8_t listen = 0;
/* Global to know to number of live connections */
static uint8_t connection_live = 0;

/* TODO: Get this values from config file */
static const struct nrf24_mac addr_gw = {
					.address.uint64 = 0xDEADBEEF12345678};

static struct nrf24_mac addr_thing = {.address.uint64 = 0 };

/* Structure to save broadcast context */
struct nrf24_mgmt {
	int8_t pipe;
	uint8_t buffer_rx[DATA_SIZE];
	size_t len_rx;
	uint8_t buffer_tx[DATA_SIZE];
	size_t len_tx;
};

static struct nrf24_mgmt mgmt = {.pipe = -1, .len_rx = 0};

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
	unsigned long keepalive_wait;
	uint8_t keepalive;
	struct nrf24_mac mac;
};

#ifndef ARDUINO	/* If gateway then 5 peers */
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

/* TODO: TODO: Get this values from config file
 * Access Address for each pipe
 */
static uint8_t aa_pipes[6][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x35, 0x96, 0xB6, 0xC1, 0x6B},
	{0x77, 0x96, 0xB6, 0xC1, 0x6B},
	{0xD3, 0x96, 0xB6, 0xC1, 0x6B},
	{0xE7, 0x96, 0xB6, 0xC1, 0x6B},
	{0xF0, 0x96, 0xB6, 0xC1, 0x6B}
};

#else	/* If thing then 1 peer */
static struct nrf24_data peers[1] = {
	{.pipe = -1, .len_rx = 0, .seqnumber_tx = 0,
		.seqnumber_rx = 0, .offset_rx = 0},
};

/* TODO: TODO: Get this value from config file
 * Access Address for pipe 0
 * AA for pipe 1 are in connect req pkt
 */
static uint8_t aa_pipes[1][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
};

#endif

/* ARRAY SIZE */
#define CONNECTION_COUNTER	((int) (sizeof(peers) \
				 / sizeof(peers[0])))

/* Global to save driver index */
static int driverIndex = -1;
/* Channel to management and raw data */
static int channel_mgmt = 20;
static int channel_raw = 10;
static uint16_t window_bcast = 1;	/* ms */
static uint16_t interval_bcast = 2;	/* ms */

enum {
	START_MGMT,
	MGMT,
	START_RAW,
	RAW
};

enum {
	PRESENCE,
	TIMEOUT_WINDOW,
	STANDBY,
	TIMEOUT_INTERVAL
};

/* Local functions */
static inline int alloc_pipe(void)
{
	uint8_t i;

	for (i = 0; i < CONNECTION_COUNTER; i++) {
		if (peers[i].pipe == -1) {

			peers[i].keepalive_wait = 0;
			peers[i].keepalive = 0;
			peers[i].mac.address.uint64 = 0;
			peers[i].len_rx = 0;
			peers[i].seqnumber_rx = 0;
			peers[i].seqnumber_tx = 0;
			peers[i].offset_rx = 0;
			/* one peer for pipe*/
			peers[i].pipe = i+1;
			return peers[i].pipe;
		}
	}

	/* No free pipe */
	return -1;
}

static int write_keepalive(int spi_fd, int sockfd, int keepalive_op,
				struct nrf24_mac dst, struct nrf24_mac src)
{
	int err;
	/* Assemble keep alive package */
	struct nrf24_io_pack p;
	struct nrf24_ll_data_pdu *opdu =
		(struct nrf24_ll_data_pdu *)p.payload;
	struct nrf24_ll_crtl_pdu *ctrl =
		(struct nrf24_ll_crtl_pdu *)opdu->payload;

	struct nrf24_ll_keepalive *kpalive =
		(struct nrf24_ll_keepalive *) ctrl->payload;

	opdu->lid = NRF24_PDU_LID_CONTROL;
	p.pipe = sockfd;
	/* Keep alive opcode - Request or Response */
	ctrl->opcode = keepalive_op;
	/* src and dst address to keepalive */
	kpalive->dst_addr.address.uint64 = dst.address.uint64;
	kpalive->src_addr.address.uint64 = src.address.uint64;
	/* Sends keep alive packet */
	err = phy_write(spi_fd, &p,
					sizeof(struct nrf24_ll_data_pdu) +
					sizeof(struct nrf24_ll_crtl_pdu) +
					sizeof(struct nrf24_ll_keepalive));

	if (err < 0)
		return err;

	return 0;
}

static int check_keepalive(int spi_fd, int sockfd)
{

	int err = 0;

	/* Check if timeout occurred */
	if (hal_timeout(hal_time_ms(), peers[sockfd-1].keepalive_wait,
		NRF24_KEEPALIVE_TIMEOUT_MS) > 0)
		return -ETIMEDOUT;

	/* If keepalive is disable */
	if (peers[sockfd-1].keepalive == 0)
		return err;

	/* Sends keepalive request every NRF24_KEEPALIVE_SEND_MS */
	if (hal_timeout(hal_time_ms(), peers[sockfd-1].keepalive_wait,
		peers[sockfd-1].keepalive * NRF24_KEEPALIVE_SEND_MS) > 0) {
		/* Sends keepalive packet */
		err = write_keepalive(spi_fd, sockfd,
				NRF24_LL_CRTL_OP_KEEPALIVE_REQ,
				peers[sockfd-1].mac, addr_thing);

		peers[sockfd-1].keepalive += 1;
	}


	return err;
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
	ssize_t ilen;
	struct nrf24_io_pack p;
	struct nrf24_ll_mgmt_pdu *ipdu = (struct nrf24_ll_mgmt_pdu *)p.payload;

	/* Read from management pipe */
	p.pipe = 0;

	/* Read data */
	ilen = phy_read(spi_fd, &p, NRF24_MTU);
	if (ilen < 0)
		return -EAGAIN;

	/* If already has something in rx buffer then return BUSY*/
	if (mgmt.len_rx != 0)
		return -EBUSY;

	switch (ipdu->type) {
	/* If is a presente type */
	case NRF24_PDU_TYPE_PRESENCE:
	{
		/* Event header structure */
		struct mgmt_nrf24_header *evt =
			(struct mgmt_nrf24_header *) mgmt.buffer_rx;
		/* Event presence structure */
		struct mgmt_evt_nrf24_bcast_presence *evt_presence =
			(struct mgmt_evt_nrf24_bcast_presence *)evt->payload;
		/* Mac address structure */
		struct nrf24_mac *mac = (struct nrf24_mac *)ipdu->payload;

		/* Header type is a broadcast presence */
		evt->opcode = MGMT_EVT_NRF24_BCAST_PRESENCE;
		evt->index = 0;
		/* Copy source address */
		evt_presence->mac.address.uint64 = mac->address.uint64;

		mgmt.len_rx = sizeof(struct nrf24_mac) +
				sizeof(struct mgmt_nrf24_header);
	}
		break;
	/* If is a connect request type */
	case NRF24_PDU_TYPE_CONNECT_REQ:
	{
		/* Event header structure */
		struct mgmt_nrf24_header *evt =
			(struct mgmt_nrf24_header *) mgmt.buffer_rx;
		/* Event connect structure */
		struct mgmt_evt_nrf24_connected *evt_connect =
			(struct mgmt_evt_nrf24_connected *)evt->payload;
		/* Link layer connect structure */
		struct nrf24_ll_mgmt_connect *connect =
				(struct nrf24_ll_mgmt_connect *) ipdu->payload;

		/* Header type is a connect request type */
		evt->opcode = MGMT_EVT_NRF24_CONNECTED;
		evt->index = 0;
		/* Copy src and dst address*/
		evt_connect->src.address.uint64 =
			connect->src_addr.address.uint64;
		evt_connect->dst.address.uint64 =
			connect->dst_addr.address.uint64;
		/* Copy channel */
		evt_connect->channel = connect->channel;
		/* Copy access address */
		memcpy(evt_connect->aa, connect->aa, sizeof(aa_pipes[0]));

		mgmt.len_rx = sizeof(struct mgmt_nrf24_header) +
				sizeof(struct mgmt_evt_nrf24_connected);

	}
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
	if (peers[sockfd-1].len_tx > NRF24_MAX_MSG_SIZE)
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

	/* Restart keepalive timeout */
	peers[sockfd-1].keepalive_wait = hal_time_ms();
	if (peers[sockfd-1].keepalive >= 1)
		peers[sockfd-1].keepalive = 1;

	err = peers[sockfd-1].len_tx;

	/* Resets controls */
	peers[sockfd-1].len_tx = 0;
	peers[sockfd-1].seqnumber_tx = 0;

	return err;
}

static int read_raw(int spi_fd, int sockfd)
{
	ssize_t ilen;
	size_t plen;
	struct nrf24_io_pack p;
	const struct nrf24_ll_data_pdu *ipdu = (void *)p.payload;

	p.pipe = sockfd;
	/*
	 * Reads the data while to exist,
	 * on success, the number of bytes read is returned
	 */
	while ((ilen = phy_read(spi_fd, &p, NRF24_MTU)) > 0) {
		/* Check if is data or Control */
		switch (ipdu->lid) {

		/* If is Control */
		case NRF24_PDU_LID_CONTROL:
		{
			struct nrf24_ll_crtl_pdu *ctrl =
				(struct nrf24_ll_crtl_pdu *)ipdu->payload;

			struct nrf24_ll_keepalive *kpalive =
				(struct nrf24_ll_keepalive *) ctrl->payload;
			/*
			 * If is keep alive then resets keepalive_wait
			 * Thing side
			 */
			if (ctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_RSP &&
				kpalive->src_addr.address.uint64 ==
				peers[sockfd-1].mac.address.uint64 &&
				kpalive->dst_addr.address.uint64 ==
				addr_thing.address.uint64) {
				peers[sockfd-1].keepalive_wait = hal_time_ms();
				peers[sockfd-1].keepalive = 1;
			}

			/*
			 * If is keep alive then resets keepalive_wait
			 * NRFD side
			 */

			if (ctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_REQ &&
				kpalive->src_addr.address.uint64 ==
				peers[sockfd-1].mac.address.uint64 &&
				kpalive->dst_addr.address.uint64 ==
				addr_gw.address.uint64) {
				peers[sockfd-1].keepalive_wait = hal_time_ms();
				write_keepalive(spi_fd, sockfd,
					NRF24_LL_CRTL_OP_KEEPALIVE_RSP,
					peers[sockfd-1].mac,
					addr_gw);
			}

		}
			break;

		/* If is Data */
		case NRF24_PDU_LID_DATA_FRAG:
		case NRF24_PDU_LID_DATA_END:
			/* Restart keepalive timeout */
			peers[sockfd-1].keepalive_wait = hal_time_ms();
			if (peers[sockfd-1].keepalive >= 1)
				peers[sockfd-1].keepalive = 1;

			if (peers[sockfd-1].len_rx != 0)
				break; /* Discard packet */

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
	struct nrf24_mac *payload =
				(struct nrf24_mac *) opdu->payload;
	size_t len;
	static unsigned long start;
	/* Start timeout */
	static uint8_t state = 0;

	switch (state) {
	case PRESENCE:
		/* Send Presence */
		if (addr_thing.address.uint64 == 0)
			break;

		p.pipe = 0;
		opdu->type = NRF24_PDU_TYPE_PRESENCE;
		payload->address.uint64 = addr_thing.address.uint64;
		len = sizeof(struct nrf24_ll_mgmt_pdu)+sizeof(struct nrf24_mac);
		phy_write(spi_fd, &p, len);
		/* Init time */
		start = hal_time_ms();
		state = TIMEOUT_WINDOW;
		break;
	case TIMEOUT_WINDOW:
		if (hal_timeout(hal_time_ms(), start, window_bcast) > 0)
			state = STANDBY;

		break;
	case STANDBY:
		phy_ioctl(spi_fd, NRF24_CMD_SET_STANDBY, NULL);
		state = TIMEOUT_INTERVAL;
		break;
	case TIMEOUT_INTERVAL:
		if (hal_timeout(hal_time_ms(), start, interval_bcast) > 0)
			state = PRESENCE;

		break;
	}
}

static void running(void)
{

	static int state = 0;
	/* Index peers */
	static int sockIndex = 1;
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
		/* Check if 10ms timeout occurred */
		if (hal_timeout(hal_time_ms(), start, 10) > 0)
			if (connection_live > 0)
				state = START_RAW;

		if (listen)
			presence_connect(driverIndex);

		read_mgmt(driverIndex);
		write_mgmt(driverIndex);
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
		/* Check if 60ms timeout occurred */
		if (hal_timeout(hal_time_ms(), start, 60) > 0)
			state = START_MGMT;

		/* Check if pipe is allocated */
		if (peers[sockIndex-1].pipe != -1) {
			read_raw(driverIndex, sockIndex);
			write_raw(driverIndex, sockIndex);

			/*
			 * If keepalive is enabled
			 * Check if timeout occurred and generates
			 * disconnect event
			 */

			if (check_keepalive(driverIndex, sockIndex) == -ETIMEDOUT &&
				mgmt.len_rx == 0) {

				struct mgmt_nrf24_header *evt =
					(struct mgmt_nrf24_header *) mgmt.buffer_rx;

				struct mgmt_evt_nrf24_disconnected *evt_discon =
					(struct mgmt_evt_nrf24_disconnected *)evt->payload;

				evt->opcode = MGMT_EVT_NRF24_DISCONNECTED;

				evt_discon->mac.address.uint64 =
					peers[sockIndex-1].mac.address.uint64;
				mgmt.len_rx =
					sizeof(struct mgmt_nrf24_header) +
					sizeof(struct mgmt_evt_nrf24_disconnected);

				peers[sockIndex-1].keepalive_wait
					= hal_time_ms();

				/* TODO: Send disconnect packet to thing */
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
int hal_comm_init(const char *pathname)
{
	/* If driver not opened */
	if (driverIndex != -1)
		return -EPERM;

	/* Open driver and returns the driver index */
	driverIndex = phy_open(pathname);
	if (driverIndex < 0)
		return driverIndex;

	return 0;
}

int hal_comm_deinit(void)
{
	int err;

	/* If try to close driver with no driver open */
	if (driverIndex == -1)
		return -EPERM;

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
		ap.ack = false;
		retval = 0;
		mgmt.pipe = 0;
		break;

	case HAL_COMM_PROTO_RAW:
		if (mgmt.pipe == -1) {
			/* If Management is not open*/
			ap.ack = false;
			mgmt.pipe = 0;
			retval = 0;
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

		ap.ack = true;
		break;

	default:
		return -EINVAL; /* Invalid argument */
	}

	ap.pipe = retval;
	memcpy(ap.aa, aa_pipes[retval], sizeof(aa_pipes[retval]));

	/* Open pipe */
	phy_ioctl(driverIndex, NRF24_CMD_SET_PIPE, &ap);

	return retval;
}

int hal_comm_close(int sockfd)
{
	if (driverIndex == -1)
		return -EPERM;

	/* Pipe 0 is not closed because ACK arrives in this pipe */
	if (sockfd >= 1 && sockfd <= 5) {
		/* Free pipe */
		peers[sockfd-1].pipe = -1;
		phy_ioctl(driverIndex, NRF24_CMD_RESET_PIPE, &sockfd);
		connection_live--;
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

	return 0;
}

int hal_comm_accept(int sockfd, uint64_t *addr)
{

	/* TODO: Run background procedures */
	struct mgmt_nrf24_header *evt =
				(struct mgmt_nrf24_header *) mgmt.buffer_rx;
	struct mgmt_evt_nrf24_connected *evt_connect =
			(struct mgmt_evt_nrf24_connected *)evt->payload;
	struct addr_pipe p_addr;
	int pipe;
	/* Run background procedures */
	running();

	/* Save thing address */
	addr_thing.address.uint64 = *addr;

	if (mgmt.len_rx == 0)
		return -EAGAIN;

	/* Free management read to receive new packet */
	mgmt.len_rx = 0;

	if (evt->opcode != MGMT_EVT_NRF24_CONNECTED ||
		evt_connect->dst.address.uint64 != *addr)
		return -EAGAIN;

	pipe = alloc_pipe();
	/* If not pipe available */
	if (pipe < 0)
		return -EUSERS; /* Returns too many users */

	/* If accept then stop listen */
	listen = 0;

	/* Set aa in pipe */
	p_addr.pipe = pipe;
	memcpy(p_addr.aa, evt_connect->aa, sizeof(evt_connect->aa));
	p_addr.ack = 1;
	/*open pipe*/
	phy_ioctl(driverIndex, NRF24_CMD_SET_PIPE, &p_addr);

	/* If accept then increment connection_live */
	connection_live++;
	/* Source address for keepalive message */
	peers[pipe-1].mac.address.uint64 =
		evt_connect->src.address.uint64;
	/* Enable peer to send keep alive request */
	peers[pipe-1].keepalive = 1;
	/* Start timeout */
	peers[pipe-1].keepalive_wait = hal_time_ms();

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

	payload->src_addr = addr_gw;
	payload->dst_addr.address.uint64 = *addr;
	payload->channel = channel_raw;
	/*
	 * Set in payload the addr to be set in client.
	 * sockfd contains the pipe allocated for the client
	 * aa_pipes contains the Access Address for each pipe
	 */
	memcpy(payload->aa, aa_pipes[sockfd],
		sizeof(aa_pipes[sockfd]));

	/* Source address for keepalive message */
	peers[sockfd-1].mac.address.uint64 = *addr;

	len = sizeof(struct nrf24_ll_mgmt_connect);
	len += sizeof(struct nrf24_ll_mgmt_pdu);

	/* If connect then increment connection_live */
	connection_live++;
	/* Start timeout */
	peers[sockfd-1].keepalive_wait = hal_time_ms();
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

	return (rc != 8 ? -1 : 0);
}
