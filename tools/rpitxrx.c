/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

/*
 * Build instructions while a Makefile is not available:
 * gcc $(pkg-config --libs --cflags glib-2.0) -Iinclude -Itools \
 * tools/rpitxrx.c -o tools/rpitxrxd
 */

#include <errno.h>
#include <stdio.h>

#include <glib.h>

static GMainLoop *main_loop;

static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);
}

static gboolean timeout_watch(gpointer user_data)
{
	/*
	 * This function gets called frequently to verify
	 * if there is SPI/nRF24L01 data to be read or
	 * written. Later GPIO sys interface can be used
	 * to avoid busy loop.
	 */

	return TRUE;
}

int main(int argc, char *argv[])
{
	int err, timeout_id;

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	main_loop = g_main_loop_new(NULL, FALSE);

	printf("RPi nRF24L01 Radio test tool\n");

	timeout_id = g_timeout_add_seconds(1, timeout_watch, NULL);

	g_main_loop_run(main_loop);

	g_source_remove(timeout_id);

	g_main_loop_unref(main_loop);

	return 0;
}
