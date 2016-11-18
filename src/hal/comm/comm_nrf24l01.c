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
#include "phy_driver.h"
#include "phy_driver_nrf24.h"
#include "nrf24l01_ll.h"

#define DATA_SIZE 128

/* TODO: Get this values from config file */
static const struct nrf24_mac addr_gw = {
					.address.uint64 = 0xDEADBEEF12345678};

/* Structure to save broadcast context */
struct nrf24_mgmt {
	uint8_t buffer_rx[DATA_SIZE];
	size_t len_rx;
	uint8_t buffer_tx[DATA_SIZE];
	size_t len_tx;
};

static struct nrf24_mgmt mgmt = {.len_rx = 0};

/* Structure to save peers context */
struct nrf24_data {
	int8_t pipe;
	uint8_t buffer_rx[DATA_SIZE];
	size_t len_rx;
	uint8_t buffer_tx[DATA_SIZE];
	size_t len_tx;
};

#ifndef ARDUINO	/* If gateway then 5 peers */
static struct nrf24_data peers[5] = {
	{.pipe = -1, .len_rx = 0},
	{.pipe = -1, .len_rx = 0},
	{.pipe = -1, .len_rx = 0},
	{.pipe = -1, .len_rx = 0},
	{.pipe = -1, .len_rx = 0}
};
#else	/* If thing then 1 peer */
static struct nrf24_data peers[1] = {
	{.pipe = -1, .len_rx = 0},
};
#endif

/* ARRAY SIZE */
#define CONNECTION_COUNTER	((int) (sizeof(peers) \
				 / sizeof(peers[0])))

/* Access Address for each pipe */
static uint8_t aa_pipes[6][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x35, 0x96, 0xB6, 0xC1, 0x6B},
	{0x77, 0x96, 0xB6, 0xC1, 0x6B},
	{0xD3, 0x96, 0xB6, 0xC1, 0x6B},
	{0xE7, 0x96, 0xB6, 0xC1, 0x6B},
	{0xF0, 0x96, 0xB6, 0xC1, 0x6B}
};

/* Global to save driver index */
static int driverIndex = -1;

static int channel_mgmt = 20;
static int channel_raw = 10;
enum {
	START_MGMT,
	MGMT,
	START_RAW,
	RAW
};

/* Local functions */
static inline int alloc_pipe(void)
{
	uint8_t i;

	for (i = 0; i < CONNECTION_COUNTER; i++) {
		if (peers[i].pipe == -1) {

			peers[i].len_rx = 0;
			/* one peer for pipe*/
			peers[i].pipe = i+1;
			return peers[i].pipe;
		}
	}

	/* No free pipe */
	return -1;
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

static void running(void)
{

	static int state = 0;
	static int sockIndex = 0; /* Index peers */

	switch (state) {

	case START_MGMT:
		/* Set channel to management channel */
		phy_ioctl(driverIndex, NRF24_CMD_SET_CHANNEL, &channel_mgmt);

		/* TODO: Start timeout */

		/* Go to next state */
		state = MGMT;
		break;

	case MGMT:
		/* TODO: Check if 10ms timeout occurred */
		/* If timeout occurred then go to next state */
			state = START_RAW;

		/* TODO: Send presence packets */
		/* TODO: Read management packets until timeout occurs */
		write_mgmt(driverIndex);
		break;

	case START_RAW:
		/* Set channel to data channel */
		phy_ioctl(driverIndex, NRF24_CMD_SET_CHANNEL, &channel_raw);

		/* TODO: Start timeout */

		/* Go to next state */
		state = RAW;
		break;

	case RAW:
		/* TODO: Check if 60ms timeout occurred */
		/* If timeout occurred then go to next state */
			state = START_MGMT;

		if (sockIndex == CONNECTION_COUNTER)
			sockIndex = 0;

		/* Check if pipe is allocated */
		if (peers[sockIndex].pipe != -1) {
			/* TODO: call read/write raw data */
		}


		sockIndex++;
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
		ap.ack = false;
		retval = 0;
		break;

	case HAL_COMM_PROTO_RAW:
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

	return -ENOSYS;
}

int hal_comm_accept(int sockfd, uint64_t *addr)
{


	uint8_t datagram[NRF24_MTU];
	struct nrf24_ll_mgmt_pdu *ipdu = (struct nrf24_ll_mgmt_pdu *) datagram;
	struct nrf24_ll_mgmt_connect *payload =
			(struct nrf24_ll_mgmt_connect *) ipdu->payload;
	size_t len;

	/* Run background procedures */
	running();

	/* Read connect_request from pipe broadcast */
	len = mgmt.len_rx;
	if (len == 0)
		return -EAGAIN;

	/* Get the response */
	memcpy(datagram, mgmt.buffer_rx, len);
	/* Reset rx len */
	mgmt.len_rx = 0;
	/* If this packet is not connect request */
	if (ipdu->type != NRF24_PDU_TYPE_CONNECT_REQ)
		return -EINVAL;
	/* If this packet is not for me*/
	if (payload->dst_addr.address.uint64 != *addr)
		return -EINVAL;

	return 0;
}


int hal_comm_connect(int sockfd, uint64_t *addr)
{

	uint8_t datagram[NRF24_MTU];
	struct nrf24_ll_mgmt_pdu *opdu = (struct nrf24_ll_mgmt_pdu *)datagram;
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

	len = sizeof(struct nrf24_ll_mgmt_connect);
	len += sizeof(struct nrf24_ll_mgmt_pdu);

	/* Copy data to be write in tx buffer for BROADCAST*/
	memcpy(mgmt.buffer_tx, datagram, len);
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
