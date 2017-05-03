/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined (RPI_BOARD) || defined (RPI2_BOARD)

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

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

#define INPUT 0
#define OUTPUT 1

#define LOW 0
#define HIGH 1
#endif

int hal_gpio_setup(void);
void hal_gpio_unmap(void);
void hal_gpio_pin_mode(uint8_t, uint8_t);
void hal_gpio_digital_write(uint8_t, uint8_t);
int hal_gpio_digital_read(uint8_t);
int hal_gpio_analog_read(uint8_t);
void hal_gpio_analog_reference(uint8_t mode);
void hal_gpio_analog_write(uint8_t, int);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_GPIO_H__ */
