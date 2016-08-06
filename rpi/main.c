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
static gboolean data_watch(GIOChannel *io, GIOCondition cond,
						gpointer user_data)
{
	gsize rbytes;
	char buffer[1024];
	GIOStatus status;
	GError *gerr = NULL;

	/*
	 * Manages TCP data from knotd. All traffic(raw data)
	 * should be queued to be transferred over-the-air.
	 */

	if (cond & (G_IO_HUP | G_IO_ERR))
		return FALSE;

	memset(buffer, 0, sizeof(buffer));

	/* Incoming data through TCP socket */
	status = g_io_channel_read_chars(io, buffer, sizeof(buffer),
						 &rbytes, &gerr);
	if (status == G_IO_STATUS_ERROR) {
		printf("read(): %s\n", gerr->message);
		g_error_free(gerr);
		return FALSE;
	}

	if (rbytes == 0)
		return FALSE;

	printf("read(): %lu bytes\n", rbytes);

	return TRUE;
}

static gboolean accept_watch(GIOChannel *io, GIOCondition cond,
						gpointer user_data)
{
	GIOChannel *cli_io;
	GIOCondition cli_cond = G_IO_IN | G_IO_ERR | G_IO_HUP;
	struct sockaddr_in client;
	int svr_sk, cli_sk, err;
	socklen_t sklen;

	if (cond & (G_IO_HUP | G_IO_ERR))
		return FALSE;

	svr_sk = g_io_channel_unix_get_fd(io);

	sklen = sizeof(client);
	memset(&client, 0, sklen);

	cli_sk = accept(svr_sk, (struct sockaddr *) &client, &sklen);
	if (cli_sk == -1) {
		err = errno;
		printf("accept(): %s(%d)\n", strerror(err), err);
		return TRUE;
	}

	printf("Peer's IP address is: %s\n", inet_ntoa(client.sin_addr));

	cli_io = g_io_channel_unix_new(cli_sk);
	g_io_channel_set_close_on_unref(cli_io, TRUE);

	/* Ending 'NULL' for binary data */
	g_io_channel_set_encoding(cli_io, NULL, NULL);
	g_io_channel_set_buffered(cli_io, FALSE);

	/* TCP client handler: incoming data from nrfd */
	g_io_add_watch(cli_io, cli_cond, data_watch, NULL);

	g_io_channel_unref(cli_io);

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

	return sk;
}

static GOptionEntry options[] = {
	{ "port", 'p', 0, G_OPTION_ARG_INT, &opt_port, "port", "passthrough port" },
	{ NULL },
};

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	GIOChannel *tcp_io;
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP;
	int tcp_id, watch_id, sock;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		printf("Invalid arguments: %s\n", gerr->message);
		g_error_free(gerr);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}

	g_option_context_free(context);

	printf("RPi SPI passthrough over TCP\n");
	printf("\tRunning at port %u\n\n", opt_port);

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGPIPE, SIG_IGN);

	main_loop = g_main_loop_new(NULL, FALSE);

	/* Creates TCP server socket */
	sock = passthrough_init();
	if (sock < 0) {
		printf("init: %s(%d)\n", strerror(-sock), -sock);
		return -sock;
	}

	tcp_io = g_io_channel_unix_new(sock);
	g_io_channel_set_close_on_unref(tcp_io, TRUE);

	/* Incoming connection handler */
	tcp_id = g_io_add_watch(tcp_io, cond, accept_watch, NULL);

	watch_id = g_idle_add(idle_watch, NULL);

	g_main_loop_run(main_loop);

	/* Closing TCP socket */
	g_io_channel_unref(tcp_io);

	g_source_remove(tcp_id);
	g_source_remove(watch_id);

	g_main_loop_unref(main_loop);

	return 0;
}
