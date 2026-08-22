// Copyright (c) 2008-2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <kernel/spinlock.h>
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
    // Protects the two fields above, and the wait_queue_node of every thread on
    // the list. It does not own the threads' scheduling state: see
    // kernel/thread_lock.h for what does, and for where this lock sits in the
    // lock order. The primitives built on a wait queue (mutex, event,
    // semaphore) use it for their own state as well.
    spin_lock_t lock;
} wait_queue_t;

// Initialize a wait queue to the default state. Can statically initialize a wait queue
// with the WAIT_QUEUE_INITIAL_VALUE macro or dynamically initialize it with wait_queue_init().
#define WAIT_QUEUE_INITIAL_VALUE(q) \
{ \
    .magic = WAIT_QUEUE_MAGIC, \
    .count = 0, \
    .list = LIST_INITIAL_VALUE((q).list), \
    .lock = SPIN_LOCK_INITIAL_VALUE, \
}
void wait_queue_init(wait_queue_t *wait);

// All of the below apis must be called with interrupts disabled and the wait
// queue's lock held, wait_queue_lock() in kernel/thread_lock.h, and return the
// same way -- except wait_queue_block(), which releases it.

// Release all the threads on this wait queue with a return code of ERR_OBJECT_DESTROYED.
// The caller must assure that no other thread uses the wait queue once this is
// called. A thread woken by it never touches the queue again on its way out of
// wait_queue_block(), so the memory may be freed as soon as the caller is done
// with it; the one exception is a waiter whose timeout fires at the same time
// (see wait_queue_block()), which is the caller's race to avoid.
void wait_queue_destroy(wait_queue_t *);

// Block on a wait queue.
// Return status is whatever the caller of wait_queue_wake_*() specifies.
// A timeout other than INFINITE_TIME will set abort after the specified time
// and return ERR_TIMED_OUT. A timeout of 0 will immediately return.
//
// Always returns with the wait queue lock *released*; interrupts are still
// disabled. A woken thread is off the queue before it runs and has no reason
// to touch it again, so it does not: a thread released by wait_queue_destroy()
// can count on never dereferencing a destroyed queue. A thread that timed out
// is the exception: the timeout acts on the thread and leaves its node on the
// list, and the thread takes the lock once on its way out to pull it off.
// Callers that need the queue's lock again after a block take it themselves.
status_t wait_queue_block(wait_queue_t *, lk_time_t timeout);

// Release one or more threads from the wait queue.
// wait_queue_error = what wait_queue_block() should return for the blocking thread.
// Returns the number of threads released from the wait queue.
//
// A released thread is scheduled immediately unless preemption is disabled -- by an
// interrupt handler, or by a caller batching a run of wakeups -- in which case the
// reschedule is taken when preemption is reenabled. These may therefore be called
// from interrupt context without the caller having to know that it is.
int wait_queue_wake_one(wait_queue_t *, status_t wait_queue_error);
int wait_queue_wake_all(wait_queue_t *, status_t wait_queue_error);

// Wake a thread out of whatever wait queue it is blocked on, with the given
// return code for its wait_queue_block(). The thread leaves the queue's list
// itself (see wait_queue_block()), so this needs no lock, and works the way a
// timeout does. Returns ERR_NOT_BLOCKED if the thread is not blocked.
struct thread;
status_t thread_unblock_from_wait_queue(struct thread *t, status_t wait_queue_error);

__END_CDECLS
