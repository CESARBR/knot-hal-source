/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __HAL_LOG_H__
#define __HAL_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

int hal_log_open(const char *pathname);

void hal_log_str(const char *str);
void hal_log_int(int value);
void hal_log_long(long value);
void hal_log_double(double value);
void hal_log_hex(int value);

void hal_log_close(void);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_LOG_H__ */
