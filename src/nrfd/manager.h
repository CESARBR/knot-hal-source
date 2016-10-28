/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

int manager_start(const char *file, const char *host, int port,
			const char *spi, uint8_t channel, uint8_t tx_pwr);
void manager_stop(void);
