/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "spi_bus.h"

#define BITS_PER_WORD		8
#define MSBFIRST		0

static uint8_t *pdummy;
static int pdummy_len;

static uint32_t speed = 10000000; /* 10 MHz */

int8_t spi_bus_init(const char *dev)
{
	int spi_fd;

	spi_fd = open(dev, O_RDWR);

	if (spi_fd < 1)
		return -errno;

	ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
		close(spi_fd);
		return -errno;
	}

	pdummy = (uint8_t *) malloc(sizeof(uint8_t));
	if (pdummy == NULL) {
		close(spi_fd);
		return -ENOMEM;
	}
	pdummy_len = 1;

	return spi_fd;
}

void spi_bus_deinit(int8_t spi_fd)
{
	if (spi_fd > 0) {
		close(spi_fd);
	}

	free(pdummy);
	pdummy_len = 0;
}

/*
 * This function performs a write through SPI without
 * overwriting the tx buffer and then performs a read
 * on the device.
 *
 * This strategy is particularly good when dealing with
 * simple devices such as radios, nfc readers and memory
 * chips. Those devices usually wait for a command and
 * then answer it.
 *
 * With this function is also possible to perform a
 * more common SPI transfer (which reads and writes
 * at the same time and overwrite the tx buffer) by
 * simply calling:
 *
 * 	spi_bus_transfer(spi_fd, NULL, 0, tx_buffer, tx_len);
 *
 */
int spi_bus_transfer(int8_t spi_fd, const uint8_t *tx, int ltx, uint8_t *rx,
		int lrx)
{
	struct spi_ioc_transfer data_ioc[2],
				*pdata_ioc = data_ioc;
	uint8_t mode = SPI_MODE_0,
		bits = BITS_PER_WORD,
		lsbfirst = MSBFIRST;
	int ntransfer = 0;
	unsigned int ret;

	if (spi_fd < 0)
		return -EIO;

	memset(data_ioc, 0, sizeof(data_ioc));

	/*
	 * If tx isn't empty, tx contains the command that will be send
	 * to spi( read or write command)
	 */
	if (tx != NULL && ltx != 0) {
		/*
		 * Resize the dummy buffer using malloc() for better
		 * performance, once realloc() may have to move the
		 * content of the buffer when it allocates more space.
		 */
		if (ltx > pdummy_len) {
			if (pdummy != NULL)
				free(pdummy);

			pdummy = (uint8_t *) malloc(ltx * sizeof(uint8_t));
			if (pdummy == NULL)
				return -ENOMEM;

			pdummy_len = ltx;
		}

		pdata_ioc->tx_buf = (unsigned long) tx;
		pdata_ioc->rx_buf = (unsigned long) pdummy;
		pdata_ioc->len = ltx;
		++ntransfer;
		++pdata_ioc;
	}

	/*
	 * rx will receive from spi the value of the command that was
	 * send previously
	 */
	if (rx != NULL && lrx != 0) {
		pdata_ioc->tx_buf = (unsigned long) rx;
		pdata_ioc->rx_buf = (unsigned long) rx;
		pdata_ioc->len = lrx;
		++ntransfer;
	}

	ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &lsbfirst);

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(ntransfer), data_ioc);

	return ret ? -errno : 0;
}
