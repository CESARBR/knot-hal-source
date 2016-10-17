/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <glib.h>

#include "manager.h"

static GMainLoop *main_loop;
static const char *opt_serial = NULL;
static gboolean opt_unix = FALSE;

static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);
}

static GOptionEntry options[] = {
	{ "serial", 's', 0, G_OPTION_ARG_STRING, &opt_serial,
					"serial", "Serial device" },
	{ "unix", 'u', 0, G_OPTION_ARG_NONE, &opt_unix,
		"Unix socket", "Enable unix socket clients" },
	{ NULL },
};

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	int err;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	g_option_context_free(context);

	if (!opt_unix && opt_serial == NULL) {
		printf("Missing arguments\n");
		return EXIT_FAILURE;
	}

	err = manager_start(opt_serial, opt_unix);
	if (err < 0)
		return EXIT_FAILURE;

	/* Set user id to nobody */
	setuid(65534);

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	main_loop = g_main_loop_new(NULL, FALSE);

	g_main_loop_run(main_loop);

	manager_stop();

	g_main_loop_unref(main_loop);

	return 0;
}

