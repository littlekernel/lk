/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/**
 * @file
 * @brief  Wait queues
 *
 * The wait queue is the blocking primitive every other synchronization object in
 * the kernel is built out of: mutexes, events, semaphores and thread_join all
 * reduce to putting a thread on one of these and taking it back off.
 *
 * Everything here currently runs under the global thread lock, which is also
 * what makes the hand-off to the scheduler safe: a thread taken off a wait queue
 * is handed straight to sched_insert_runnable() without the lock being dropped
 * in between.
 *
 * @defgroup  wait  Wait Queue
 * @{
 */
#include <kernel/wait.h>

#include <assert.h>
#include <kernel/preempt.h>
#include <kernel/sched.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>

void wait_queue_init(wait_queue_t *wait) {
    *wait = (wait_queue_t)WAIT_QUEUE_INITIAL_VALUE(*wait);
}

static enum handler_return wait_queue_timeout_handler(timer_t *timer, lk_time_t now, void *arg) {
    thread_t *thread = (thread_t *)arg;

    DEBUG_ASSERT(thread->magic == THREAD_MAGIC);

    spin_lock(&thread_lock);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (thread_unblock_from_wait_queue(thread, ERR_TIMED_OUT) >= NO_ERROR) {
        ret = INT_RESCHEDULE;
        if (preempt_set_pending_if_disabled()) {
            // If preemption was disabled, we can't reschedule now.
            ret = INT_NO_RESCHEDULE;
        }
    }

    spin_unlock(&thread_lock);

    return ret;
}

/**
 * @brief  Block until a wait queue is notified.
 *
 * This function puts the current thread at the end of a wait
 * queue and then blocks until some other thread wakes the queue
 * up again.
 *
 * @param  wait     The wait queue to enter
 * @param  timeout  The maximum time, in ms, to wait
 *
 * If the timeout is zero, this function returns immediately with
 * ERR_TIMED_OUT.  If the timeout is INFINITE_TIME, this function
 * waits indefinitely.  Otherwise, this function returns with
 * ERR_TIMED_OUT at the end of the timeout period.
 *
 * @return ERR_TIMED_OUT on timeout, else returns the return
 * value specified when the queue was woken by wait_queue_wake_one().
 */
status_t wait_queue_block(wait_queue_t *wait, lk_time_t timeout) {
    timer_t timer;

    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    if (timeout == 0) {
        return ERR_TIMED_OUT;
    }

    list_add_tail(&wait->list, &current_thread->queue_node);
    wait->count++;
    current_thread->state = THREAD_BLOCKED;
    current_thread->blocking_wait_queue = wait;
    current_thread->wait_queue_block_ret = NO_ERROR;

    /* if the timeout is nonzero or noninfinite, set a callback to yank us out of the queue */
    if (timeout != INFINITE_TIME) {
        timer_initialize(&timer);
        timer_set_oneshot(&timer, timeout, wait_queue_timeout_handler, (void *)current_thread);
    }

    sched_resched();

    /* we don't really know if the timer fired or not, so it's better safe to try to cancel it */
    if (timeout != INFINITE_TIME) {
        timer_cancel(&timer);
    }

    return current_thread->wait_queue_block_ret;
}

/**
 * @brief  Wake up one thread sleeping on a wait queue
 *
 * This function removes one thread (if any) from the head of the wait queue and
 * makes it executable.  The new thread will be placed at the head of the
 * run queue.
 *
 * @param wait  The wait queue to wake
 * @param wait_queue_error  The return value which the new thread will receive
 * from wait_queue_block().
 *
 * @return  The number of threads woken (zero or one)
 */
int wait_queue_wake_one(wait_queue_t *wait, status_t wait_queue_error) {
    thread_t *t;
    int ret = 0;

    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    t = list_remove_head_type(&wait->list, thread_t, queue_node);
    if (t) {
        wait->count--;
        DEBUG_ASSERT(t->state == THREAD_BLOCKED);
        t->state = THREAD_READY;
        t->wait_queue_block_ret = wait_queue_error;
        t->blocking_wait_queue = NULL;

        /* If preemption is disabled -- by an interrupt handler, or by a caller
         * batching a run of wakeups -- just record that a reschedule is owed and
         * let whoever reenables preemption take it. Otherwise switch now.
         */
        const bool resched_now = !preempt_set_pending_if_disabled();

        /* if we're rescheduling, stick the current thread on the head
         * of the run queue first, so that the newly awakened thread gets a chance to run
         * before the current one, but the current one doesn't get unnecessarily punished.
         */
        if (resched_now) {
            current_thread->state = THREAD_READY;
            sched_insert_runnable_head_on(arch_curr_cpu_num(), current_thread);
        }
        sched_poke_cpus(1U << sched_insert_runnable(t));
        if (resched_now) {
            sched_resched();
        }
        ret = 1;
    }

    return ret;
}

