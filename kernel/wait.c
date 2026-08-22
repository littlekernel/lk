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
 * Each queue has its own lock, which protects the list and the count and
 * nothing else: the scheduling state of a thread on the list belongs to the
 * scheduler's per-cpu locks, which nest inside this one (see
 * kernel/thread_lock.h). So the division of labor with sched.c is that this
 * file moves threads on and off lists, and the scheduler makes them BLOCKED
 * and READY: sched_block() for the thread going to sleep, and
 * sched_wake_list() and sched_wake_list_and_resched() for a run of threads
 * just popped off the list. A wake pops the threads first, under the queue
 * lock, and hands the scheduler the batch; it then decides per thread whether
 * it is still there to be woken, because the other way off a wait queue, the
 * timeout, does not go through the list at all. It wakes the thread directly,
 * and the thread pulls its own node off afterwards. A waker that pops such a
 * node finds nothing to wake and moves on.
 *
 * @defgroup  wait  Wait Queue
 * @{
 */
#include <kernel/wait.h>

#include <assert.h>
#include <kernel/preempt.h>
#include <kernel/sched.h>
#include <kernel/thread.h>
#include <kernel/thread_lock.h>
#include <kernel/timer.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>

void wait_queue_init(wait_queue_t *wait) {
    *wait = (wait_queue_t)WAIT_QUEUE_INITIAL_VALUE(*wait);
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
 * The wait queue lock is held on entry and released on return, whether or
 * not the thread blocked; see the header for why.
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
    DEBUG_ASSERT(wait_queue_lock_held(wait));

    if (timeout == 0) {
        spin_unlock(wait_queue_lock(wait));
        return ERR_TIMED_OUT;
    }

    list_add_tail(&wait->list, &current_thread->wait_queue_node);
    wait->count++;
    current_thread->blocking_wait_queue = wait;
    current_thread->wait_queue_block_ret = NO_ERROR;

    /* The timer lives on this stack and is armed by the scheduler once the
     * thread is BLOCKED, so that it cannot fire before there is anything for
     * it to wake. It is cancelled on the way out whether it fired or not. */
    const bool timed = (timeout != INFINITE_TIME);
    if (timed) {
        timer_initialize(&timer);
    }

    /* switch away; the wait queue lock is dropped for the switch and stays
     * dropped. Back here the thread has been woken, by a waker that took it
     * off the list, or by the timeout, which did not. */
    const bool still_queued = sched_block(wait, timed ? &timer : NULL, timeout);

    if (timed) {
        timer_cancel(&timer);
    }
    if (unlikely(still_queued)) {
        sched_leave_wait_queue();
    }

    return current_thread->wait_queue_block_ret;
}

/* Hand a staged list of threads to the scheduler, switching away to let them
 * run first unless preemption is disabled -- by an interrupt handler, or by a
 * caller batching a run of wakeups -- in which case a reschedule is recorded
 * as owed and whoever reenables preemption takes it. The wait queue lock is
 * dropped for a switch and retaken after, so the caller sees the same lock
 * state either way. Returns the number of threads actually woken. */
static int wait_queue_wake_staged(wait_queue_t *wait, struct list_node *staged,
                                  status_t wait_queue_error) {
    if (preempt_set_pending_if_disabled()) {
        return sched_wake_list(staged, wait_queue_error);
    }

    const int woken = sched_wake_list_and_resched(wait, staged, wait_queue_error);
    spin_lock(wait_queue_lock(wait));
    return woken;
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
    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(wait_queue_lock_held(wait));

    /* A node whose thread has already timed out wakes nobody, and the caller
     * asked for one thread, so keep going until one is found or the list is
     * empty: a mutex_release() that woke nobody would leave the next waiter
     * stranded. */
    for (;;) {
        thread_t *t = list_remove_head_type(&wait->list, thread_t, wait_queue_node);
        if (t == NULL) {
            return 0;
        }
        wait->count--;

        struct list_node staged = LIST_INITIAL_VALUE(staged);
        list_add_head(&staged, &t->wait_queue_node);
        if (wait_queue_wake_staged(wait, &staged, wait_queue_error) > 0) {
            return 1;
        }
    }
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
 * @return  The number of threads woken
 */
int wait_queue_wake_all(wait_queue_t *wait, status_t wait_queue_error) {
    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(wait_queue_lock_held(wait));

    if (wait->count == 0) {
        return 0;
    }

    /* stage the whole list, in order, and hand it over in one go */
    struct list_node staged = LIST_INITIAL_VALUE(staged);
    thread_t *t;
    while ((t = list_remove_head_type(&wait->list, thread_t, wait_queue_node)) != NULL) {
        list_add_tail(&staged, &t->wait_queue_node);
    }
    wait->count = 0;

    return wait_queue_wake_staged(wait, &staged, wait_queue_error);
}

/**
 * @brief  Free all resources allocated in wait_queue_init()
 *
 * If any threads were waiting on this queue, they are all woken.
 */
void wait_queue_destroy(wait_queue_t *wait) {
    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(wait_queue_lock_held(wait));

    wait_queue_wake_all(wait, ERR_OBJECT_DESTROYED);
    wait->magic = 0;
}

/**
 * @brief  Wake a specific thread out of whatever wait queue it is in
 *
 * The thread is made runnable at the head of a run queue, and pulls its own
 * node off the wait queue on its way out of wait_queue_block(), the same as
 * it does on a timeout. No lock is needed; interrupts must be disabled. Does
 * not reschedule: if the thread landed on this cpu a reschedule is recorded
 * as pending when preemption is disabled, and is otherwise the caller's to
 * take.
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

    uint target;
    if (!sched_unblock(t, wait_queue_error, &target)) {
        return ERR_NOT_BLOCKED;
    }
    sched_poke_cpus(1U << target);
    if (target == arch_curr_cpu_num()) {
        preempt_set_pending_if_disabled();
    }

    return NO_ERROR;
}

/** @} */
