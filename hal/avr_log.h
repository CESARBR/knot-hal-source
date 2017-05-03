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

void logger(PGM_P category, const char *format, ...);

extern const char PROGMEM libInfoString[];
extern const char PROGMEM libWarnString[];
extern const char PROGMEM libErrorString[];

#define hal_log_info(format, ...)					\
	logger(libInfoString, format, __VA_ARGS__)

#define hal_log_warn(format, ...)					\
	logger(libWarnString, format, __VA_ARGS__)

#define hal_log_error(format, ...)					\
	logger(libErrorString, format, __VA_ARGS__)

void hal_log_close(void);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_LOG_H__ */
