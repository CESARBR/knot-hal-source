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
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <glib.h>

#include "spi.h"
#include "nrf24l01.h"

#define NRF24L01_NO_PIPE		0b111
static GMainLoop *main_loop;

int esperado_par = 2;
int esperado_impar = 3;
int qtd_errado;
int qtd_certo;
char buffer[128];
int q = 1;
static void sig_term(int sig)
{
	nrf24l01_deinit();
	g_main_loop_quit(main_loop);
	q = 0;
}

static gboolean timeout_watch(gpointer user_data)
{
	int index;
	int32_t buf;
	uint16_t rxlen;
	int len;
	int pipe;
	 // while(q){
		for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
				len = nrf24l01_prx_data(&buf, sizeof(buf));
				if (pipe == 1){
					if (buf == esperado_par){
						printf("pipe %d ----- rcv: %d OK\n",pipe, buf);
					}
					else {

						printf("pipe %d ----- rcv: %d ----- esperado %d\n",pipe, buf,esperado_par);
					}
					esperado_par = buf+2;
				}
				else{
					if (buf == esperado_impar){
						printf("pipe %d ----- rcv: %d OK\n",pipe, buf);
					}
					else {

						printf("pipe %d ----- rcv: %d ----- esperado %d\n",pipe, buf,esperado_impar);
					}
					esperado_impar = buf+2;
					}
		 }
	// }

	return TRUE;
}


static int radio_init(void)
{

	printf("call spi init\n");


	nrf24l01_init("/dev/spidev0.0");
	nrf24l01_set_channel(10);
	nrf24l01_set_standby();
	nrf24l01_open_pipe(0,0);
	nrf24l01_open_pipe(1,1);
	nrf24l01_open_pipe(2,2);
	printf("set PRX\n");
	nrf24l01_set_prx();

	return 0;
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

	err = radio_init();
	if (err < 0) {
		g_main_loop_unref(main_loop);
		return -1;
	}

	g_main_loop_run(main_loop);

	g_source_remove(timeout_id);

	g_main_loop_unref(main_loop);

	return 0;
}

