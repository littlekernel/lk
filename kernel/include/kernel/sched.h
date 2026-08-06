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
// here runs with the thread lock held unless noted.

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

// Put a thread on the head of the run queue of the cpu the scheduler picks for
// it, and return that cpu so the caller can poke it. Does not reschedule.
uint sched_insert_runnable(thread_t *t);

// Put a thread on a specific cpu's run queue. Used for the current thread,
// which by definition belongs on the local cpu.
void sched_insert_runnable_head_on(uint cpu, thread_t *t);
void sched_insert_runnable_tail_on(uint cpu, thread_t *t);

// Make the cpus a set of threads were just steered at notice them. Batched so a
// caller waking several threads sends one round of ipis.
void sched_poke_cpus(mp_cpu_mask_t targets);

__END_CDECLS
