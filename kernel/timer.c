/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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

/**
 * @file
 * @brief  Kernel timer subsystem
 * @defgroup timer Timers
 *
 * The timer subsystem allows functions to be scheduled for later
 * execution.  Each timer object is used to cause one function to
 * be executed at a later time.
 *
 * Timer callback functions are called in interrupt context.
 *
 * @{
 */
#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <list.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/debug.h>
#include <kernel/spinlock.h>
#include <platform/timer.h>
#include <platform.h>

#define LOCAL_TRACE 0

spin_lock_t timer_lock;

struct timer_state {
    struct list_node timer_queue;
} __CPU_ALIGN;

static struct timer_state timers[SMP_MAX_CPUS];

static enum handler_return timer_tick(void *arg, lk_time_t now);

/**
 * @brief  Initialize a timer object
 */
void timer_initialize(timer_t *timer)
{
    *timer = (timer_t)TIMER_INITIAL_VALUE(*timer);
}

static void insert_timer_in_queue(uint cpu, timer_t *timer)
{
    timer_t *entry;

    DEBUG_ASSERT(arch_ints_disabled());

    LTRACEF("timer %p, cpu %u, scheduled %u, periodic %u\n", timer, cpu, timer->scheduled_time, timer->periodic_time);

    list_for_every_entry(&timers[cpu].timer_queue, entry, timer_t, node) {
        if (TIME_GT(entry->scheduled_time, timer->scheduled_time)) {
            list_add_before(&entry->node, &timer->node);
            return;
        }
    }

    /* walked off the end of the list */
    list_add_tail(&timers[cpu].timer_queue, &timer->node);
}

static void timer_set(timer_t *timer, lk_time_t delay, lk_time_t period, timer_callback callback, void *arg)
{
    lk_time_t now;

    LTRACEF("timer %p, delay %u, period %u, callback %p, arg %p\n", timer, delay, period, callback, arg);

    DEBUG_ASSERT(timer->magic == TIMER_MAGIC);

    if (list_in_list(&timer->node)) {
        panic("timer %p already in list\n", timer);
    }

    now = current_time();
    timer->scheduled_time = now + delay;
    timer->periodic_time = period;
    timer->callback = callback;
    timer->arg = arg;

    LTRACEF("scheduled time %u\n", timer->scheduled_time);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&timer_lock, state);

    uint cpu = arch_curr_cpu_num();
    insert_timer_in_queue(cpu, timer);

#if PLATFORM_HAS_DYNAMIC_TIMER
    if (list_peek_head_type(&timers[cpu].timer_queue, timer_t, node) == timer) {
        /* we just modified the head of the timer queue */
        LTRACEF("setting new timer for %u msecs\n", delay);
        platform_set_oneshot_timer(timer_tick, NULL, delay);
    }
#endif

    spin_unlock_irqrestore(&timer_lock, state);
}

/**
 * @brief  Set up a timer that executes once
 *
 * This function specifies a callback function to be called after a specified
 * delay.  The function will be called one time.
 *
 * @param  timer The timer to use
 * @param  delay The delay, in ms, before the timer is executed
 * @param  callback  The function to call when the timer expires
 * @param  arg  The argument to pass to the callback
 *
 * The timer function is declared as:
 *   enum handler_return callback(struct timer *, lk_time_t now, void *arg) { ... }
 */
void timer_set_oneshot(timer_t *timer, lk_time_t delay, timer_callback callback, void *arg)
{
    if (delay == 0)
        delay = 1;
    timer_set(timer, delay, 0, callback, arg);
}

/**
 * @brief  Set up a timer that executes repeatedly
 *
 * This function specifies a callback function to be called after a specified
 * delay.  The function will be called repeatedly.
 *
 * @param  timer The timer to use
 * @param  delay The delay, in ms, before the timer is executed
 * @param  callback  The function to call when the timer expires
 * @param  arg  The argument to pass to the callback
 *
 * The timer function is declared as:
 *   enum handler_return callback(struct timer *, lk_time_t now, void *arg) { ... }
 */
void timer_set_periodic(timer_t *timer, lk_time_t period, timer_callback callback, void *arg)
{
    if (period == 0)
        period = 1;
    timer_set(timer, period, period, callback, arg);
}

/**
 * @brief  Cancel a pending timer
 */
void timer_cancel(timer_t *timer)
{
    DEBUG_ASSERT(timer->magic == TIMER_MAGIC);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&timer_lock, state);

#if PLATFORM_HAS_DYNAMIC_TIMER
    uint cpu = arch_curr_cpu_num();

    timer_t *oldhead = list_peek_head_type(&timers[cpu].timer_queue, timer_t, node);
