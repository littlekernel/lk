// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <arch/ops.h>
#include <kernel/percpu.h>
#include <kernel/spinlock.h>
#include <kernel/wait.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <sys/types.h>

__BEGIN_CDECLS

// The locks the thread lock was split into.
//
// The scheduler has a lock per cpu, every wait queue has its own, and the
// thread list keeps the original global lock. Each accessor below names the
// lock a site needs, and the order they nest in is fixed here:
//
//   thread_list_lock()  >  wait_queue_lock(wq)  >  sched_lock(cpu)  >  timer_lock
//
// outermost first. The sched lock is the innermost of the three because the
// wait queue and thread list code hand threads to the scheduler, never the
// other way around. Two sched locks may be held at once, lowest cpu number
// first; see sched_lock_pair() in sched.c. Two wait queue locks never are.
//
// A thread's scheduling state -- state, the run queue node, pinned_cpu,
// last_cpu -- is always owned by a sched lock, which one following from the
// state:
//
//   RUNNING              sched_lock(curr_cpu)
//   READY, on a queue    sched_lock(last_cpu), the cpu whose queue it is on
//   BLOCKED, SUSPENDED,
//   SLEEPING, DEATH      sched_lock(last_cpu): the cpu it last ran on, or for
//                        a thread that never has, the cpu that created it
//
// so last_cpu is the key for anything not running, and is never -1 on a
// thread the scheduler can see. The wait queue lock owns only the queue: its
// list, its count, and so the thread's wait_queue_node while it is on one.
// That a blocked thread is *not* owned by its queue's lock is what lets the
// timeout path and thread_set_pinned_cpu() act on a blocked thread without
// finding the queue, which they would have no safe way to do: the queue may
// be gone by the time a pointer to it is followed. A waker holds both -- the
// queue's lock to take the thread off the list, then the sched lock pair to
// make it READY -- and a wake racing a timeout is decided by the sched lock,
// with the loser doing nothing (see sched_make_ready_from() in sched.c).
//
// The one way to see a thread READY with run_queue_node not on a list is the
// thread itself on its way off the cpu, between marking itself READY and
// queueing itself; the one site that can (thread_set_pinned_cpu) retries.
//
// Exactly one lock is ever held across a context switch: the local cpu's
// sched lock, which is handed to the incoming thread. Everything else -- the
// wait queue lock a blocking thread came in with, a remote sched lock -- is
// dropped before sched_resched(). Holding the global lock across the switch
// used to be a release in disguise, since the incoming thread could be anyone,
// so dropping it explicitly changes nothing a caller could rely on.

// The thread list lock. Was the one lock for all of the above, hence the name.
extern spin_lock_t thread_lock;

// The lock protecting a cpu's run queues, and with them the state of every
// thread queued on or running on that cpu. It lives in the cpu's struct percpu
// on the same cache line as the run queue bitmap it guards (kernel/percpu.h).
static inline spin_lock_t *sched_lock(uint cpu) {
    return &percpu_get(cpu)->sched_lock;
}

// The lock protecting a wait queue: its list and count, not the scheduling
// state of the threads on it, which their sched locks own (see above).
static inline spin_lock_t *wait_queue_lock(struct wait_queue *wq) {
    return &wq->lock;
}

// The lock protecting the global thread list.
static inline spin_lock_t *thread_list_lock(void) {
    return &thread_lock;
}

// Take and release the local cpu's sched lock.
//
// The cpu number is read with interrupts already disabled, so the caller
// cannot be migrated between reading it and taking the lock. The unlock reads
// it again, for the same reason and for a second one: a caller that context
// switched while holding the lock resumes on whichever cpu picked it up,
// holding *that* cpu's lock, since the lock is handed across the switch rather
// than released. So the lock to drop is the one local to wherever we are now,
// not the one taken going in.
static inline arch_interrupt_saved_state_t sched_lock_local_irqsave(void) {
    arch_interrupt_saved_state_t state = arch_interrupt_save();
    spin_lock(sched_lock(arch_curr_cpu_num()));
    return state;
}

static inline void sched_unlock_local_irqrestore(arch_interrupt_saved_state_t state) {
    spin_unlock(sched_lock(arch_curr_cpu_num()));
    arch_interrupt_restore(state);
}

static inline arch_interrupt_saved_state_t wait_queue_lock_irqsave(struct wait_queue *wq) {
    return spin_lock_irqsave(wait_queue_lock(wq));
}

static inline void wait_queue_unlock_irqrestore(struct wait_queue *wq,
                                                arch_interrupt_saved_state_t state) {
    spin_unlock_irqrestore(wait_queue_lock(wq), state);
}

static inline arch_interrupt_saved_state_t thread_list_lock_irqsave(void) {
    return spin_lock_irqsave(thread_list_lock());
}

static inline void thread_list_unlock_irqrestore(arch_interrupt_saved_state_t state) {
    spin_unlock_irqrestore(thread_list_lock(), state);
}

// "Do I hold X?" predicates for the asserts. These check ownership rather than
// "held by anybody": once there are several locks, some cpu holding some sched
// lock asserts nothing.
static inline bool sched_lock_held(uint cpu) {
    return spin_lock_held_by_me(sched_lock(cpu));
}

static inline bool sched_lock_local_held(void) {
    return sched_lock_held(arch_curr_cpu_num());
}

static inline bool wait_queue_lock_held(struct wait_queue *wq) {
    return spin_lock_held_by_me(wait_queue_lock(wq));
}

static inline bool thread_list_lock_held(void) {
    return spin_lock_held_by_me(thread_list_lock());
}

__END_CDECLS
