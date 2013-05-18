/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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
#ifndef __KERNEL_WAIT_H
#define __KERNEL_WAIT_H

#include <sys/types.h>
#include <list.h>
#include <compiler.h>
#include <arch/defines.h>
#include <arch/ops.h>
#include <arch/thread.h>

/* wait queue stuff */
#define WAIT_QUEUE_MAGIC 'wait'

typedef struct wait_queue {
	int magic;
	struct list_node list;
	int count;
} wait_queue_t;

#define WAIT_QUEUE_INITIAL_VALUE(q) \
{ \
	.magic = WAIT_QUEUE_MAGIC, \
	.list = LIST_INITIAL_VALUE((q).list), \
	.count = 0 \
}

/* wait queue primitive */
/* NOTE: must be inside critical section when using these */
void wait_queue_init(wait_queue_t *wait);

/*
 * release all the threads on this wait queue with a return code of ERR_OBJECT_DESTROYED.
 * the caller must assure that no other threads are operating on the wait queue during or
 * after the call.
 */
void wait_queue_destroy(wait_queue_t *, bool reschedule);

/*
 * block on a wait queue.
 * return status is whatever the caller of wait_queue_wake_*() specifies.
 * a timeout other than INFINITE_TIME will set abort after the specified time
 * and return ERR_TIMED_OUT. a timeout of 0 will immediately return.
 */
status_t wait_queue_block(wait_queue_t *, lk_time_t timeout);

/*
 * release one or more threads from the wait queue.
 * reschedule = should the system reschedule if any is released.
 * wait_queue_error = what wait_queue_block() should return for the blocking thread.
 */
int wait_queue_wake_one(wait_queue_t *, bool reschedule, status_t wait_queue_error);
int wait_queue_wake_all(wait_queue_t *, bool reschedule, status_t wait_queue_error);

/*
 * remove the thread from whatever wait queue it's in.
 * return an error if the thread is not currently blocked (or is the current thread)
 */
status_t thread_unblock_from_wait_queue(struct thread *t, status_t wait_queue_error);

#endif

