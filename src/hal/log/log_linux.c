/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <syslog.h>
#include <stdarg.h>

#include "hal/linux_log.h"

void hal_log_error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_ERR, format, ap);
	va_end(ap);
}

void hal_log_warn(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_WARNING, format, ap);
	va_end(ap);
}

void hal_log_info(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_INFO, format, ap);
	va_end(ap);
}

void hal_log_dbg(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsyslog(LOG_DEBUG, format, ap);
	va_end(ap);
}

void hal_log_init(const char *ident, int detach)
{
	int option = LOG_NDELAY | LOG_PID | LOG_PERROR;

	openlog(ident, option, LOG_DAEMON);
}

void hal_log_close(void)
{
	closelog();
}
