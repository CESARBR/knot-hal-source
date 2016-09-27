/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __HAL_TIME_H__
#define __HAL_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

uint32_t hal_time_ms(void);
uint32_t hal_time_us(void);
void hal_delay_ms(uint32_t ms);
void hal_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_TIME_H__ */
