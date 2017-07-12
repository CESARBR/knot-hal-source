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

#if (KNOT_DEBUG_ENABLED == 1)

int _hal_log_open(const char *pathname);
void _hal_log_str(const char *str);
void _hal_log_int(int value);
void _hal_log_long(long value);
void _hal_log_double(double value);
void _hal_log_hex(int value);
void _hal_log_close(void);

#define hal_log_open(p) _hal_log_open(p)
#define hal_log_str(x) _hal_log_str(x)
#define hal_log_int(x) _hal_log_int(x)
#define hal_log_long(x) _hal_log_long(x)
#define hal_log_double(x) _hal_log_double(x)
#define hal_log_hex(x) _hal_log_hex(x)
#define hal_log_close() _hal_log_close()

#else

#define hal_log_open(p) return 0
#define hal_log_str(x)
#define hal_log_int(x)
#define hal_log_long(x)
#define hal_log_double(x)
#define hal_log_hex(x)
#define hal_log_close()

#endif

#ifdef __cplusplus
}
#endif

#endif /* __HAL_LOG_H__ */
