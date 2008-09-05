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
#include <list.h>
#include <malloc.h>
#include <err.h>
#include <kernel/dpc.h>
#include <kernel/thread.h>
#include <kernel/event.h>

struct dpc {
	struct list_node node;	

	dpc_callback cb;
	void *arg;
};

static struct list_node dpc_list = LIST_INITIAL_VALUE(dpc_list);
static event_t dpc_event;

static int dpc_thread_routine(void *arg);

void dpc_init(void)
{
	event_init(&dpc_event, false, 0);

	thread_resume(thread_create("dpc", &dpc_thread_routine, NULL, DPC_PRIORITY, DEFAULT_STACK_SIZE));
}

status_t dpc_queue(dpc_callback cb, void *arg, uint flags)
{
	struct dpc *dpc;

	dpc = malloc(sizeof(struct dpc));

	dpc->cb = cb;
	dpc->arg = arg;
	enter_critical_section();
	list_add_tail(&dpc_list, &dpc->node);
	event_signal(&dpc_event, (flags & DPC_FLAG_NORESCHED) ? false : true);
	exit_critical_section();

	return NO_ERROR;
}

static int dpc_thread_routine(void *arg)
{
	for (;;) {
		event_wait(&dpc_event);

		enter_critical_section();
		struct dpc *dpc = list_remove_head_type(&dpc_list, struct dpc, node);
		if (!dpc)
			event_unsignal(&dpc_event);
		exit_critical_section();

		if (dpc) {
//			dprintf("dpc calling %p, arg %p\n", dpc->cb, dpc->arg);
			dpc->cb(dpc->arg);

			free(dpc);
		}
	}

	return 0;
}


