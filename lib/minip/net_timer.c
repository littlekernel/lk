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
#include <stdlib.h>
#include <list.h>
#include <err.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <platform.h>

#define LOCAL_TRACE 0

static struct list_node net_timer_list = LIST_INITIAL_VALUE(net_timer_list);
static event_t net_timer_event = EVENT_INITIAL_VALUE(net_timer_event, false, 0);
static mutex_t net_timer_lock = MUTEX_INITIAL_VALUE(net_timer_lock);

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

bool net_timer_set(net_timer_t *t, net_timer_callback_t cb, void *callback_args, lk_time_t delay)
{
    bool newly_queued = true;

    lk_time_t now = current_time();

    mutex_acquire(&net_timer_lock);

    if (list_in_list(&t->node)) {
        list_delete(&t->node);
        newly_queued = false;
    }

    t->cb = cb;
    t->arg = callback_args;
    t->sched_time = now + delay;

    add_to_queue(t);

    mutex_release(&net_timer_lock);

    event_signal(&net_timer_event, true);

    return newly_queued;
}

bool net_timer_cancel(net_timer_t *t)
{
    bool was_queued = false;

    mutex_acquire(&net_timer_lock);

    if (list_in_list(&t->node)) {
        list_delete(&t->node);
        was_queued = true;
    }

    mutex_release(&net_timer_lock);

    return was_queued;
}

/* returns the delay to the next event */
static lk_time_t net_timer_work_routine(void)
{
    lk_time_t now = current_time();
    lk_time_t delay = INFINITE_TIME;

    mutex_acquire(&net_timer_lock);

    for (;;) {
        net_timer_t *e;
        e = list_peek_head_type(&net_timer_list, net_timer_t, node);
        if (!e) {
            delay = INFINITE_TIME;
            goto done;
        }

        if (TIME_GT(e->sched_time, now)) {
            delay = e->sched_time - now;
            goto done;
        }

        list_delete(&e->node);

        mutex_release(&net_timer_lock);

        LTRACEF("firing timer %p, cb %p, arg %p\n", e, e->cb, e->arg);
        e->cb(e->arg);

        mutex_acquire(&net_timer_lock);
    }

done:
    if (delay == INFINITE_TIME)
        event_unsignal(&net_timer_event);

    mutex_release(&net_timer_lock);

    return delay;
}

int net_timer_work_thread(void *args)
{
    for (;;) {
        event_wait(&net_timer_event);

        lk_time_t delay = net_timer_work_routine();
        if (delay != INFINITE_TIME) {
            thread_sleep(MIN(delay, 100));
        }
    }

    return 0;
}

void net_timer_init(void)
{
    thread_detach_and_resume(thread_create("net timer", &net_timer_work_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
}


