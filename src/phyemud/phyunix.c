/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "comm_private.h"

#define THING_TO_NRFD_UNIX_SOCKET	":thing:nrfd"

static int probe_unix(void)
{
	return 0;
}

static void remove_unix(void)
{
}

static int open_unix(const char *pathname)
{
	struct sockaddr_un addr;
	int err, sock;

	sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (sock < 0)
		return -errno;

	/* Represents unix socket from thing to nrfd */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path + 1, THING_TO_NRFD_UNIX_SOCKET,
					strlen(THING_TO_NRFD_UNIX_SOCKET));
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		err = -errno;
		return err;
	}

	return sock;
}

static int listen_unix(int sock)
{
	int err;

	if (listen(sock, 1) == -1) {
		err = -errno;
		return err;
	}

	return sock;
}

static int connect_unix(int sock, uint8_t to_addr)
{
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path + 1, THING_TO_NRFD_UNIX_SOCKET,
					strlen(THING_TO_NRFD_UNIX_SOCKET));

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		return -errno;

	return sock;
}

static int accept_unix(int srv_sock)
{
	return accept(srv_sock, NULL, NULL);
}

static ssize_t recv_unix(int sock, void *buffer, size_t len)
{
	return read(sock, buffer, len);
}

static ssize_t send_unix(int sock, const void *buffer, size_t len)
{
	return write(sock, buffer, len);
}

static void close_unix(int sock)
{
	close(sock);
}

struct phy_driver phy_unix = {
	.name = "Unix",
	.probe = probe_unix,
	.remove = remove_unix,
	.open = open_unix,
	.listen = listen_unix,
	.accept = accept_unix,
	.connect = connect_unix,
	.recv = recv_unix,
	.send = send_unix,
	.close = close_unix,
};
