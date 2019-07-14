/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/defines.h>
#include <arch/ops.h>
#include <arch/thread.h>
#include <lk/compiler.h>
#include <lk/list.h>
#include <sys/types.h>

__BEGIN_CDECLS

/* wait queue stuff */
#define WAIT_QUEUE_MAGIC (0x77616974) // 'wait'

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

__END_CDECLS
