/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#include <glib.h>

static GMainLoop *main_loop;

static unsigned int opt_port = 9000;

static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);
}

static gboolean idle_watch(gpointer user_data)
{
	/*
	 * This function gets called frequently to verify
	 * if there is SPI data to be read. Later GPIO
	 * sys interface can be used to avoid busy loop.
	 */
	return TRUE;
}

static int passthrough_init(void)
{
	struct sockaddr_in server;
	int sk;

	/* Create the TCP socket */
	sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sk < 0) {
		printf("Failed to create TCP socket\n");
		return -errno;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(opt_port);

	/* Bind the server socket */
	if (bind(sk, (struct sockaddr *) &server, sizeof(server)) < 0) {
		close(sk);
		return -errno;
	}

	/* Listen on the server socket */
	if (listen(sk, 1) < 0) {
		close(sk);
		return -errno;
	}

	return 0;
}

static GOptionEntry options[] = {
	{ "port", 'p', 0, G_OPTION_ARG_INT, &opt_port, "port", "passthrough port" },
	{ NULL },
};

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	int err, watch_id;

	printf("RPi SPI passthrough over TCP\n");

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	g_option_context_free(context);

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	main_loop = g_main_loop_new(NULL, FALSE);

	err = passthrough_init();
	if (err < 0) {
		printf("init: %s(%d)\n", strerror(-err), -err);
		return -err;
	}

	watch_id = g_idle_add(idle_watch, NULL);

	g_main_loop_run(main_loop);

	g_source_remove(watch_id);

	g_main_loop_unref(main_loop);

	return 0;
}
