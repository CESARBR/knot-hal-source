/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifndef __HAL_AVR_ERRNO_H__
#define __HAL_AVR_ERRNO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define	EINVAL			22	/* Invalid argument */
#define EAGAIN			11	/* Resource temporarily unavailable */
#define EBADMSG			74	/* Not a data message */
#define EILSEQ			84	/* Illegal byte sequence */
#define ENOSYS			38	/*  Function not implemented */
#define EPERM			01	/* Operation not permitted */
#define EUSERS			87	/* Too many users */
#define EBUSY			16	/* Device or resource busy */
#define ETIMEDOUT		110 /* Connection timed out */

#ifdef __cplusplus
}
#endif

#endif /* __HAL_AVR_ERRNO_H__ */
