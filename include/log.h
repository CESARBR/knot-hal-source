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

void logger(const char *file, const char *function, long line,
				const char *args, const char *category);

#define hal_log_info(...) {						\
	logger(__FILE__, __func__, __LINE__, __VA_ARGS__, "[info] ");	\
}

#define hal_log_warn(...) {						\
	logger(__FILE__, __func__, __LINE__, __VA_ARGS__, "[warn] ");	\
}

#define hal_log_error(...) {						\
	logger(__FILE__, __func__, __LINE__, __VA_ARGS__, "[error] ");	\
}

void hal_log_close(void);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_LOG_H__ */
