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

#ifdef ARDUINO
#include <avr_errno.h>
#include <avr_unistd.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "include/time.h"
#include "comm_private.h"
#include "nrf24l01.h"
#include "nrf24l01_ll.h"

/* Access Address for each pipe */
static uint8_t aa_pipes[6][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x6B, 0x96, 0xB6, 0xC1, 0x35},
	{0x6B, 0x96, 0xB6, 0xC1, 0x77},
	{0x6B, 0x96, 0xB6, 0xC1, 0xD3},
	{0x6B, 0x96, 0xB6, 0xC1, 0xE7},
	{0x6B, 0x96, 0xB6, 0xC1, 0xF0}
};

int8_t pipes_allocate[] = {0, 0, 0, 0, 0, 0};

#define NRF24_PIPE0		0

/* TODO: Get this values from config file */
static const uint64_t addr_thing = 0xACDCDEAD98765432;
static const uint64_t addr_gw = 0xDEADBEEF12345678;
static const uint8_t channel_data = 20;

static int nrf24l01_probe(void)
{
	return nrf24l01_init("/dev/spidev0.0");
}

static void nrf24l01_remove(void)
{

}

static ssize_t send_data(int sockfd, const void *buffer, size_t len)
{
	int err;
	/* Puts the radio in TX mode  enabling Acknowledgment */
	nrf24l01_set_ptx(aa_pipes[sockfd], true);

	/* Transmits the data */
	nrf24l01_ptx_data((void *)buffer, len);

	/* Waits for ACK */
	err = nrf24l01_ptx_wait_datasent();

	if (err < 0)
		return err;
	/*
	 * The radio do not receive and send at the same time
	 * It's a good practice to put the radio in RX mode
	 * and only switch to TX mode when transmitting data.
	 */

	nrf24l01_set_prx();

	/*
	 * On success, the number of bytes written is returned
	 * Otherwise, -1 is returned.
	 */

	return len;
}

static ssize_t read_data(int sockfd, void *buffer, size_t len)
{
	ssize_t length = -1;

	/* If the pipe available */
	if (nrf24l01_prx_pipe_available() == sockfd)
		/* Copy data to buffer */
		length = nrf24l01_prx_data(buffer, len);

	/*
	 * On success, the number of bytes read is returned
	 * Otherwise, -1 is returned.
	 */

	return length;
}

static int nrf24l01_open(const char *pathname)
{
	/*
	 * Considering 16-bits to address adapter and logical
	 * channels. The most significant 4-bits will be used to
	 * address the local adapter, the remaining (12-bits) will
	 * used to address logical channels(clients/pipes).
	 * eg: For nRF24 '0' will be mapped to pipe0.
	 * TODO: Implement addressing
	 */

	/*
	 * Returns an identification for the available pipe.
	 * Index '0' is reserved for pipe zero. Assigning '1'
	 * means that the pipe 'i' is now allocated (busy).
	 */
	int i;

	for (i = 1; i < sizeof(pipes_allocate); i++) {
		if (pipes_allocate[i] == 0) {
			/* one peer for pipe*/
			pipes_allocate[i] = 1;
			return i;
		}
	}
	return 0;
}

static ssize_t nrf24l01_recv(int sockfd, void *buffer, size_t len)
{
	ssize_t ilen;
	size_t plen;
	uint8_t lid, datagram[NRF24_MTU];
	const struct nrf24_ll_data_pdu *ipdu = (void *)datagram;
	static int offset = 0;
	static uint8_t seqnumber = 0;

	if (len < 0)
		return -EINVAL;

	do {

		/*
		 * Reads the data,
		 * on success, the number of bytes read is returned
		 */
		ilen = read_data(sockfd, datagram, NRF24_MTU);

		if (ilen < 0)
			return -EAGAIN;	/* Try again */

		if (seqnumber != ipdu->nseq)
			return -EILSEQ; /* Illegal byte sequence */

		if (seqnumber == 0)
			offset = 0;

		/* Payloag length = input length - header size */
		plen = ilen - DATA_HDR_SIZE;
		lid = ipdu->lid;

		if (lid == NRF24_PDU_LID_DATA_FRAG && plen < NRF24_PW_MSG_SIZE)
			return -EBADMSG; /* Not a data message */

		/* Reads no more than len bytes */
		if (offset + plen > len)
			plen = len - offset;

		memcpy(buffer + offset, ipdu->payload, plen);
		offset += plen;
		seqnumber++;

	} while (lid != NRF24_PDU_LID_DATA_END && offset < len);

	/* If the complete msg is received, resets the sequence number */
	seqnumber = 0;

	/* Returns de number of bytes received */
	return offset;

}

