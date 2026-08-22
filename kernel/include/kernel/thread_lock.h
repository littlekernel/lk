// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <arch/ops.h>
#include <kernel/spinlock.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <sys/types.h>

__BEGIN_CDECLS

struct wait_queue;

// The locks the thread lock has been split into, so far.
//
// The scheduler now has a lock per cpu. The wait queue and thread list locks
// are still the one global thread_lock; each accessor below names the lock a
// site needs, and the order they nest in is fixed here:
//
//   thread_list_lock()  >  wait_queue_lock(wq)  >  sched_lock(cpu)  >  timer_lock
//
// outermost first. The sched lock is the innermost of the three because the
// wait queue and thread list code hand threads to the scheduler, never the
// other way around. Two sched locks may be held at once, lowest cpu number
// first; see sched_lock_pair() in sched.c. The list and wait queue locks are
// the same lock today, so a site never holds both: it takes them in sequence.
//
// Which lock owns a thread's scheduling state follows from the thread's state:
//
//   RUNNING              sched_lock(curr_cpu)
//   READY, on a queue    sched_lock(last_cpu), the cpu whose queue it is on
//   BLOCKED              wait_queue_lock(blocking_wait_queue)
//   SUSPENDED, SLEEPING,
//   DEATH                sched_lock(last_cpu): the cpu it last ran on, or for
//                        a thread that never has, the cpu that created it
//
// so last_cpu is the key for everything but a blocked thread, and is never -1
// on a thread the scheduler can see. A thread taken off a queue is invisible
// to everyone but the remover, which owns it outright until it is queued
// again; the one way to observe that window is READY with run_queue_node not
// on a list, and the one site that can (thread_set_pinned_cpu) retries.
//
// Exactly one lock is ever held across a context switch: the local cpu's
// sched lock, which is handed to the incoming thread. Everything else -- the
// wait queue lock a blocking thread came in with, a remote sched lock -- is
// dropped before sched_resched(). Holding the global lock across the switch
// used to be a release in disguise, since the incoming thread could be anyone,
// so dropping it explicitly changes nothing a caller could rely on.

extern spin_lock_t thread_lock;

// The lock protecting a cpu's run queues, and with them the state of every
// thread queued on or running on that cpu. Each lock gets its own cache line;
// the rest of the per-cpu scheduler state is private to sched.c.
struct sched_lock_slot {
    spin_lock_t lock;
} __CPU_ALIGN;
extern struct sched_lock_slot sched_lock_slots[SMP_MAX_CPUS];

static inline spin_lock_t *sched_lock(uint cpu) {
    return &sched_lock_slots[cpu].lock;
}

// The lock protecting a wait queue, and the state of every thread blocked on it.
static inline spin_lock_t *wait_queue_lock(struct wait_queue *wq) {
    (void)wq;
    return &thread_lock;
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

// For the two backward walkers in wait.c, which reach a wait queue through a
// thread and cannot name it until they hold it. Sound only while every wait
// queue lock is this one lock; see the comments at the sites.
static inline bool thread_lock_held(void) {
    return spin_lock_held_by_me(&thread_lock);
}

__END_CDECLS
