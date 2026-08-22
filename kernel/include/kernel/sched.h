// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <kernel/thread.h>
#include <kernel/mp.h>
#include <lk/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

// Scheduler interface, used internally by thread.c and the wait queue
// implementation. NOT intended for use by regular kernel code, which should
// stick to the thread_* api in <kernel/thread.h>.
//
// The scheduler owns the per-cpu run queues, the choice of which cpu a runnable
// thread lands on, the idle threads, and the context switch itself. Everything
// here runs with a sched lock held unless noted: the local cpu's for anything
// touching the current thread, the target cpu's for an insert onto a specific
// cpu. There is one per cpu; see kernel/thread_lock.h for the order and for
// which lock owns a thread in which state.

// Initialize the run queues. Must run before any thread is made runnable,
// including the half-constructed bootstrap thread. Called by thread_init_early().
void sched_init_early(void);

// Second stage init, once timers exist. Called by thread_init().
void sched_init(void);

// The statically allocated idle thread for a cpu. Idle threads are never on a
// run queue; the scheduler falls back to them when a cpu has nothing to run.
thread_t *sched_idle_thread(uint cpu);

// The idle thread body. Never returns.
void sched_idle_routine(void) __NO_RETURN;

// Pick a new thread and switch to it. The caller must have already put the
// current thread into whatever state and queue it belongs in.
void sched_resched(void);

// For the arch's initial_thread_func: the first thing a new thread does.
// Finishes the context switch that started it and releases the local sched
// lock, which was handed across the switch and which this thread never
// acquired. Interrupts are still disabled on return; the arch enables them.
// Not present on arches that set ARCH_CONTEXT_SWITCH_DROPS_LOCK (cortex-m),
// where nothing is handed off and a new thread starts with no lock held.
void sched_initial_thread_entry(void);

// Put a thread on the head of the run queue of the cpu the scheduler picks for
// it, and return that cpu so the caller can poke it. Does not reschedule. The
// caller holds no sched lock and owns the thread: it has just taken it off a
// wait queue, and the thread is already READY. The target's lock is taken here.
uint sched_insert_runnable(thread_t *t);

// The same for a thread that is on no queue at all (suspended or sleeping) and
// is therefore owned by the sched lock of its last cpu. Moves it from |from| to
// READY atomically with the insert, or returns false if it was not in |from|.
bool sched_insert_runnable_from(thread_t *t, enum thread_state from, uint *target_out);

// Switch away on behalf of the wait queue code, which holds |wq|'s lock and has
// already queued or otherwise disposed of the current thread. The wait queue
// lock is dropped for the switch and held again on return.
struct wait_queue;
void sched_resched_from_wait_queue(struct wait_queue *wq);

// For a wake path about to switch away: put the current thread at the head of
// the local run queue first, so the woken threads go in ahead of it. Takes and
// releases the local sched lock; the caller holds none.
void sched_requeue_current_for_wake(void);

// Free a detached thread that has exited. Called by the scheduler on the far
// side of its last context switch, once nothing is running on its stack.
void thread_reap_detached(thread_t *t);

// Put a thread on a specific cpu's run queue. Used for the current thread,
// which by definition belongs on the local cpu.
void sched_insert_runnable_head_on(uint cpu, thread_t *t);
void sched_insert_runnable_tail_on(uint cpu, thread_t *t);

// Make the cpus a set of threads were just steered at notice them. Batched so a
// caller waking several threads sends one round of ipis.
void sched_poke_cpus(mp_cpu_mask_t targets);

__END_CDECLS
