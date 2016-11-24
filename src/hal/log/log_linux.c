/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>

#include "include/log.h"

#define SIZE_CHAR_BUF 10

static int fd;

int hal_log_open(const char *pathname)
{
	setlocale(LC_ALL, "");

	fd = open(pathname, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR |
					S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd == -1)
		return -errno;

	return 0;
}

void logger(const char *file, const char *function, long line,
		const char *category, const char *format, ...)
{
	char buf[1024] = {0};
	va_list args;
	int len = 0;

	len = snprintf(buf, 1024, "%s%s::%s(%lu):", category, file, function,
								line);

	va_start(args, format);
	len += vsnprintf(&(buf[len]), 1024-len, format, args);
	va_end(args);

	len += snprintf(&(buf[len]), 1024-len, "\n");

	write(fd, buf, len);
}

void hal_log_close(void)
{
	close(fd);
}
