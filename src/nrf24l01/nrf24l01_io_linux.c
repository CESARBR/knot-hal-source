/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include "nrf24l01_io.h"
#include "spi.h"

#define CE	25

#define LOW	0
#define HIGH	1

#define BCM2709_RPI2		0x3F000000
#define BCM2708_RPI		0x20000000

#ifdef RPI2_BOARD
#define BCM2708_PERI_BASE	BCM2709_RPI2
#elif RPI_BOARD
#define BCM2708_PERI_BASE	BCM2708_RPI
#else
#error Board identifier required to BCM2708_PERI_BASE.
#endif

#define GPIO_BASE		(BCM2708_PERI_BASE + 0x200000)
#define PAGE_SIZE		(4*1024)
#define BLOCK_SIZE		(4*1024)

/* Raspberry pi GPIO Macros */
#define INP_GPIO(g)		(*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3)))
#define OUT_GPIO(g)		(*(gpio+((g)/10)) |=  (1<<(((g)%10)*3)))
#define SET_GPIO_ALT(g, a)	(*(gpio+(((g)/10))) |= (((a) <= 3?(a)+4:(a) == 4?3:2)\
							<< (((g)%10)*3)))

/* sets bits which are 1 ignores bits which are 0 */
#define GPIO_SET		*(gpio+7)

/* clears bits which are 1 ignores bits which are 0 */
#define GPIO_CLR		*(gpio+10)

/* 0 if LOW, (1<<g) if HIGH */
#define GET_GPIO(g)		(*(gpio+13) & (1 << g))

/*
 * Pull up/pull down followed by pull up/down clock
 */
#define GPIO_PULL		*(gpio+37)
#define GPIO_PULLCLK0		*(gpio+38)

static volatile unsigned	*gpio;

void delay_us(float us)
{
	usleep(us);
}

void enable(void)
{
	GPIO_SET = (1<<CE);
}

void disable(void)
{
	GPIO_CLR = (1<<CE);
}

int io_setup(const char *dev)
{
	int mem_fd;

	mem_fd = open("/dev/mem", O_RDWR|O_SYNC);

	if (mem_fd < 0)
		return -errno;

	gpio = (volatile unsigned *)mmap(NULL, BLOCK_SIZE,
						PROT_READ | PROT_WRITE,
						MAP_SHARED, mem_fd, GPIO_BASE);
	close(mem_fd);

	if (gpio == MAP_FAILED)
		return -errno;

	GPIO_CLR = (1<<CE);
	INP_GPIO(CE);
	OUT_GPIO(CE);

	disable();
	return spi_init(dev);
}

void io_reset(int spi_fd)
{
	disable();
	munmap((void*)gpio, BLOCK_SIZE);
	spi_deinit(spi_fd);
}
