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
#ifndef __KERNEL_EVENT_H
#define __KERNEL_EVENT_H

#include <kernel/thread.h>

#define EVENT_MAGIC 'evnt'

typedef struct event {
	int magic;
	bool signalled;
	uint flags;
	wait_queue_t wait;
} event_t;

#define EVENT_FLAG_AUTOUNSIGNAL 1

void event_init(event_t *, bool initial, uint flags);
void event_destroy(event_t *);
status_t event_wait(event_t *);
status_t event_wait_timeout(event_t *, time_t); /* wait on the event with a timeout */
status_t event_signal(event_t *, bool reschedule);
status_t event_unsignal(event_t *);

#endif

