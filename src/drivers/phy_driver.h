/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifdef __cplusplus
extern "C"{
#endif

int phy_open(const char *pathname);
int phy_close(int sockfd);
ssize_t phy_read(int sockfd, void *buffer, size_t len);
ssize_t phy_write(int sockfd, const void *buffer, size_t len);
int phy_ioctl(int sockfd, int cmd, void *arg);

#ifdef __cplusplus
} // extern "C"
#endif
