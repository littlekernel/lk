// Copyright (c) 2008-2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lk/compiler.h>
#include <lk/list.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

// The wait queue is the basis for all locking primitives.
// It is a simple FIFO queue of threads that are blocked waiting for some
// condition to be met.  The wait queue is used by mutexes, semaphores,
// condition variables, and other synchronization primitives.
//
// The wait queue is not a general purpose queue, it is only used for
// blocking threads.  It is not intended to be used for other purposes.
#define WAIT_QUEUE_MAGIC (0x77616974) // 'wait'

typedef struct wait_queue {
    uint32_t magic;
    int count;
    struct list_node list;
} wait_queue_t;

// Initialize a wait queue to the default state. Can statically initialize a wait queue
// with the WAIT_QUEUE_INITIAL_VALUE macro or dynamically initialize it with wait_queue_init().
#define WAIT_QUEUE_INITIAL_VALUE(q) \
{ \
    .magic = WAIT_QUEUE_MAGIC, \
    .count = 0, \
    .list = LIST_INITIAL_VALUE((q).list) \
}
void wait_queue_init(wait_queue_t *wait);

// All of the below apis must be called with interrupts disabled and the main thread
// lock held (see kernel/thread.h for details).

// Release all the threads on this wait queue with a return code of ERR_OBJECT_DESTROYED.
// the caller must assure that no other threads are operating on the wait queue during or
// after the call.
void wait_queue_destroy(wait_queue_t *, bool reschedule);

// Block on a wait queue.
// Return status is whatever the caller of wait_queue_wake_*() specifies.
// A timeout other than INFINITE_TIME will set abort after the specified time
// and return ERR_TIMED_OUT. A timeout of 0 will immediately return.
status_t wait_queue_block(wait_queue_t *, lk_time_t timeout);

// Release one or more threads from the wait queue.
// reschedule = should the system reschedule if any is released.
// wait_queue_error = what wait_queue_block() should return for the blocking thread.
// Returns the number of threads released from the wait queue.
//
// May be called at interrupt context, but reschedule *must* be false in that case.
int wait_queue_wake_one(wait_queue_t *, bool reschedule, status_t wait_queue_error);
int wait_queue_wake_all(wait_queue_t *, bool reschedule, status_t wait_queue_error);

// Remove the thread from whatever wait queue it's in.
// Return an error if the thread is not currently blocked (or is the current thread).
struct thread;
status_t thread_unblock_from_wait_queue(struct thread *t, status_t wait_queue_error);

__END_CDECLS