#endif

    if (list_in_list(&timer->node))
        list_delete(&timer->node);

    /* to keep it from being reinserted into the queue if called from
     * periodic timer callback.
     */
    timer->periodic_time = 0;
    timer->callback = NULL;
    timer->arg = NULL;

#if PLATFORM_HAS_DYNAMIC_TIMER
    /* see if we've just modified the head of the timer queue */
    timer_t *newhead = list_peek_head_type(&timers[cpu].timer_queue, timer_t, node);
    if (newhead == NULL) {
        LTRACEF("clearing old hw timer, nothing in the queue\n");
        platform_stop_timer();
    } else if (newhead != oldhead) {
        lk_time_t delay;
        lk_time_t now = current_time();

        if (TIME_LT(newhead->scheduled_time, now))
            delay = 0;
        else
            delay = newhead->scheduled_time - now;

        LTRACEF("setting new timer to %u\n", (uint) delay);
        platform_set_oneshot_timer(timer_tick, NULL, delay);
    }
#endif

    spin_unlock_irqrestore(&timer_lock, state);
}

/* called at interrupt time to process any pending timers */
static enum handler_return timer_tick(void *arg, lk_time_t now)
{
    timer_t *timer;
    enum handler_return ret = INT_NO_RESCHEDULE;

    DEBUG_ASSERT(arch_ints_disabled());

    THREAD_STATS_INC(timer_ints);
//  KEVLOG_TIMER_TICK(); // enable only if necessary

    uint cpu = arch_curr_cpu_num();

    LTRACEF("cpu %u now %u, sp %p\n", cpu, now, __GET_FRAME());

    spin_lock(&timer_lock);

    for (;;) {
        /* see if there's an event to process */
        timer = list_peek_head_type(&timers[cpu].timer_queue, timer_t, node);
        if (likely(timer == 0))
            break;
        LTRACEF("next item on timer queue %p at %u now %u (%p, arg %p)\n", timer, timer->scheduled_time, now, timer->callback, timer->arg);
        if (likely(TIME_LT(now, timer->scheduled_time)))
            break;

        /* process it */
        LTRACEF("timer %p\n", timer);
        DEBUG_ASSERT(timer && timer->magic == TIMER_MAGIC);
        list_delete(&timer->node);

        /* we pulled it off the list, release the list lock to handle it */
        spin_unlock(&timer_lock);

        LTRACEF("dequeued timer %p, scheduled %u periodic %u\n", timer, timer->scheduled_time, timer->periodic_time);

        THREAD_STATS_INC(timers);

        bool periodic = timer->periodic_time > 0;

        LTRACEF("timer %p firing callback %p, arg %p\n", timer, timer->callback, timer->arg);
        KEVLOG_TIMER_CALL(timer->callback, timer->arg);
        if (timer->callback(timer, now, timer->arg) == INT_RESCHEDULE)
            ret = INT_RESCHEDULE;

        /* it may have been requeued or periodic, grab the lock so we can safely inspect it */
        spin_lock(&timer_lock);

        /* if it was a periodic timer and it hasn't been requeued
         * by the callback put it back in the list
         */
        if (periodic && !list_in_list(&timer->node) && timer->periodic_time > 0) {
            LTRACEF("periodic timer, period %u\n", timer->periodic_time);
            timer->scheduled_time = now + timer->periodic_time;
            insert_timer_in_queue(cpu, timer);
        }
    }

#if PLATFORM_HAS_DYNAMIC_TIMER
    /* reset the timer to the next event */
    timer = list_peek_head_type(&timers[cpu].timer_queue, timer_t, node);
    if (timer) {
        /* has to be the case or it would have fired already */
        DEBUG_ASSERT(TIME_GT(timer->scheduled_time, now));

        lk_time_t delay = timer->scheduled_time - now;

        LTRACEF("setting new timer for %u msecs for event %p\n", (uint)delay, timer);
        platform_set_oneshot_timer(timer_tick, NULL, delay);
    }

    /* we're done manipulating the timer queue */
    spin_unlock(&timer_lock);
#else
    /* release the timer lock before calling the tick handler */
    spin_unlock(&timer_lock);

    /* let the scheduler have a shot to do quantum expiration, etc */
    /* in case of dynamic timer, the scheduler will set up a periodic timer */
    if (thread_timer_tick() == INT_RESCHEDULE)
        ret = INT_RESCHEDULE;
#endif

    return ret;
}

void timer_init(void)
{
    timer_lock = SPIN_LOCK_INITIAL_VALUE;
    for (uint i = 0; i < SMP_MAX_CPUS; i++) {
        list_initialize(&timers[i].timer_queue);
    }
#if !PLATFORM_HAS_DYNAMIC_TIMER
    /* register for a periodic timer tick */
    platform_set_periodic_timer(timer_tick, NULL, 10); /* 10ms */
#endif
}
