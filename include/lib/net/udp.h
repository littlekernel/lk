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
#ifndef _LIB_NET_UDP_H
#define _LIB_NET_UDP_H

#include <lib/net/if.h>
#include <lib/net/ipv4.h>
#include <lib/net/socket.h>
#include <lib/net/cbuf.h>

int udp_input(cbuf *buf, ifnet *i, ipv4_addr source_address, ipv4_addr target_address);
int udp_open(void **prot_data);
int udp_bind(void *prot_data, sockaddr *addr);
int udp_connect(void *prot_data, sockaddr *addr);
int udp_listen(void *prot_data);
int udp_accept(void *prot_data, sockaddr *addr, void **new_socket);
int udp_close(void *prot_data);
ssize_t udp_recvfrom(void *prot_data, void *buf, ssize_t len, sockaddr *saddr, int flags, time_t timeout);
ssize_t udp_sendto(void *prot_data, const void *buf, ssize_t len, sockaddr *addr);
int udp_init(void);

#endif

