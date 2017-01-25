/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

void log_init(const char *ident, int detach);
void log_close(void);

void log_info(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_error(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_warn(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_dbg(const char *format, ...) __attribute__((format(printf, 1, 2)));
