/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARDUINO
#include "include/avr_unistd.h"
#endif

// Identifier of data type to be stored
#define HAL_STORAGE_ID_UUID		0
#define HAL_STORAGE_ID_TOKEN		1
#define HAL_STORAGE_ID_MAC		2
#define HAL_STORAGE_ID_SCHEMA_FLAG	3
#define HAL_STORAGE_ID_CONFIG		4
#define HAL_STORAGE_ID_PRIVATE_KEY      5
#define HAL_STORAGE_ID_PUBLIC_KEY       6

#define CONFIG_SIZE_UNITY		40

ssize_t hal_storage_read(uint16_t addr, uint8_t *value, size_t len);
ssize_t hal_storage_write(uint16_t addr, const uint8_t *value, size_t len);
ssize_t hal_storage_write_end(uint8_t id, void *value, size_t len);
ssize_t hal_storage_read_end(uint8_t id, void *value, size_t len);
void hal_storage_reset_end(void);

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_H__ */
