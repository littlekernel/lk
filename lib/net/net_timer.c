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
#include <debug.h>
#include <string.h>
#include <err.h>
#include <list.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/event.h>
#include <lib/net/net_timer.h>
#include <platform.h>

#define NET_TIMER_INTERVAL 200 // 200 ms

typedef struct {
	struct list_node list;

	mutex_t lock;

	thread_t *runner_thread;
} net_timer_queue;

static net_timer_queue net_q;

static void add_to_queue(net_timer_event *e)
{
	net_timer_event *tmp;

	list_for_every_entry(&net_q.list, tmp, net_timer_event, node) {
		if(tmp->sched_time > e->sched_time) {
			// put it here
			list_add_before(&e->node, &tmp->node);
			return;
		}
	}

	// walked off the end of the list
	list_add_tail(&net_q.list, &e->node);
}

int set_net_timer(net_timer_event *e, unsigned int delay_ms, net_timer_callback callback, void *args, int flags)
{
	int err = NO_ERROR;

	mutex_acquire(&net_q.lock);

	if(e->pending) {
		if(flags & NET_TIMER_PENDING_IGNORE) {
			err = ERROR;
			goto out;
		}
		cancel_net_timer(e);
	}

	// set up the timer
	e->func = callback;
	e->args = args;
	e->sched_time = current_time() + delay_ms;
	e->pending = true;

	add_to_queue(e);

out:
	mutex_release(&net_q.lock);

	return err;
}

int cancel_net_timer(net_timer_event *e)
{
	int err = NO_ERROR;

	mutex_acquire(&net_q.lock);

	if(!e->pending) {
		err = ERROR;
		goto out;
	}

	list_delete(&e->node);
	e->pending = false;

out:
	mutex_release(&net_q.lock);

	return err;
}

static int net_timer_runner(void *arg)
{
	net_timer_event *e;
	bigtime_t now;

	for(;;) {
		thread_sleep(NET_TIMER_INTERVAL);

		now = current_time();

retry:
		mutex_acquire(&net_q.lock);

		// pull off the head of the list and run it, if it timed out
		if((e = list_peek_head_type(&net_q.list, net_timer_event, node)) != NULL && e->sched_time <= now) {

			list_delete(&e->node);
			e->pending = false;

			mutex_release(&net_q.lock);

			e->func(e->args);

			// Since we ran an event, loop back and check the head of 
			// the list again, because the list may have changed while
			// inside the callback.
			goto retry;
			
		} else {
			mutex_release(&net_q.lock);
		}
	}

	return 0;
}

int net_timer_init(void)
{
	list_initialize(&net_q.list);

	mutex_init(&net_q.lock);

	net_q.runner_thread = thread_create("net timer runner", &net_timer_runner, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
	thread_resume(net_q.runner_thread);

	return 0;
}

