/*
 * Copyright (c) 2014 Travis Geiselbrecht
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

#include "minip-internal.h"

#include <trace.h>
#include <debug.h>
#include <compiler.h>
#include <list.h>
#include <err.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>

#define LOCAL_TRACE 1

static struct list_node net_timer_list = LIST_INITIAL_VALUE(net_timer_list);

static void add_to_queue(net_timer_t *t)
{
    net_timer_t *e;
    list_for_every_entry(&net_timer_list, e, net_timer_t, node) {
        if (TIME_GT(e->sched_time, t->sched_time)) {
            list_add_before(&e->node, &t->node);
            return;
        }
    }

    list_add_tail(&net_timer_list, &t->node);
}

status_t net_timer_set(net_timer_t *t, net_timer_callback_t cb, void *callback_args, lk_time_t delay)
{
    enter_critical_section();

    if (list_in_list(&t->node)) {
        list_delete(&t->node);
    }

    t->cb = cb;
    t->arg = callback_args;
    t->sched_time = current_time() + delay;

    add_to_queue(t);

    exit_critical_section();

    return NO_ERROR;
}


status_t net_timer_cancel(net_timer_t *t)
{
    enter_critical_section();

    if (list_in_list(&t->node)) {
        list_delete(&t->node);
    }

    exit_critical_section();

    return NO_ERROR;
}

static void net_timer_work_routine(void)
{
    enter_critical_section();

    for (;;) {
        net_timer_t *e;
        e = list_peek_head_type(&net_timer_list, net_timer_t, node);
        if (!e)
            goto done;

        if (TIME_GT(e->sched_time, current_time()))
            goto done;

        list_delete(&e->node);

        exit_critical_section();

        e->cb(e->arg);

        enter_critical_section();
    }

done:
    exit_critical_section();
}

int net_timer_work_thread(void *args)
{
    for (;;) {
        // XXX be smarter
        thread_sleep(100);

        net_timer_work_routine();
    }

    return 0;
}

void net_timer_init(void)
{
    thread_detach_and_resume(thread_create("net timer", &net_timer_work_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
}


