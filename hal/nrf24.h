/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __HAL_NRF24_H__
#define __HAL_NRF24_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * nRF24 64-bits address example (string)
 * 88:77:66:55:44:33:22:11 (On left MSB of the address)
 * b0:b1:b2:b3:b4:b5:b6:b7 (mapping to nrf24_mac.b[i])
 * Address should not be converted to host order due
 * potential errors related to endianess.
 */
struct nrf24_mac {
	union {
		uint8_t b[8];
		uint64_t uint64;
	} address;
};

/* Converts nrf24_mac address to string */
int nrf24_mac2str(const struct nrf24_mac *, char *);

/* Converts string to nrf24_mac address */
int nrf24_str2mac(const char *, struct nrf24_mac *);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_NRF24_H__ */
