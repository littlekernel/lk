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
#ifndef _LIB_NET_IF_H
#define _LIB_NET_IF_H

#include <sys/types.h>
#include <lib/net.h>
#include <lib/net/cbuf.h>
#include <lib/net/queue.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/event.h>

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

typedef struct ifhook {
	int type;
	size_t mtu;
	netaddr linkaddr;
	void *cookie;
	int (*if_input)(void *cookie, void *buf, size_t len);
	int (*if_output)(void *cookie, const void *buf, size_t len);
} ifhook;

typedef int if_id;

typedef struct ifnet {
	struct ifnet *next;
	const ifhook *hook;
	if_id id;
	int type;
	thread_t *rx_thread;
	thread_t *tx_thread;
	ifaddr *addr_list;
	ifaddr *link_addr;
	int (*link_input)(cbuf *buf, struct ifnet *i);
	int (*link_output)(cbuf *buf, struct ifnet *i, netaddr *target, int protocol_type);
	event_t tx_queue_event;
	mutex_t tx_queue_lock;
	fixed_queue tx_queue;
	uint8_t tx_buf[2048];
	uint8_t rx_buf[2048];
} ifnet;

int if_init(void);
ifnet *if_id_to_ifnet(if_id id);
int if_register_interface(const ifhook *hook, ifnet **i);
void if_bind_address(ifnet *i, ifaddr *addr);
void if_bind_link_address(ifnet *i, ifaddr *addr);
int if_boot_interface(ifnet *i);
int if_output(cbuf *b, ifnet *i);

#endif

