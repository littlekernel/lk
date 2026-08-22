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

// The kernel's one thread lock, seen through the names of the locks it is
// going to be split into.
//
// Today every accessor below returns &thread_lock, so naming a lock changes
// nothing at runtime. What it changes is the code: each site that takes the
// lock says which lock it needs, and the order the locks will nest in is fixed
// here rather than discovered at the point they actually separate:
//
//   thread_list_lock()  >  wait_queue_lock(wq)  >  sched_lock(cpu)  >  timer_lock
//
// outermost first. The sched lock is the innermost of the three because the
// wait queue and thread list code hand threads to the scheduler, never the
// other way around. A site holds at most one lock of each rank, with a single
// known exception: moving a queued thread between cpus needs two sched locks.
//
// Sites that cannot name their lock yet keep using THREAD_LOCK(), with a
// comment saying why. The two recurring reasons are that the thread being
// touched is on no queue at all (suspended, sleeping or dead, so neither a
// sched lock nor a wait queue lock owns it), and that the cpu a thread is
// headed for is only chosen inside the call that enqueues it. Those sites are
// the work items for the split, and they are deliberately not disguised
// behind a confident name.

extern spin_lock_t thread_lock;

// The lock protecting a cpu's run queues, and with them the state of every
// thread queued on or running on that cpu.
static inline spin_lock_t *sched_lock(uint cpu) {
    (void)cpu;
    return &thread_lock;
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

// The unresolved sites. Every use carries a comment naming what it is waiting
// on; do not add one without.
#define THREAD_LOCK(state) arch_interrupt_saved_state_t state = spin_lock_irqsave(&thread_lock)
#define THREAD_UNLOCK(state) spin_unlock_irqrestore(&thread_lock, state)

static inline bool thread_lock_held(void) {
    return spin_lock_held_by_me(&thread_lock);
}

__END_CDECLS
