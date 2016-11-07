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

#include "spi.h"

#define LOW			0
#define HIGH			1

/* delay for CSN */
#define DELAY_US		5

#define SPI_SPEED		1000000
#define BITS_PER_WORD		8

int8_t spi_init(const char *dev)
{
	int spi_fd;

#if defined(RPI_BOARD) || defined(RPI2_BOARD)
	spi_fd = open(dev, O_RDWR);
#endif

	if (spi_fd < 1)
		return -errno;

	return spi_fd;
}

void spi_deinit(int8_t spi_fd)
{
	if (spi_fd > 0) {
		close(spi_fd);
		spi_fd = -1;
	}
}

int spi_transfer(int8_t spi_fd, const uint8_t *tx, int ltx, uint8_t *rx,
		int lrx)
{
	struct spi_ioc_transfer data_ioc[2], *pdata_ioc = data_ioc;
	uint8_t  mode;
	uint8_t bits = 8;
	uint32_t speed = 1000000;
	uint16_t delay = 5;
	uint8_t *pdummy = NULL;
	int ntransfer = 0;
	unsigned int ret;

	if (spi_fd < 0)
		return -EIO;

	memset(data_ioc, 0, sizeof(data_ioc));

	/* If tx isn't empty, tx contains the command that will be send
	 * to spi( read or write command)
	 */
	if (tx != NULL && ltx != 0) {
		pdummy = (uint8_t *) malloc(ltx);
		if (pdummy == NULL)
			return -ENOMEM;

		memset(pdummy, 0, ltx);

		pdata_ioc->tx_buf = (unsigned long) tx;
		pdata_ioc->rx_buf = (unsigned long) pdummy;
		pdata_ioc->len = ltx;
		pdata_ioc->delay_usecs =
			(rx != NULL && lrx != 0) ? 0 : DELAY_US;
		pdata_ioc->cs_change = (rx != NULL && lrx != 0) ? LOW : HIGH;
		pdata_ioc->speed_hz = speed;
		pdata_ioc->bits_per_word = bits;
		++ntransfer;
		++pdata_ioc;

	}

	/* rx will receive from spi the value of the command that was
	 * send previously
	 */
	if (rx != NULL && lrx != 0) {
		pdata_ioc->tx_buf = (unsigned long) rx;
		pdata_ioc->rx_buf = (unsigned long) rx;
		pdata_ioc->len = lrx;
		pdata_ioc->delay_usecs = delay;
		pdata_ioc->cs_change = HIGH;
		pdata_ioc->speed_hz = speed;
		pdata_ioc->bits_per_word = bits;
		++ntransfer;
	}

	mode = SPI_MODE_0;

	ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	ioctl(spi_fd, SPI_IOC_RD_MODE, &mode);

	ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);

	ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(ntransfer), data_ioc);

	if (tx != NULL)
		free(pdummy);

	return ret ? -errno : 0;
}
