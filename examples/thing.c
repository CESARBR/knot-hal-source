#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "include/nrf24.h"
#include "include/comm.h"
#include "include/time.h"

/* application packet size maximum */
#define PACKET_SIZE_MAX	128

#define MESSAGE "The quick brown fox jumps over the lazy dog"
#define MESSAGE_SIZE	(sizeof(MESSAGE)-1)

unsigned long start;
bool  connected;
char  msg[PACKET_SIZE_MAX],
		buffer[PACKET_SIZE_MAX+1];

uint64_t my_addr = 0xACDCDEAD98765490;

int main(void)
{
	int sock_srv,
		sock_raw,
		nbytes,
		nread,
		msg_count,
		inc,
		err;

	size_t size, count;

	err = hal_comm_init("NRF0");
	if (err < 0)
		printf("erro init radio\n");

	sock_srv = hal_comm_socket(HAL_COMM_PF_NRF24, HAL_COMM_PROTO_RAW);
	if (sock_srv < 0)
		printf("erro open sockt mgmt");

	printf("NRF24L01 loaded\n");

	for (size = 0, inc = MESSAGE_SIZE; size < sizeof(msg);) {
		if ((inc+size) > sizeof(msg))
			inc = sizeof(msg) - size;
		memcpy(msg+size, MESSAGE, inc);
		size += inc;
	}
	for (;;) {

		if (!connected) {
			hal_comm_listen(sock_srv);
			while (sock_raw <= 0)
				sock_raw = hal_comm_accept(sock_srv, &my_addr);

			printf("connected sock_raw=%d\n", sock_raw);
			count = 1;
			msg_count = 0;
			start = hal_time_ms() + 1000;
			connected = true;
		}

		if (hal_timeout(hal_time_ms(), start, 1000) > 0) {
			memcpy(buffer, msg, count);
			nbytes = hal_comm_write(sock_raw, buffer, count);
			if (nbytes > 0) {
				inc = (count == size) ? -1 :
						(count == 1 ? 1 : inc);
				count += inc;
				buffer[nbytes] = '\0';
				printf("TX:[%03d]: %d-'%s'\n", nbytes,
						++msg_count, buffer);
			}
			start = hal_time_ms();
		}

		nread = hal_comm_read(sock_raw, buffer, sizeof(buffer));
		if (nread > 0) {
			buffer[nread] = '\0';
			printf("RX:[%03d]: '%s'\n", nread, buffer);
		}
		nread = hal_comm_read(sock_srv, buffer, sizeof(buffer));
		if (nread > 0) {
			struct mgmt_nrf24_header *evt =
				(struct mgmt_nrf24_header *) buffer;
			struct mgmt_evt_nrf24_disconnected *evt_discon =
				(struct mgmt_evt_nrf24_disconnected *)
					evt->payload;

			switch (evt->opcode) {
			case MGMT_EVT_NRF24_DISCONNECTED:
				printf("evt->disconnect %llX\n",
					(long long)
					evt_discon->mac.address.uint64);
				hal_comm_close(sock_raw);
				sock_raw = -1;
				connected = false;
				break;

			default:
				break;
			}
		}
	}
}
