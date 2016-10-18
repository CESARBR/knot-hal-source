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
				const char *args, const char *category)
{
	int len = strlen(file) + strlen(function) + strlen(args) +
					strlen(category) + SIZE_CHAR_BUF + 1;
	char buf[len];

	snprintf(buf, len, "%s%s::%s(%lu):%s\n", category, file, function,
								line, args);

	write(fd, buf, strlen(buf));
}

void hal_log_close(void)
{
	close(fd);
}