/**
 * @brief  Wake all threads sleeping on a wait queue
 *
 * This function removes all threads (if any) from the wait queue and
 * makes them executable.  The new threads will be placed at the head of the
 * run queue.
 *
 * @param wait  The wait queue to wake
 * @param wait_queue_error  The return value which the new thread will receive
 * from wait_queue_block().
 *
 * @return  The number of threads woken (zero or one)
 */
int wait_queue_wake_all(wait_queue_t *wait, status_t wait_queue_error) {
    thread_t *t;
    int ret = 0;
    uint32_t cpu_mask = 0;

    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    if (wait->count == 0) {
        /* Nothing to wake. Return before touching the current thread's run queue
         * state: the self-insert below is only unwound by the sched_resched() at
         * the end of this function, which is gated on having woken something.
         * Inserting here would leave the current thread sitting on the run queue
         * while it is still running.
         */
        return 0;
    }

    /* If preemption is disabled -- by an interrupt handler, or by a caller
     * batching a run of wakeups -- just record that a reschedule is owed and
     * let whoever reenables preemption take it. Otherwise switch now.
     */
    const bool resched_now = !preempt_set_pending_if_disabled();
    if (resched_now) {
        /* stick the current thread on the head of the run queue first, so that the
         * newly awakened threads get a chance to run before the current one, but the
         * current one doesn't get unnecessarilly punished.
         */
        current_thread->state = THREAD_READY;
        sched_insert_runnable_head_on(arch_curr_cpu_num(), current_thread);
    }

    /* pop all the threads off the wait queue into the run queue */
    while ((t = list_remove_tail_type(&wait->list, thread_t, queue_node))) {
        wait->count--;
        DEBUG_ASSERT(t->state == THREAD_BLOCKED);
        t->state = THREAD_READY;
        t->wait_queue_block_ret = wait_queue_error;
        t->blocking_wait_queue = NULL;
        /* accumulate the targets and send one batch of ipis at the end rather
         * than one per thread woken */
        cpu_mask |= (1U << sched_insert_runnable(t));
        ret++;
    }

    DEBUG_ASSERT(wait->count == 0);

    if (ret > 0) {
        sched_poke_cpus(cpu_mask);
        if (resched_now) {
            sched_resched();
        }
    }

    return ret;
}

/**
 * @brief  Free all resources allocated in wait_queue_init()
 *
 * If any threads were waiting on this queue, they are all woken.
 */
void wait_queue_destroy(wait_queue_t *wait) {
    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    wait_queue_wake_all(wait, ERR_OBJECT_DESTROYED);
    wait->magic = 0;
}

/**
 * @brief  Wake a specific thread in a wait queue
 *
 * This function extracts a specific thread from a wait queue, wakes it, and
 * puts it at the head of the run queue.
 *
 * @param t  The thread to wake
 * @param wait_queue_error  The return value which the new thread will receive
 *   from wait_queue_block().
 *
 * @return ERR_NOT_BLOCKED if thread was not in any wait queue.
 */
status_t thread_unblock_from_wait_queue(thread_t *t, status_t wait_queue_error) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    if (t->state != THREAD_BLOCKED) {
        return ERR_NOT_BLOCKED;
    }

    DEBUG_ASSERT(t->blocking_wait_queue != NULL);
    DEBUG_ASSERT(t->blocking_wait_queue->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(list_in_list(&t->queue_node));

    list_delete(&t->queue_node);
    t->blocking_wait_queue->count--;
    t->blocking_wait_queue = NULL;
    t->state = THREAD_READY;
    t->wait_queue_block_ret = wait_queue_error;
    sched_poke_cpus(1U << sched_insert_runnable(t));

    return NO_ERROR;
}

/** @} */
