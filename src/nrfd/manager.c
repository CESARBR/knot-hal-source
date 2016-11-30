/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <glib.h>
#include <json-c/json.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "include/comm.h"
#include "include/nrf24.h"

#include "nrf24l01_io.h"
#include "manager.h"

#define KNOTD_UNIX_ADDRESS		"knot"
#define MAX_PEERS 5
static int mgmtfd;
static guint mgmtwatch;

struct peer {
	struct nrf24_mac mac;
	int8_t socket_fd;
	int8_t knotd_fd;
	GIOChannel *knotd_io;
	unsigned int knotd_id;
};

static struct peer peers[MAX_PEERS] = {
	{.mac.address.uint64 = 0, .socket_fd = -1},
	{.mac.address.uint64 = 0, .socket_fd = -1},
	{.mac.address.uint64 = 0, .socket_fd = -1},
	{.mac.address.uint64 = 0, .socket_fd = -1},
	{.mac.address.uint64 = 0, .socket_fd = -1}
};

static uint8_t count_clients;

/* Get peer position in vector of peers*/
static int8_t get_peer(struct nrf24_mac mac)
{
	int8_t i;

	for (i = 0; i < MAX_PEERS; i++)
		if (peers[i].mac.address.uint64 == mac.address.uint64)
			return i;

	return -EINVAL;
}

/* Get free position in vector for peers*/
static int8_t get_peer_index(void)
{
	int8_t i;

	for (i = 0; i < MAX_PEERS; i++)
		if (peers[i].socket_fd == -1)
			return i;

	return -EINVAL;
}

static int connect_unix(void)
{
	struct sockaddr_un addr;
	int sock;

	sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (sock < 0)
		return -errno;

	/* Represents unix socket from nrfd to knotd */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path + 1, KNOTD_UNIX_ADDRESS,
					strlen(KNOTD_UNIX_ADDRESS));

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		return -errno;

	return sock;
}

static void knotd_io_destroy(gpointer user_data)
{

	struct peer *p = (struct peer *)user_data;

	hal_comm_close(p->socket_fd);
	p->socket_fd = -1;
	p->knotd_id = 0;
	p->knotd_io = NULL;
}

