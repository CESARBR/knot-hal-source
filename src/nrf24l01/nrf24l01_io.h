/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
/* ---------------------------------------------
 * Register address map of nRF24L01(+)
*/
/* Configurator (reset value: 0b00001000) */
#define CONFIG			0x00
#define CONFIG_RST		0b00001000
#define CONFIG_MASK		0b01111111
#define CFG_MASK_RX_DR		0b01000000
#define CFG_MASK_TX_DS		0b00100000
#define CFG_MASK_MAX_RT		0b00010000
#define CFG_EN_CRC		0b00001000
#define CFG_CRCO		0b00000100
#define CFG_PWR_UP		0b00000010
#define CFG_PRIM_RX		0b00000001
