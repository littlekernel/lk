/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* The minip stack worker: one thread that runs all receive processing and
 * all network timers. Drivers hand packets in with minip_rx_pktbuf() (an
 * IRQ safe, non blocking ownership transfer) or the copying variants; the
 * worker drains the input queue and dispatches into the stack, so protocol
 * code never runs in interrupt context and drivers need no threads of
 * their own.
 *
 * The worker is deliberately an instantiable struct: today one instance
 * serves every interface, but nothing in the contract prevents per netif
 * workers later. Locks protecting protocol state remain in place;
 * correctness does not depend on there being a single worker.
 */

#include "minip-internal.h"

#include <lk/trace.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/compiler.h>
#include <stdlib.h>
#include <lk/list.h>
#include <sys/types.h>
#include <kernel/event.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <platform.h>

#define LOCAL_TRACE 0

struct netstack_worker {
    struct list_node rx_queue;  // pktbufs in flight to the stack, via pktbuf.list
    spin_lock_t lock;
    event_t event;              // autounsignal; kicked by rx enqueue and timer arming
    thread_t *thread;
};

static struct netstack_worker main_worker;

/* --- net timers, run on the worker thread --- */

static struct list_node net_timer_list = LIST_INITIAL_VALUE(net_timer_list);
static mutex_t net_timer_lock = MUTEX_INITIAL_VALUE(net_timer_lock);

static void add_to_queue(net_timer_t *t) {
    net_timer_t *e;
    list_for_every_entry(&net_timer_list, e, net_timer_t, node) {
        if (TIME_GT(e->sched_time, t->sched_time)) {
            list_add_before(&e->node, &t->node);
            return;
        }
    }

    list_add_tail(&net_timer_list, &t->node);
}

bool net_timer_set(net_timer_t *t, net_timer_callback_t cb, void *callback_args, lk_time_t delay) {
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

    event_signal(&main_worker.event, false);

    return newly_queued;
}

bool net_timer_cancel(net_timer_t *t) {
    bool was_queued = false;

    mutex_acquire(&net_timer_lock);

    if (list_in_list(&t->node)) {
        list_delete(&t->node);
        was_queued = true;
    }

    mutex_release(&net_timer_lock);

    return was_queued;
}

/* run expired timers; returns the delay until the next pending one */
static lk_time_t net_timer_work_routine(void) {
    lk_time_t now = current_time();
    lk_time_t delay;

    mutex_acquire(&net_timer_lock);

    for (;;) {
        net_timer_t *e;
        e = list_peek_head_type(&net_timer_list, net_timer_t, node);
        if (!e) {
            delay = INFINITE_TIME;
            break;
        }

        if (TIME_GT(e->sched_time, now)) {
            delay = e->sched_time - now;
            break;
        }

        list_delete(&e->node);

        mutex_release(&net_timer_lock);

        LTRACEF("firing timer %p, cb %p, arg %p\n", e, e->cb, e->arg);
        e->cb(e->arg);

        mutex_acquire(&net_timer_lock);
    }

    mutex_release(&net_timer_lock);

    return delay;
}

/* --- receive input queue --- */

/* Hand a received frame to the stack. Transfers ownership of the pktbuf:
 * the driver must not touch it again in any way after this call. IRQ safe
 * and non blocking.
 */
void minip_rx_pktbuf(netif_t *netif, pktbuf_t *p) {
    DEBUG_ASSERT(netif);
    DEBUG_ASSERT(p);

    struct netstack_worker *w = &main_worker;

    p->netif = netif;

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&w->lock);
    list_add_tail(&w->rx_queue, &p->list);
    spin_unlock_irqrestore(&w->lock, state);

    event_signal(&w->event, false);
}

/* Hand a received frame to the stack by copying it into a pool pktbuf, for
 * drivers that cannot give their receive buffer away. IRQ safe and non
 * blocking; the frame is dropped if the pool is exhausted.
 */
status_t minip_rx_driver_callback_copy(netif_t *netif, const void *frame, size_t len) {
    DEBUG_ASSERT(netif);
    DEBUG_ASSERT(frame);

    if (len > PKTBUF_SIZE) {
        return ERR_TOO_BIG;
    }

    pktbuf_t *p = pktbuf_alloc_rx();
    if (!p) {
        return ERR_NO_MEMORY;
    }

    pktbuf_append_data(p, frame, len);
    minip_rx_pktbuf(netif, p);

    return NO_ERROR;
}

/* --- the worker itself --- */

static int netstack_worker_thread(void *arg) {
    struct netstack_worker *w = (struct netstack_worker *)arg;

    for (;;) {
        /* run any expired timers and learn how long until the next one */
        lk_time_t delay = net_timer_work_routine();

        /* wait for a kick (rx traffic or a newly armed timer) or the next deadline */
        event_wait_timeout(&w->event, delay);

        /* drain the receive queue */
        for (;;) {
            arch_interrupt_saved_state_t state = spin_lock_irqsave(&w->lock);
            pktbuf_t *p = list_remove_head_type(&w->rx_queue, pktbuf_t, list);
            spin_unlock_irqrestore(&w->lock, state);
            if (!p) {
                break;
            }

            if (!minip_rx_process(p->netif, p)) {
                pktbuf_free(p, true);
            }
        }
    }

    return 0;
}

/* True when called from the stack worker itself. Anything that would block
 * waiting on the stack (a resolver waiting for a reply, a blocking pktbuf
 * allocation) must refuse to run here: there would be no one left to
 * deliver what it is waiting for.
 */
bool netstack_is_stack_thread(void) {
    return get_current_thread() == main_worker.thread;
}

void netstack_init(void) {
    struct netstack_worker *w = &main_worker;

    list_initialize(&w->rx_queue);
    spin_lock_init(&w->lock);
    event_init(&w->event, false, EVENT_FLAG_AUTOUNSIGNAL);

    w->thread = thread_create("netstack", &netstack_worker_thread, w, HIGH_PRIORITY,
                              DEFAULT_STACK_SIZE);
    thread_detach_and_resume(w->thread);
}
