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
#ifndef _NEWOS_KERNEL_NET_IF_H
#define _NEWOS_KERNEL_NET_IF_H

#include <newos/types.h>
#include <kernel/cbuf.h>
#include <kernel/queue.h>
#include <kernel/lock.h>
#include <kernel/net/net.h>
#include <newos/defines.h>

typedef struct ifaddr {
	struct ifaddr *next;
	struct ifnet *if_owner;
	netaddr addr;
	netaddr netmask;
	netaddr broadcast;
} ifaddr;

enum {
	IF_TYPE_NULL = 0,
	IF_TYPE_LOOPBACK,
	IF_TYPE_ETHERNET
};

typedef int if_id;

typedef struct ifnet {
	struct ifnet *next;
	if_id id;
	char path[SYS_MAX_PATH_LEN];
	int type;
	int fd;
	thread_id rx_thread;
	thread_id tx_thread;
	ifaddr *addr_list;
	ifaddr *link_addr;
	size_t mtu;
	int (*link_input)(cbuf *buf, struct ifnet *i);
	int (*link_output)(cbuf *buf, struct ifnet *i, netaddr *target, int protocol_type);
	sem_id tx_queue_sem;
	mutex tx_queue_lock;
	fixed_queue tx_queue;
	uint8 tx_buf[2048];
	uint8 rx_buf[2048];
} ifnet;

int if_init(void);
ifnet *if_id_to_ifnet(if_id id);
ifnet *if_path_to_ifnet(const char *path);
int if_register_interface(const char *path, ifnet **i);
void if_bind_address(ifnet *i, ifaddr *addr);
void if_bind_link_address(ifnet *i, ifaddr *addr);
int if_boot_interface(ifnet *i);
int if_output(cbuf *b, ifnet *i);

#endif