static ssize_t nrf24l01_send(int sockfd, const void *buffer, size_t len)
{
	int err;
	uint8_t seqnumber, datagram[NRF24_MTU];
	struct nrf24_ll_data_pdu *opdu = (void *)datagram;
	size_t plen, left;

	/* If len is larger than the maximum message size */
	if (len > NRF24_MAX_MSG_SIZE)
		return -EINVAL;	/* FIX: erro */

	left = len;
	seqnumber = 0;	/* Packet sequence number */

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
		opdu->nseq = seqnumber;

		/* Offset = len - left */
		memcpy(opdu->payload, buffer + (len - left), plen);

		/* Send packet */
		err = send_data(sockfd, datagram, plen + DATA_HDR_SIZE);
		if (err < 0)
			return err;

		left -= plen;
		seqnumber++;

	}

	return len;
}


static int nrf24l01_listen(int sockfd)
{
	if (sockfd < 0 || sockfd > NRF24_PIPE_ADDR_MAX)
		return -EINVAL;

	/* Set channel */
	if (nrf24l01_set_channel(NRF24_CHANNEL_DEFAULT) == -1)
		return -EINVAL;

	/* Open pipe zero in the sockfd address */
	nrf24l01_open_pipe(NRF24_PIPE0, aa_pipes[NRF24_PIPE0]);

	/*
	 * Standby mode is used to minimize average
	 * current consumption while maintaining short
	 * start up times. In this mode only
	 * part of the crystal oscillator is active.
	 * FIFO state: No ongoing packet transmission.
	 */
	nrf24l01_set_standby();

	/* Put the radio in RX mode to start receiving packets.*/
	nrf24l01_set_prx();

	return 0;
}

static int nrf24l01_connect(int cli_sockfd, uint8_t to_addr)
{
	uint8_t datagram[NRF24_MTU];
	struct nrf24_ll_mgmt_pdu *opdu = (struct nrf24_ll_mgmt_pdu *)datagram;
	struct nrf24_ll_mgmt_connect *payload =
				(struct nrf24_ll_mgmt_connect *) opdu->payload;
	size_t len, offset;
	int err;

	opdu->type = NRF24_PDU_TYPE_CONNECT_REQ;

	payload->src_addr = addr_gw;
	payload->dst_addr = addr_thing;
	payload->channel = channel_data;
	/*
	 * Set in payload the addr to be set in client.
	 * cli_sockfd contains the pipe allocated for the client
	 * aa_pipes contains the Access Address for each pipe
	 */
	memcpy(payload->aa, aa_pipes[cli_sockfd],
		sizeof(aa_pipes[cli_sockfd]));

	len = sizeof(struct nrf24_ll_mgmt_connect);
	len += sizeof(struct nrf24_ll_mgmt_pdu);

	/*Send connect_request in broadcast */
	err = send_data(PIPE_BROADCAST, datagram, len);
	if (err < 0)
		return -1;

	return cli_sockfd;
}

struct phy_driver nrf24l01 = {
	.name = "nRF24L01",
	.probe = nrf24l01_probe,
	.remove = nrf24l01_remove,
	.open = nrf24l01_open,
	.recv = nrf24l01_recv,
	.send = nrf24l01_send,
	.listen = nrf24l01_listen,
	.connect = nrf24l01_connect
};
