/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <glib.h>

#include "comm_private.h"
#include "manager.h"

/* Application packet size maximum, same as knotd */
#define PACKET_SIZE_MAX			512
#define KNOTD_UNIX_ADDRESS		"knot"

static guint unix_watch_id = 0;

struct session {
	unsigned int thing_id;	/* Thing event source */
	unsigned int knotd_id;	/* KNoT event source */
	GIOChannel *knotd_io;	/* Knotd GIOChannel reference */
	GIOChannel *thing_io;	/* Knotd GIOChannel reference */
	struct phy_driver *ops;
};

extern struct phy_driver phy_unix;
extern struct phy_driver phy_serial;

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
	struct session *session = user_data;

	session->knotd_io = NULL;
	session->knotd_id = 0;
	printf("knotd_io_destroy\n\r");
}

static gboolean knotd_io_watch(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{
	struct session *session = user_data;
	char buffer[PACKET_SIZE_MAX];
	struct phy_driver *ops = session->ops;
	int thing_sock, knotd_sock;
	ssize_t readbytes_knotd;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		return FALSE;

	knotd_sock = g_io_channel_unix_get_fd(io);
	thing_sock = g_io_channel_unix_get_fd(session->thing_io);

	printf("Incomming data from knotd (%d)\n\r", knotd_sock);

	readbytes_knotd = read(knotd_sock, buffer, sizeof(buffer));
	if (readbytes_knotd < 0) {
		printf("read_knotd() error\n\r");
		return FALSE;
	}
	printf("RX_KNOTD: '%ld'\n\r", readbytes_knotd);

	if (ops->send(thing_sock, buffer, readbytes_knotd) < 0) {
		printf("send_thing() error\n\r");
		return FALSE;
	}

	return TRUE;
}

static void generic_io_destroy(gpointer user_data)
{
	struct session *session = user_data;
	GIOChannel *thing_io;

	thing_io = session->thing_io;

	if (session->thing_id) {
		g_source_remove(session->thing_id);

		g_io_channel_shutdown(thing_io, FALSE, NULL);
		g_io_channel_unref(thing_io);
	}

	g_free(session);
	printf("generic_io_destroy\n\r");
}

static gboolean generic_io_watch(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{
	struct session *session = user_data;
	struct phy_driver *ops = session->ops;
	char buffer[PACKET_SIZE_MAX];
	ssize_t nbytes;
	int sock, knotdfd, offset, msg_size;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		return FALSE;

	sock = g_io_channel_unix_get_fd(io);

	printf("Generic IO Watch, reading from (%d)\n\r", sock);

	nbytes = ops->recv(sock, buffer, sizeof(buffer));
	if (nbytes < 0) {
		printf("read() error\n");
		return FALSE;
	}
	printf("Read (%ld) bytes from thing\n\r", nbytes);

	printf("Opt type = (%02X), Payload length = (%d)\n", buffer[0],
								buffer[1]);

	/*
	 * At the moment there isn't a header describing the size of
	 * the datagram. The field 'payload_len' (see buffer[1]) defined
	 * at knot_protocol.h is being used to determine the expected
	 * datagram length.
	 */
	msg_size = buffer[1] + 2;

	offset = nbytes;
	/* If payload + header (2 Bytes) < nbytes, keep reading */
	while (offset < msg_size) {
		nbytes = ops->recv(sock, buffer + offset,
						sizeof(buffer - offset));
		if (nbytes < 0) {
			 /* Malformed datagram: ignore received data */
			goto done;
		}

		offset += nbytes;
	}

	printf("Total bytes read = %d\n", offset);

	knotdfd = g_io_channel_unix_get_fd(session->knotd_io);

	if (write(knotdfd, buffer, msg_size) < 0) {
		printf("write_knotd() error\n\r");
		return FALSE;
	}

done:

	return TRUE;
}

static gboolean generic_accept_cb(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{
	GIOChannel *thing_io, *knotd_io;
	int cli_sock, srv_sock, knotdfd;
	struct session *session;
	struct phy_driver *ops = user_data;

	if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR))
		return FALSE;

	/* Accepting thing connection */
	srv_sock = g_io_channel_unix_get_fd(io);
	cli_sock = ops->accept(srv_sock);

	if (cli_sock < 0)
		return TRUE;

	knotdfd = connect_unix();
	if (knotdfd < 0) {
		ops->close(cli_sock);
		return TRUE;
	}
	printf("Connected to (%d)\n\r", knotdfd);

	/* Tracking unix socket connection & data */
	knotd_io = g_io_channel_unix_new(knotdfd);
	g_io_channel_set_flags(knotd_io, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_close_on_unref(knotd_io, TRUE);

	/* Tracking thing connection & data */
	thing_io = g_io_channel_unix_new(cli_sock);
	g_io_channel_set_flags(thing_io, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_close_on_unref(thing_io, TRUE);

	session = g_new0(struct session, 1);
	session->knotd_io = knotd_io;
	session->thing_io = thing_io;
	session->ops = ops;

	/* Watch for unix socket disconnection */
	session->thing_id = g_io_add_watch_full(thing_io, G_PRIORITY_DEFAULT,
				G_IO_HUP | G_IO_NVAL | G_IO_ERR | G_IO_IN,
				generic_io_watch, session, generic_io_destroy);
	g_io_channel_unref(thing_io);

	/* Watch for unix socket disconnection */
	session->knotd_id = g_io_add_watch_full(knotd_io, G_PRIORITY_DEFAULT,
				G_IO_HUP | G_IO_NVAL | G_IO_ERR | G_IO_IN,
				knotd_io_watch, session, knotd_io_destroy);
	g_io_channel_unref(knotd_io);

	return TRUE;
}

static int unix_start(void)
{
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	GIOChannel *io;
	int sock;

	phy_unix.probe();
	sock = phy_unix.open(NULL);
	if (sock < 0)
		return sock;

	sock = phy_unix.listen(sock);
	if (sock < 0) {
		phy_unix.close(sock);
		return -1;
	}

	printf("Unix server started\n\r");
	io = g_io_channel_unix_new(sock);
	g_io_channel_set_flags(io, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_close_on_unref(io, TRUE);

	unix_watch_id = g_io_add_watch(io, cond, generic_accept_cb, &phy_unix);

	/* Keep only one ref: server watch  */
	g_io_channel_unref(io);

	return 0;
}

static int serial_start(const char *pathname)
{
	struct session *session;
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	GIOChannel *io;
	int virtualfd, realfd, knotdfd;

	if (phy_serial.probe() < 0)
		return -EIO;

	virtualfd = phy_serial.open(pathname);
	printf("virtualfd = (%d)\n\r", virtualfd);

	realfd = phy_serial.listen(0);
	if (realfd < 0) {
		phy_serial.close(realfd);
		return -errno;
	}

	knotdfd = connect_unix();
	if (knotdfd < 0) {
		phy_unix.close(realfd);
		return FALSE;
	}

	printf("Serial server started\n\r");

	/* Tracking thing connection & data */
	io = g_io_channel_unix_new(realfd);
	g_io_channel_set_flags(io, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_close_on_unref(io, TRUE);

	session = g_new0(struct session, 1);

	/* Watch knotd socket */
	session->knotd_io = g_io_channel_unix_new(knotdfd);
	g_io_channel_set_flags(session->knotd_io, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_close_on_unref(session->knotd_io, TRUE);

	session->knotd_id = g_io_add_watch_full(session->knotd_io,
							G_PRIORITY_DEFAULT,
							cond,
							knotd_io_watch, session,
							knotd_io_destroy);
	g_io_channel_unref(session->knotd_io);

	session->thing_io = io;
	session->ops = &phy_serial;

	session->thing_id = g_io_add_watch_full(io, G_PRIORITY_DEFAULT,
						cond, generic_io_watch, session,
							generic_io_destroy);
	g_io_channel_unref(io);

	return 0;
}

static void unix_stop(void)
{
	if (unix_watch_id)
		g_source_remove(unix_watch_id);

	phy_unix.remove();
}

static void serial_stop(void)
{
	phy_serial.remove();
}

int manager_start(const char *serial, gboolean unix_sock)
{
	int err;

	if (unix_sock)
		err = unix_start();

	if (serial)
		err = serial_start(serial);

	if (err < 0)
		return err;

	return 0;
}

void manager_stop(void)
{
	unix_stop();
	serial_stop();
}
