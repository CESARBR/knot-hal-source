/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <errno.h>
#include <stdint.h>
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

/* FIXME: Remove this header */
#include "hal/nrf24.h"

#include "hal/comm.h"

int hal_comm_init(const char *pathname, const void *params)
{
	struct termios toptions;
	int fd;
	speed_t brate;

	if (!pathname)
		return -EINVAL;

	fd = open(pathname, O_RDWR | O_NONBLOCK);
	if (fd == -1)  {
		perror("serialport_init: Unable to open port ");
		return -1;
	}

	if (tcgetattr(fd, &toptions) < 0) {
		perror("serialport_init: Couldn't get term attributes");
		return -1;
	}

	brate = B9600;
	cfsetispeed(&toptions, brate);
	cfsetospeed(&toptions, brate);

	// 8N1
	toptions.c_cflag &= ~PARENB;
	toptions.c_cflag &= ~CSTOPB;
	toptions.c_cflag &= ~CSIZE;
	toptions.c_cflag |= CS8;
	// no flow control
	toptions.c_cflag &= ~CRTSCTS;

	/* turn on READ & ignore ctrl lines */
	toptions.c_cflag |= CREAD | CLOCAL;
	toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

	toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
	toptions.c_oflag &= ~OPOST; // make raw

	// see: http://unixwiz.net/techtips/termios-vmin-vtime.html
	toptions.c_cc[VMIN]  = 2;
	toptions.c_cc[VTIME] = 0.75;

	tcsetattr(fd, TCSANOW, &toptions);
	if (tcsetattr(fd, TCSAFLUSH, &toptions) < 0) {
		perror("init_serialport: Couldn't set term attributes");
		return -1;
	}

	return fd;
}

int hal_comm_deinit(void)
{
	return -ENOSYS;
}

int hal_comm_socket(int domain, int protocol)
{
	return -ENOSYS;
}

int hal_comm_close(int sockfd)
{
	return close(sockfd);
}
/* Non-blocking read operation. Returns -EGAIN if there isn't data available */
ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{
	return read(sockfd, buffer, count);
}

/* Blocking write operation. Returns -EBADF if not connected */
ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{
	return write(sockfd, buffer, count);
}

int hal_comm_listen(int sockfd)
{
	return -ENOSYS;
}

/* Non-blocking operation. Returns -EGAIN if there isn't a new client */
int hal_comm_accept(int sockfd, void *addr)
{
	return -ENOSYS;
}

/* Blocking operation. Returns -ETIMEOUT */
int hal_comm_connect(int sockfd, uint64_t *addr)
{
	return -ENOSYS;
}
