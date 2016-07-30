/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __KNOT_HAL_H__
#define __KNOT_HAL_H__

/*
 * Domain specifies a communication domain; this selects the
 * protocol family which will be used for communication
 */
#define PF_NRF24L01			1

/*
 * Type specifies the communication semantics.
 */
#define TYPE_SOCK_RAW			0

/*
 * Protocol specifies a particular protocol to be used with the
 * socket. It might be used to access signaling channels or
 * domain specific protocols.
 */
#define PROTO_NONE				0
#define PROTO_TCP_FWD			1 /* Forward over TCP */

int ksocket(int domain, int type, int protocol);

#endif /* __KNOT_HAL_H__ */
