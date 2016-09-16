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

int hal_storage_read(uint16_t addr, uint8_t *value, uint16_t len);
int hal_storage_write(uint16_t addr, const uint8_t *value, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_H__ */
