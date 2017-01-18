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
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <glib.h>

#include "include/nrf24.h"
#include "include/comm.h"
#include "manager.h"

/* Application packet size maximum, same as knotd */
#define PACKET_SIZE_MAX			512
#define KNOTD_UNIX_ADDRESS		"knot"

static int commfd;

struct session {
	unsigned int thing_id;	/* Thing event source */
	unsigned int knotd_id;	/* KNoT event source */
	GIOChannel *knotd_io;	/* Knotd GIOChannel reference */
	GIOChannel *thing_io;	/* Knotd GIOChannel reference */
};

static int connect_unix(void)
{
	struct sockaddr_un addr;
	int sock;

	sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (sock < 0)
		return -errno;

	/* Represents unix socket from seriald to knotd */
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

	session->knotd_id = 0;
	session->knotd_io = NULL;
}

static gboolean knotd_io_watch(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{
	struct session *session = user_data;
	char buffer[PACKET_SIZE_MAX];
	int thing_sock, knotd_sock;
	ssize_t readbytes_knotd;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		return FALSE;

	knotd_sock = g_io_channel_unix_get_fd(io);
	thing_sock = g_io_channel_unix_get_fd(session->thing_io);

	printf("Incoming data from knotd (%d)\n\r", knotd_sock);

	readbytes_knotd = read(knotd_sock, buffer, sizeof(buffer));
	if (readbytes_knotd < 0) {
		printf("read_knotd() error\n\r");
		return FALSE;
	}
	printf("RX_KNOTD: '%ld'\n\r", readbytes_knotd);

	if (hal_comm_write(thing_sock, buffer, readbytes_knotd) < 0) {
		printf("send_thing() error\n\r");
		return FALSE;
	}

	return TRUE;
}
/* If thing initiated disconnection decrement ref count */
static void serial_io_destroy(gpointer user_data)
{
	struct session *session = user_data;

	if (session->thing_id > 0) {
		g_io_channel_shutdown(session->thing_io, FALSE, NULL);
		g_io_channel_unref(session->thing_io);
	}
}

static gboolean serial_io_watch(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{
	struct session *session = user_data;
	char buffer[PACKET_SIZE_MAX];
	ssize_t nbytes;
	int sock, knotdfd, offset, msg_size, err, remaining;
	ssize_t i;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		session->thing_id = 0;
		return FALSE;
	}

	sock = g_io_channel_unix_get_fd(io);

	printf("Generic IO Watch, reading from (%d)\n\r", sock);

	nbytes = hal_comm_read(sock, (void *)buffer, sizeof(buffer));
	if (nbytes < 0) {
		printf("read() error\n");
		return FALSE;
	}
	printf("Read (%zu) bytes from thing\n\r", nbytes);

	printf("Opt type = (0x%02X), Payload length = (%d)\n", buffer[0],
								buffer[1]);

	printf("Read \"");
	for (i = 0; i < nbytes; i++)
		printf("%c ", buffer[(int)i]);
	printf("\" from thing\n\r");
	/*
	 * At the moment there isn't a header describing the size of
	 * the datagram. The field 'payload_len' (see buffer[1]) defined
	 * at knot_protocol.h is being used to determine the expected
	 * datagram length.
	 */
	msg_size = buffer[1] + 2;

	offset = (int)nbytes;
	/* If payload + header (2 Bytes) < nbytes, keep reading */
	while (offset < msg_size) {
		remaining = sizeof(buffer) - offset;
		if (remaining > 0)
			nbytes = hal_comm_read(sock, buffer + offset,
								remaining);
		else
			goto done;
		err = errno;
		/* Only consider nbytes < 0 when errno is not EAGAIN*/
		if (nbytes < 0 && err != EAGAIN) {
			/* Malformed datagram: ignore received data */
			goto done;
		} else if (nbytes > 0)
			offset += nbytes;
	}

	printf("Total bytes read = %d\n", offset);
	printf("Read \"");
	for (i = 0; (int)i < offset; i++)
		printf("%c ", buffer[(int)i]);
	printf("\" from thing\n\r");

	knotdfd = g_io_channel_unix_get_fd(session->knotd_io);

	if (write(knotdfd, buffer, msg_size) < 0) {
		printf("write_knotd() error\n\r");
		return FALSE;
	}

done:

	return TRUE;
}

static int serial_start(const char *pathname)
{
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	struct session *session;
	GIOChannel *io;
	int knotdfd;

	commfd = hal_comm_init(pathname, NULL);
	printf("commfd = (%d)\n\r", commfd);
	if (commfd < 0)
		return -errno;

	knotdfd = connect_unix();
	if (knotdfd < 0) {
		hal_comm_close(commfd);
		return FALSE;
	}

	printf("Serial server started\n\r");

	/* Tracking thing connection & data */
	io = g_io_channel_unix_new(commfd);
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

	session->thing_id = g_io_add_watch_full(io, G_PRIORITY_DEFAULT,
						cond, serial_io_watch, session,
						serial_io_destroy);
	g_io_channel_unref(io);

	return 0;
}

static void serial_stop(void)
{
	printf("serial_stop\n\r");
	hal_comm_close(commfd);
}

int manager_start(const char *serial)
{
	if (serial)
		return serial_start(serial);

	return 0;
}

void manager_stop(void)
{
	serial_stop();

	printf("Manager stop\n");
}
