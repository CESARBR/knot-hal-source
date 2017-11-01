/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
/* necessary to compile in arduino */
#ifdef __cplusplus
extern "C"{
#endif

int8_t spi_bus_init(const char *dev);
int spi_bus_transfer(int8_t spi_fd, const uint8_t *tx, int ltx, uint8_t *rx,
			int lrx);
void spi_bus_deinit(int8_t spi_fd);

#ifdef __cplusplus
}
#endif
