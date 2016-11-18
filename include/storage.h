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

// Identifier of data type to be stored
#define HAL_STORAGE_ID_UUID	0
#define HAL_STORAGE_ID_TOKEN	1
#define HAL_STORAGE_ID_MAC	2
#define HAL_STORAGE_ID_CONFIG	3

size_t hal_storage_read(uint16_t addr, uint8_t *value, size_t len);
size_t hal_storage_write(uint16_t addr, const uint8_t *value, size_t len);
size_t hal_storage_write_end(uint8_t id, void *value, size_t len);
size_t hal_storage_read_end(uint8_t id, void *value, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_H__ */
