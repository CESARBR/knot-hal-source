/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __HAL_COMM_H__
#define __HAL_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Domain: selects radio technology */
#define HAL_COMM_PF_NRF24		1
#define HAL_COMM_PF_SERIAL		2

#define HAL_COMM_PROTO_RAW		0 /* Raw data(User): serial/NRF24 */
#define HAL_COMM_PROTO_MGMT		1 /* Management: Commands and events */

/*
 * nRF24 and other radios. Returns -ENFILE (limit of resources/pipes has
 * been reached) or an id representing the logical communication channel
 * for a new client.
 * FIXME: MAC should be replaced by a generic structure since init is
 * common to any transport.
 */

int hal_comm_init(const char *pathname, const struct nrf24_mac *mac);
int hal_comm_deinit(void);

int hal_comm_socket(int domain, int protocol);

int hal_comm_close(int sockfd);

/* Non-blocking read operation. Returns -EGAIN if there isn't data available */
ssize_t hal_comm_read(int sockfd, void *buffer, size_t count);

/* Blocking write operation. Returns -EBADF if not connected */
ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count);

int hal_comm_listen(int sockfd);

/* Non-blocking operation. Returns -EGAIN if there isn't a new client */
int hal_comm_accept(int sockfd, void *addr);

/* Blocking operation. Returns -ETIMEOUT */
int hal_comm_connect(int sockfd, uint64_t *addr);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_COMM_H__ */
