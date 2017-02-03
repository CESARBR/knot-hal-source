/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

void hal_log_init(const char *ident, int detach);
void hal_log_close(void);

void hal_log_info(const char *format, ...) __attribute__((format(printf, 1, 2)));
void hal_log_error(const char *format, ...) __attribute__((format(printf, 1, 2)));
void hal_log_warn(const char *format, ...) __attribute__((format(printf, 1, 2)));
void hal_log_dbg(const char *format, ...) __attribute__((format(printf, 1, 2)));
