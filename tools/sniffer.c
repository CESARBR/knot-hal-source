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

static char *opt_mode = "mgmt";

int cli_fd;
int quit;

static int channel_mgmt = 76;
static int channel_raw = 10;

/* Access Address for each pipe */
static uint8_t aa_pipes[2][5] = {
	{0x8D, 0xD9, 0xBE, 0x96, 0xDE},
	{0x35, 0x96, 0xB6, 0xC1, 0x6B},
};

static void sig_term(int sig)
{
	quit = 1;
	phy_close(cli_fd);
}


static void listen_raw(void)
{
	struct nrf24_io_pack p;
	ssize_t ilen;
	const struct nrf24_ll_data_pdu *ipdu = (void *)p.payload;

	p.pipe = 1;
	while (!quit) {
		ilen = phy_read(cli_fd, &p, NRF24_MTU);
		if (ilen < 0)
			continue;

		switch (ipdu->lid) {

		/* If is Control */
		case NRF24_PDU_LID_CONTROL:
		{
			struct nrf24_ll_crtl_pdu *ctrl =
				(struct nrf24_ll_crtl_pdu *) ipdu->payload;

			struct nrf24_ll_keepalive *kpalive =
				(struct nrf24_ll_keepalive *) ctrl->payload;

			printf("NRF24_PDU_LID_CONTROL\n");
			if (ctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_RSP)
				printf("NRF24_LL_CRTL_OP_KEEPALIVE_RSP\n");

			if (ctrl->opcode == NRF24_LL_CRTL_OP_KEEPALIVE_REQ)
				printf("NRF24_LL_CRTL_OP_KEEPALIVE_REQ\n");

			printf("src_addr : %llX\n",
			(long long int) kpalive->src_addr.address.uint64);
			printf("dst_addr : %llX\n",
			(long long int)kpalive->dst_addr.address.uint64);

		}
			break;

		/* If is Data */
		case NRF24_PDU_LID_DATA_FRAG:
		{
			printf("NRF24_PDU_LID_DATA_FRAG\n");
			printf("nseq : %d\n", ipdu->nseq);
			printf("payload: %s\n", ipdu->payload);
		}
			break;
		case NRF24_PDU_LID_DATA_END:
		{
			printf("NRF24_PDU_LID_DATA_END\n");
			printf("nseq : %d\n", ipdu->nseq);
			printf("payload: %s\n", ipdu->payload);
		}
			break;
		default:
			printf("CODE INVALID %d\n", ipdu->lid);
		}
		printf("\n\n");
	}
}

static void listen_mgmt(void)
{
	struct nrf24_io_pack p;
	struct nrf24_ll_presence *ll;
	struct nrf24_ll_mgmt_connect *llcn;
	struct nrf24_ll_mgmt_pdu *ipdu =
				(struct nrf24_ll_mgmt_pdu *)p.payload;
	const char *raw = (const char *) ipdu->payload;
	char src[32], dst[32];
	unsigned long int sec, usec;
	struct timeval tm, reftm;
	ssize_t ilen;
	int i;

	/* Read from management pipe */
	p.pipe = 0;

	/* Reference time */
	gettimeofday(&reftm, NULL);

	while (!quit) {
		memset(&p, 0, sizeof(p));
		ilen = phy_read(cli_fd, &p, NRF24_MTU);
		if (ilen <= 0)
			continue;

		gettimeofday(&tm, NULL);
		if (tm.tv_usec < reftm.tv_usec) {
			sec = tm.tv_sec - reftm.tv_sec - 1;
			usec = tm.tv_usec + 1000000 - reftm.tv_usec;
		} else {
			sec = tm.tv_sec - reftm.tv_sec;
			usec = tm.tv_usec - reftm.tv_usec;
		}

		switch (ipdu->type) {
		/* If is a presente type */
		case NRF24_PDU_TYPE_PRESENCE:
			ll = (struct nrf24_ll_presence*) ipdu->payload;

			nrf24_mac2str(&ll->mac, src);
			printf("%05ld.%06ld nRF24: Beacon(0x%02x|P) plen:%zd\n",
						sec, usec, ipdu->type, ilen);
			printf("  %s %s\n", src, ll->name);
			break;
		/* If is a connect request type */
		case NRF24_PDU_TYPE_CONNECT_REQ:
			/* Link layer connect structure */
			llcn = (struct nrf24_ll_mgmt_connect *) ipdu->payload;

			/* Header type is a connect request type */
			printf("%05ld.%06ld nRF24: Connect Req(0x%02x) plen:%zd\n",
						sec, usec, ipdu->type, ilen);

			nrf24_mac2str(&llcn->src_addr, src);
			nrf24_mac2str(&llcn->dst_addr, dst);
			printf("  %s > %s\n", src, dst);
			printf("  CH: %d AA: %02x%02x%02x%02x%02x\n",
							llcn->channel,
							llcn->aa[0],
							llcn->aa[1],
							llcn->aa[2],
							llcn->aa[3],
							llcn->aa[4]);
			break;
		default:
			printf("%05ld.%06ld nRF24: Unknown (0x%02x) plen:%zd\n",
						sec, usec, ipdu->type, ilen);
			printf("  ");
			for (i = 0; i < ilen; i++)
				printf("%02x", raw[i]);
			printf("\n");
			break;
		}
	}
}

static GOptionEntry options[] = {
	{ "mode", 'm', 0, G_OPTION_ARG_STRING, &opt_mode,
				"mode", "Operation mode: server or client" },
	{ NULL },
};


int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	struct addr_pipe adrrp;

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

	cli_fd = phy_open("NRF0");
	if (cli_fd < 0) {
		printf("error open");
		return EXIT_FAILURE;
	}

	if (cli_fd < 0)
		return -1;

	g_option_context_free(context);

	printf("Sniffer nrfd knot %s\n", opt_mode);

	/* Sniffer in broadcast channel*/
	if (strcmp(opt_mode, "mgmt") == 0) {
		printf("listen mgmt\n");
		/* Set Channel */
		phy_ioctl(cli_fd, NRF24_CMD_SET_CHANNEL, &channel_mgmt);
		adrrp.pipe = 0;
		adrrp.ack = false;
		memcpy(adrrp.aa, aa_pipes[0], sizeof(aa_pipes[0]));
		phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);
		listen_mgmt();

	} else{
		/*Sniffer in data channel*/
		printf("listen raw\n");
		/* Set Channel */
		phy_ioctl(cli_fd, NRF24_CMD_SET_CHANNEL, &channel_raw);
		/* Open pipe zero */
		adrrp.pipe = 0;
		adrrp.ack = false;
		memcpy(adrrp.aa, aa_pipes[0], sizeof(aa_pipes[0]));
		phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);
		/* Open pipe 1 */
		adrrp.pipe = 1;
		adrrp.ack = false;
		memcpy(adrrp.aa, aa_pipes[1], sizeof(aa_pipes[1]));
		phy_ioctl(cli_fd, NRF24_CMD_SET_PIPE, &adrrp);
		listen_raw();
	}

	return 0;
}

