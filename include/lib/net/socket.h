/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _LIB_NET_SOCKET_H
#define _LIB_NET_SOCKET_H

#include <stdint.h>
#include <lib/net.h>

typedef int32_t sock_id;

int socket_init(void);
sock_id socket_create(int type, int flags);
int socket_bind(sock_id id, sockaddr *addr);
int socket_connect(sock_id id, sockaddr *addr);
int socket_listen(sock_id fd);
int socket_accept(sock_id fd, sockaddr *addr);
ssize_t socket_read(sock_id id, void *buf, ssize_t len);
ssize_t socket_write(sock_id id, const void *buf, ssize_t len);
ssize_t socket_recvfrom(sock_id id, void *buf, ssize_t len, sockaddr *addr);
ssize_t socket_recvfrom_etc(sock_id id, void *buf, ssize_t len, sockaddr *addr, int flags, bigtime_t timeout);
ssize_t socket_sendto(sock_id id, const void *buf, ssize_t len, sockaddr *addr);
int socket_close(sock_id id);

int socket_dev_init(void);

#endif

