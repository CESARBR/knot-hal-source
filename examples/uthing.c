/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "comm_private.h"

/* Application packet size maximum */
#define PACKET_SIZE_MAX			512

#define MESSAGE				"hello world"
#define MESSAGE_SIZE			(sizeof(MESSAGE)-1)

extern struct phy_driver phy_unix;

int main(int argc, char *argv[])
{
	char buffer[PACKET_SIZE_MAX];
	ssize_t nbytes;
	int err, sock;

	printf("KNoT Thing Starting..\n");

	sock = phy_unix.open(NULL);
	if (sock < 0)
		return -errno;

	if (phy_unix.connect(sock, 0) == -1) {
		err = errno;
		phy_unix.close(sock);
		return -err;
	}

	printf("Connected to nrfd\n");

	nbytes = phy_unix.send(sock, MESSAGE, MESSAGE_SIZE);
	if (nbytes < 0) {
		err = errno;
		printf("write(): %s(%d)\n", strerror(err), err);
		close(sock);
		return -err;
	}

	nbytes = phy_unix.recv(sock, &buffer, sizeof(buffer));
	if (nbytes < 0) {
		err = errno;
		printf("read(): %s(%d)\n", strerror(err), err);
		close(sock);
		return -err;
	}

	printf("RX:[%zu]: '%.*s'\n", nbytes, (int) nbytes, buffer);

	return 0;
}
