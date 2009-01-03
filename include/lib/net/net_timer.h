/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
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
#ifndef _NEWOS_KERNEL_NET_NET_TIMER_H
#define _NEWOS_KERNEL_NET_NET_TIMER_H

#include <kernel/kernel.h>
#include <kernel/lock.h>

typedef void (*net_timer_callback)(void *);

typedef struct net_timer_event {
	struct net_timer_event *next;
	struct net_timer_event *prev;

	net_timer_callback func;
	void *args;

	bigtime_t sched_time;

	bool pending;
} net_timer_event;

#define NET_TIMER_PENDING_IGNORE 0x1

int net_timer_init(void);

int set_net_timer(net_timer_event *e, unsigned int delay_ms, net_timer_callback callback, void *args, int flags);
int cancel_net_timer(net_timer_event *e);

void clear_net_timer(net_timer_event *e);
extern inline void clear_net_timer(net_timer_event *e)
{
	e->prev = e->next = NULL;
	e->pending = false;
}

#endif