static gboolean knotd_io_watch(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{

	char buffer[128];
	ssize_t readbytes_knotd;
	struct peer *p = (struct peer *)user_data;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		return FALSE;

	/* Read data from Knotd */
	readbytes_knotd = read(p->knotd_fd, buffer, sizeof(buffer));
	if (readbytes_knotd < 0) {
		printf("read_knotd() error\n\r");
		return FALSE;
	}

	/* Send data to thing */
	/* TODO: put data in list for transmission */
	hal_comm_write(p->socket_fd, buffer, readbytes_knotd);

	return TRUE;
}

static int8_t evt_presence(struct mgmt_nrf24_header *mhdr)
{
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	int8_t position;
	int err;
	struct mgmt_evt_nrf24_bcast_presence *evt_pre =
			(struct mgmt_evt_nrf24_bcast_presence *) mhdr->payload;


	if (count_clients >= MAX_PEERS)
		return -EUSERS; /*MAX PEERS*/

	/*Check if this peer is already allocated */
	position = get_peer(evt_pre->mac);
	/* If this is a new peer */
	if (position < 0) {
		/* Get free peers position */
		position = get_peer_index();
		if (position < 0)
			return position;

		/*Create Socket */
		err = hal_comm_socket(HAL_COMM_PF_NRF24, HAL_COMM_PROTO_RAW);
		if (err < 0)
			return err;

		peers[position].socket_fd = err;

		peers[position].knotd_fd = connect_unix();
		if (peers[position].knotd_fd < 0) {
			hal_comm_close(peers[position].socket_fd);
			peers[position].socket_fd = -1;
			return peers[position].knotd_fd;
		}

		/* Set mac value for this position */
		peers[position].mac.address.uint64 =
				evt_pre->mac.address.uint64;

		/* Watch knotd socket */
		peers[position].knotd_io =
			g_io_channel_unix_new(peers[position].knotd_fd);
		g_io_channel_set_flags(peers[position].knotd_io,
			G_IO_FLAG_NONBLOCK, NULL);
		g_io_channel_set_close_on_unref(peers[position].knotd_io,
			TRUE);

		peers[position].knotd_id =
			g_io_add_watch_full(peers[position].knotd_io,
						G_PRIORITY_DEFAULT,
						cond,
						knotd_io_watch,
						&peers[position],
						knotd_io_destroy);
		g_io_channel_unref(peers[position].knotd_io);

		count_clients++;
	}

	/*Send Connect */
	hal_comm_connect(peers[position].socket_fd,
			&evt_pre->mac.address.uint64);
	return 0;
}

/* Read RAW from Clients */
static int8_t clients_read()
{
	int8_t i;
	uint8_t buffer[256];
	int ret;

	/*No client */
	if (count_clients == 0)
		return 0;

	for (i = 0; i < MAX_PEERS; i++) {
		if (peers[i].socket_fd == -1)
			continue;

		ret = hal_comm_read(peers[i].socket_fd, &buffer,
			sizeof(buffer));
		if (ret > 0) {
			if (write(peers[i].knotd_fd, buffer, ret) < 0)
				printf("write_knotd() error\n\r");
		}
	}
	return 0;
}

static int8_t mgmt_read(void)
{

	uint8_t buffer[256];
	struct mgmt_nrf24_header *mhdr = (struct mgmt_nrf24_header *) buffer;
	ssize_t rbytes;

	rbytes = hal_comm_read(mgmtfd, buffer, sizeof(buffer));

	/* mgmt on bad state? */
	if (rbytes < 0 && rbytes != -EAGAIN)
		return -1;

	/* Nothing to read? */
	if (rbytes == -EAGAIN)
		return -1;

	/* Return/ignore if it is not an event? */
	if (!(mhdr->opcode & 0x0200))
		return -1;

	switch (mhdr->opcode) {

	case MGMT_EVT_NRF24_BCAST_PRESENCE:
		evt_presence(mhdr);
		break;

	case MGMT_EVT_NRF24_BCAST_SETUP:
		break;

	case MGMT_EVT_NRF24_BCAST_BEACON:
		break;

	case MGMT_EVT_NRF24_DISCONNECTED:
		break;
	}
	return 0;
}

static gboolean read_idle(gpointer user_data)
{
	mgmt_read();
	clients_read();
	return TRUE;
}

static int radio_init(const char *spi, uint8_t channel, uint8_t rfpwr)
{
	int err;

	err = hal_comm_init("NRF0");
	if (err < 0)
		return err;

	mgmtfd = hal_comm_socket(HAL_COMM_PF_NRF24, HAL_COMM_PROTO_MGMT);
	if (mgmtfd < 0)
		goto done;

	mgmtwatch = g_idle_add(read_idle, NULL);

	return 0;
done:
	hal_comm_deinit();

	return mgmtfd;
}

static void close_clients(void)
{
	int i;

	for (i = 0; i < MAX_PEERS; i++) {
		hal_comm_close(peers[i].socket_fd);
		peers[i].socket_fd = -1;
		peers[i].mac.address.uint64 = 0;
	}
}

static void radio_stop(void)
{
	if (mgmtwatch)
		g_source_remove(mgmtwatch);

	hal_comm_close(mgmtfd);
	close_clients();
	hal_comm_deinit();
}

static gboolean nrf_data_watch(GIOChannel *io, GIOCondition cond,
						gpointer user_data)
{
	char buffer[1024];
	GIOStatus status;
	GError *gerr = NULL;
	gsize rbytes;

	/*
	 * Manages TCP data from spiproxyd(nRF proxy). All traffic(raw
	 * data) should be transferred using unix socket to knotd.
	 */

	if (cond & (G_IO_HUP | G_IO_ERR))
		return FALSE;

	memset(buffer, 0, sizeof(buffer));

	/* Incoming data through TCP socket */
	status = g_io_channel_read_chars(io, buffer, sizeof(buffer),
						 &rbytes, &gerr);
	if (status == G_IO_STATUS_ERROR) {
		printf("read(): %s\n", gerr->message);
		g_error_free(gerr);
		return FALSE;
	}

	if (rbytes == 0)
		return FALSE;

	/*
	 * Decode based on nRF PIPE information and forward
	 * the data through a unix socket to knotd.
	 */
	printf("read(): %lu bytes\n", rbytes);

	return TRUE;
}

static int tcp_init(const char *host, int port)
{
	GIOChannel *io;
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP;
	struct hostent *hostent;		/* Host information */
	struct in_addr h_addr;			/* Internet address */
	struct sockaddr_in server;		/* nRF proxy: spiproxyd */
	int err, sock;

	hostent = gethostbyname(host);
	if (hostent == NULL) {
		err = errno;
		fprintf(stderr, "gethostbyname(): %s(%d)\n",
						strerror(err), err);
		return -err;
	}

	h_addr.s_addr = *((unsigned long *) hostent-> h_addr_list[0]);

	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		err = errno;
		fprintf(stderr, "socket(): %s(%d)\n", strerror(err), err);
		return -err;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = h_addr.s_addr;
	server.sin_port = htons(port);

	err = connect(sock, (struct sockaddr *) &server, sizeof(server));
	if (err < 0) {
		err = errno;
		fprintf(stderr, "connect(): %s(%d)\n", strerror(err), err);
		close(sock);
		return -err;
	}

	fprintf(stdout, "nRF Proxy address: %s\n", inet_ntoa(h_addr));

	io = g_io_channel_unix_new(sock);
	g_io_channel_set_close_on_unref(io, TRUE);

	/* Ending 'NULL' for binary data */
	g_io_channel_set_encoding(io, NULL, NULL);
	g_io_channel_set_buffered(io, FALSE);

	/* TCP handler: incoming data from spiproxyd (nRF proxy) */
	g_io_add_watch(io, cond, nrf_data_watch, NULL);

	/* Keep only one reference: watch */
	g_io_channel_unref(io);

	return 0;
}

static char *load_config(const char *file)
{
	char *buffer = NULL;
	int length;
	FILE *fl = fopen(file, "r");

	if (fl == NULL) {
		fprintf(stderr, "No such file available: %s\n", file);
		return NULL;
	}

	fseek(fl, 0, SEEK_END);
	length = ftell(fl);
	fseek(fl, 0, SEEK_SET);

	buffer = (char *) malloc((length+1)*sizeof(char));
	if (buffer) {
		fread(buffer, length, 1, fl);
		buffer[length] = '\0';
	}
	fclose(fl);

	return buffer;
}

/* Set TX Power from dBm to values defined at nRF24 datasheet */
static uint8_t dbm_int2rfpwr(int dbm)
{
	switch (dbm) {

	case 0:
		return NRF24_PWR_0DBM;

	case -6:
		return NRF24_PWR_6DBM;

	case -12:
		return NRF24_PWR_12DBM;

	case -18:
		return NRF24_PWR_18DBM;
	}

	/* Return default value when dBm value is invalid */
	return NRF24_PWR_0DBM;
}

/*
 * TODO: Get "host", "spi" and "port"
 * parameters when/if implemented
 * in the json configuration file
 */
static int parse_config(const char *config, int *channel, int *dbm)
{
	json_object *jobj, *obj_radio, *obj_tmp;

	int err = -EINVAL;

	jobj = json_tokener_parse(config);
	if (jobj == NULL)
		return -EINVAL;

	if (!json_object_object_get_ex(jobj, "radio", &obj_radio))
		goto done;

	if (json_object_object_get_ex(obj_radio, "channel", &obj_tmp))
		*channel = json_object_get_int(obj_tmp);

	if (json_object_object_get_ex(obj_radio,  "TxPower", &obj_tmp))
		*dbm = json_object_get_int(obj_tmp);

	/* Success */
	err = 0;

done:
	/* Free mem used in json parse: */
	json_object_put(jobj);
	return err;
}

int manager_start(const char *file, const char *host, int port,
				const char *spi, int channel, int dbm)
{
	int cfg_channel = NRF24_CH_MIN, cfg_dbm = 0;
	char *json_str;
	int err;

	/* Command line arguments have higher priority */
	json_str = load_config(file);
	if (json_str != NULL) {
		err = parse_config(json_str, &cfg_channel, &cfg_dbm);
		free(json_str);
	}

	if (err < 0) {
		fprintf(stderr, "Invalid configuration file: %s\n", file);
		return err;
	}

	 /* Validate and set the channel */
	if (channel < 0 || channel > 125)
		channel = cfg_channel;

	/*
	 * Use TX Power from configuration file if it has not been passed
	 * through cmd line. -255 means invalid: not informed by user.
	 */
	if (dbm == -255)
		dbm = cfg_dbm;

	if (host == NULL)
		return radio_init(spi, channel, dbm_int2rfpwr(dbm));
	/*
	 * TCP development mode: Linux connected to RPi(phynrfd radio
	 * proxy). Connect to phynrfd routing all traffic over TCP.
	 */
	return tcp_init(host, port);
}

void manager_stop(void)
{
	radio_stop();
}
