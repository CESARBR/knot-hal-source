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
#include <termios.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "phy_driver_private.h"

struct serial_opts {
	char	tty[24];
	int	vmin;
	int	virtualfd;
	int	realfd;
	int	knotd;
};

static struct serial_opts serial_opts;

/* Check if tty path is available*/
static int serial_probe(const char *spi, uint8_t tx_power)
{
	return 0;
}


/* Returns last character from the pathname, like 0 in /tty/ttyUSB0 */
static int serial_open(const char *pathname)
{
	struct stat st;

	/* Setting default value */
	if (!pathname)
		return -EINVAL;

	strncpy(serial_opts.tty, pathname, sizeof(serial_opts.tty));
	serial_opts.vmin = 8;

	if (stat(serial_opts.tty, &st) < 0)
		return -errno;

	return serial_opts.tty[strlen(serial_opts.tty)-1] - '0';
}

static void serial_remove(void)
{
}

static int serial_listen(int sock, uint8_t channel)
{
	struct termios term;
	int ttyfd;

	memset(&term, 0, sizeof(term));
	/*
	 * 8-bit characters, no parity bit,no Bit mask for data bits
	 * only need 1 stop bit
	 */
	term.c_cflag &= ~PARENB;
	term.c_cflag &= ~CSTOPB;
	term.c_cflag &= ~CSIZE;
	term.c_cflag |= CS8;
	/* No flow control*/
	term.c_cflag &= ~CRTSCTS;
	/* Read block until 2 bytes arrives */
	term.c_cc[VMIN] = 2;
	/* Or 0.075 seconds read timeout */
	term.c_cc[VTIME] = 0.75;
	/* Turn on READ & ignore ctrl lines */
	term.c_cflag |= CREAD | CLOCAL;

	cfsetospeed(&term, B9600);
	cfsetispeed(&term, B9600);

	ttyfd = open(serial_opts.tty, O_RDWR | O_NOCTTY);
	if (ttyfd < 0)
		return -errno;
	tcflush(ttyfd, TCIFLUSH);
	tcsetattr(ttyfd, TCSANOW, &term);

	return ttyfd;
}

static int serial_accept(int srv_sockfd)
{
	/* Map virtual fd to pipe */
	return 0;
}

static int serial_connect(int sock, uint8_t to_addr)
{
	/*TODO: Already implemented in ktool*/
	return 0;
}

static ssize_t serial_recv(int sockfd, void *buffer, size_t len)
{
	return read(sockfd, buffer, len);
}

static ssize_t serial_send(int sockfd, const void *buffer, size_t len)
{
	return write(sockfd, buffer, len);
}

/* close no virtual marca na tabela */
/* close no real realmente fecha */
static void serial_close(int fd)
{
	close(fd);
}

struct phy_driver phy_serial = {
	.name = "Serial",
	.open = serial_open,
	.probe = serial_probe,
	.remove = serial_remove,
	.close = serial_close,
	.listen = serial_listen,
	.accept = serial_accept,
	.connect = serial_connect,
	.recv = serial_recv,
	.send = serial_send
};
