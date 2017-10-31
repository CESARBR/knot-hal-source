/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <glib.h>
#include <sys/time.h>
#include "hal/nrf24.h"

#include "phy_driver.h"
#include "nrf24l01.h"
#include "nrf24l01_ll.h"
#include "phy_driver_nrf24.h"

static int cli_fd;
static int quit;

static int CHANNEL_MGMT = 76;			/* Beacon/Broadcast channel */
static struct channel channelack;
static struct addr_pipe adrrp;
static char *option_mac = NULL;

/* Access Address for each pipe */
static uint8_t mgmt_aa[] = { 0x8D, 0xD9, 0xBE, 0x96, 0xDE};

static void sig_term(int sig)
{
	quit = 1;
}

static inline void decode_raw(unsigned long sec, unsigned long usec,
					const uint8_t *payload, ssize_t plen)
{
	const struct nrf24_ll_data_pdu *ipdu =
				(const struct nrf24_ll_data_pdu *) payload;
	const struct nrf24_ll_crtl_pdu *ctrl;
	const struct nrf24_ll_keepalive *kpalive;
	char src[32], dst[32];
	int i;

	switch (ipdu->lid) {
	case NRF24_PDU_LID_CONTROL:
		ctrl = (struct nrf24_ll_crtl_pdu *) ipdu->payload;
		kpalive = (struct nrf24_ll_keepalive *) ctrl->payload;

		nrf24_mac2str(&kpalive->src_addr, src);
		nrf24_mac2str(&kpalive->dst_addr, dst);

		if (ctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_REQ) {
			printf("%05ld.%06ld nRF24: CRTL | Keep Alive Req " \
						"(0x%02x) plen:%zd\n", sec,
						usec, ctrl->opcode, plen);
			printf("  %s > %s\n", src, dst);
		} else {
			printf("%05ld.%06ld nRF24: CRTL | Keep Alive Rsp " \
						"(0x%02x) plen:%zd\n", sec,
						usec, ctrl->opcode, plen);
			printf("  %s < %s\n", dst, src);
		}
		break;
	case NRF24_PDU_LID_DATA_FRAG:
	case NRF24_PDU_LID_DATA_END:
		printf("%05ld.%06ld  nRF24: Data | SEQ:0x%02x plen:%zd\n",
					sec, usec, ipdu->nseq, plen);
		printf("  ");
		for (i = 0; i < plen; i++)
			printf("%02x", payload[i]);

		printf("\n");
		break;
	default:
		printf("%05ld.%06ld nRF24: Unknown (0x%02x) plen:%zd\n",
						sec, usec, ipdu->lid, plen);
		printf("  ");
		for (i = 0; i < plen; i++)
			printf("%02x", payload[i]);

		printf("\n");
		break;
	}
}

static inline void decode_mgmt(unsigned long sec, unsigned long usec,
				const uint8_t *payload, ssize_t plen)
{
	struct nrf24_ll_presence *ll;
	struct nrf24_ll_mgmt_connect *llcn;
	struct nrf24_ll_mgmt_pdu *ipdu = (struct nrf24_ll_mgmt_pdu *) payload;
	char src[32], dst[32];
	int i;

	switch (ipdu->type) {
		/* If is a presente type */
	case NRF24_PDU_TYPE_PRESENCE:
		ll = (struct nrf24_ll_presence*) ipdu->payload;

		nrf24_mac2str(&ll->mac, src);

		if (option_mac && strcmp(option_mac, src) != 0)
			break;

		printf("%05ld.%06ld nRF24: Beacon(0x%02x|P) plen:%zd\n",
		       sec, usec, ipdu->type, plen);
		printf("  %s %s\n", src, ll->name);
		break;
		/* If is a connect request type */
	case NRF24_PDU_TYPE_CONNECT_REQ:
		/* Link layer connect structure */
		llcn = (struct nrf24_ll_mgmt_connect *) ipdu->payload;

		nrf24_mac2str(&llcn->src_addr, src);
		nrf24_mac2str(&llcn->dst_addr, dst);

		/* Header type is a connect request type */
		printf("%05ld.%06ld nRF24: Connect Req(0x%02x) plen:%zd\n",
					       sec, usec, ipdu->type, plen);

		printf("  %s > %s\n", src, dst);
		printf("  CH: %d AA: %02x%02x%02x%02x%02x\n",
		       llcn->channel,
		       llcn->aa[0],
		       llcn->aa[1],
		       llcn->aa[2],
		       llcn->aa[3],
		       llcn->aa[4]);

		if (g_strcmp0(option_mac, dst) != 0)
			break;

		/* Now track connected device ONLY */
		channelack.value = llcn->channel;
		phy_ioctl(cli_fd, NRF24_CMD_SET_CHANNEL, &channelack);
		adrrp.pipe = 1;
		memcpy(adrrp.aa, llcn->aa, sizeof(adrrp.aa));
		phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);

		break;
	default:
		printf("%05ld.%06ld nRF24: Unknown (0x%02x) plen:%zd\n",
						sec, usec, ipdu->type, plen);
		printf("  ");
		for (i = 0; i < plen; i++)
			printf("%02x", payload[i]);
		printf("\n");
		break;
	}
}

static int sniffer_start(void)
{
	unsigned long sec, usec;
	struct timeval tm, reftm;
	struct nrf24_io_pack p;
	ssize_t plen;
	time_t last_sec = LONG_MAX;

	cli_fd = phy_open("NRF0");
	if (cli_fd < 0)
		return -EIO;

	/* Sniffer in broadcast channel*/
	channelack.value = CHANNEL_MGMT;
	channelack.ack = false;
	phy_ioctl(cli_fd, NRF24_CMD_SET_CHANNEL, &channelack);
	adrrp.pipe = 0;
	memcpy(adrrp.aa, mgmt_aa, sizeof(adrrp.aa));
	phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);

	/* Reference time */
	gettimeofday(&reftm, NULL);

	while (!quit) {

		if (channelack.value == CHANNEL_MGMT)
			p.pipe = 0;
		else
			p.pipe = 1;

		gettimeofday(&tm, NULL);

		/* Probably disconnected: return no broadcast channel */
		if ((tm.tv_sec - last_sec) > 5) {
			phy_ioctl(cli_fd, NRF24_CMD_SET_CHANNEL, &channelack);
			phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);
			last_sec = tm.tv_sec;
		}

		plen = phy_read(cli_fd, &p, NRF24_MTU);
		if (plen <= 0)
			continue;

		last_sec = tm.tv_sec;
		if (tm.tv_usec < reftm.tv_usec) {
			sec = tm.tv_sec - reftm.tv_sec - 1;
			usec = tm.tv_usec + 1000000 - reftm.tv_usec;
		} else {
			sec = tm.tv_sec - reftm.tv_sec;
			usec = tm.tv_usec - reftm.tv_usec;
		}

		if (channelack.value == CHANNEL_MGMT)
			decode_mgmt(sec, usec, p.payload, plen);
		else
			decode_raw(sec, usec, p.payload, plen);
	}

	return 0;
}

static void sniffer_stop(void)
{
	if (cli_fd >= 0)
		phy_close(cli_fd);
}

static GOptionEntry options[] = {
	{ "mac", 'm', 0, G_OPTION_ARG_STRING, &option_mac,
                               "Specify MAC to filter", NULL},
	{ NULL },
};

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	int err;

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	g_option_context_free(context);

	printf("nRF24 Sniffer\n");

	err = sniffer_start();
	if (err < 0)
	       return err;

	sniffer_stop();

	return 0;
}
