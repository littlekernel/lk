// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

// The kernel's per-cpu state, in one structure per cpu.
//
// This header is deliberately a leaf: it depends on the arch headers, the
// timer and list types, and nothing else in kernel/. spinlock.h, thread_lock.h
// and thread.h all include it -- the spinlock ownership record, the sched lock
// and the thread statistics all live in here -- so anything it pulled in from
// kernel/ would be a cycle. That is also why the handful of build knobs the
// layout depends on (THREAD_STATS, SPIN_LOCK_TRACK_HELD, NUM_PRIORITIES) are
// decided here rather than in the headers that own the feature: the struct
// has to be the same shape in every translation unit, whichever header got
// there first.
//
// The point of one structure rather than one array per subsystem is to decide
// which fields share a cache line. Under SMP each group below is pinned to its
// own line, and the groups are chosen by who touches them:
//
//   sched_lock, run_queue_bitmap, previous_thread
//       Written by the owning cpu on every context switch, and only ever read
//       under the lock. The lock and the bitmap are always touched together, so
//       taking the lock brings the bitmap in with it.
//   runnable_count
//       Written by the owner on every enqueue and dequeue, but also read
//       unlocked by *other* cpus deciding where to place a wakeup
//       (find_target_cpu). That remote read would downgrade whichever line it
//       lives on to shared, and the owner's next store to that line takes an
//       upgrade miss; on the lock's line that would be every spin_lock(). So it
//       gets a line to itself: the only line here that is meant to be shared.
//   run_queue[]
//       Eight lines of list heads on 64 bit. Only the active priority's line
//       is warm, and it stays warm, so there is nothing to gain from shrinking
//       it.
//   timer_lock, timer_queue, preempt_timer
//       The cpu's timer queue, its lock, and the preemption timer that is set
//       and cancelled as the cpu goes in and out of idle. Interrupt time and
//       idle transitions, so off the switch path proper; the lock is per cpu
//       so that a timed wait on one cpu does not contend with the tick on
//       another.
//   stats, held_locks
//       Debug-build bookkeeping. Written on every switch (stats) or every lock
//       operation (held_locks), so they still need their own lines to keep
//       adjacent cpus from false sharing; they just do not need to be near
//       anything else.
//
// With a single cpu none of that matters and the padding is pure waste --
// which on a small cortex-m is real memory -- so the line alignment only
// applies under WITH_SMP. The struct is then just the same fields packed
// together, which is as good as locality gets.

#include <arch/defines.h>
#include <arch/ops.h>
#include <arch/spinlock.h>
#include <kernel/timer.h>
#include <lk/compiler.h>
#include <lk/list.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

// Everything that changes the shape of struct percpu has to be visible to every
// translation unit, whatever it includes and in what order, or two files will
// disagree about where a field is and quietly corrupt each other's state. The
// knobs below are decided here; these two come from the build system, and
// this insists on it. (ARCH_CONTEXT_SWITCH_DROPS_LOCK was once a #define in
// an arch header; timer.c, which includes spinlock.h before thread.h, did not
// see it and took its spinlocks against a struct percpu four bytes out of
// step with the rest of the kernel.)
#ifndef ARCH_CONTEXT_SWITCH_DROPS_LOCK
#error "ARCH_CONTEXT_SWITCH_DROPS_LOCK must come from config.h (engine.mk), not a header"
#endif
#ifndef PLATFORM_HAS_DYNAMIC_TIMER
#error "PLATFORM_HAS_DYNAMIC_TIMER must come from config.h (engine.mk), not a header"
#endif

// Debug-build features whose state lives in struct percpu. Owned by this
// header so that the layout is decided in one place; see above.
#ifndef THREAD_STATS
#if LK_DEBUGLEVEL > 1
#define THREAD_STATS 1
#else
#define THREAD_STATS 0
#endif
#endif

#ifndef SPIN_LOCK_TRACK_HELD
#if LK_DEBUGLEVEL > 1
#define SPIN_LOCK_TRACK_HELD 1
#else
#define SPIN_LOCK_TRACK_HELD 0
#endif
#endif

// The number of scheduler priority levels, here because it sizes the run
// queues. The named priorities derived from it are in kernel/thread.h.
#define NUM_PRIORITIES 32

#if WITH_SMP
#define PERCPU_LINE __CPU_ALIGN
#else
#define PERCPU_LINE
#endif

struct thread;

#if THREAD_STATS
// Per-cpu scheduler statistics, written on every context switch.
struct thread_stats {
    lk_bigtime_t idle_time;
    lk_bigtime_t last_idle_timestamp;
    ulong reschedules;
    ulong context_switches;
    ulong preempts;
    ulong yields;
    ulong interrupts; // platform code increment this
    ulong timer_ints; // timer code increment this
    ulong timers; // timer code increment this

#if WITH_SMP
    ulong reschedule_ipis;
#endif
};
#endif

#if SPIN_LOCK_TRACK_HELD
// Deeper than anything in the tree nests; the overflow check in spinlock.h is
// the real limit.
#define SPIN_LOCK_HELD_MAX 8

// Debug-only record of which locks a cpu holds. See kernel/spinlock.h for why
// this lives beside the locks rather than inside the lock word.
struct spin_lock_held_state {
    spin_lock_t *locks[SPIN_LOCK_HELD_MAX];
    uint count;
};
#endif

struct percpu {
    // The scheduler's owner-private state. See kernel/thread_lock.h for what
    // sched_lock protects and where it sits in the lock order.
    spin_lock_t sched_lock;
    uint32_t run_queue_bitmap;
#if !ARCH_CONTEXT_SWITCH_DROPS_LOCK
    // The thread this cpu is in the middle of switching away from; see
    // sched_context_switch_complete() in sched.c.
    struct thread *previous_thread;
#endif

    // Read unlocked by other cpus; on its own line for that reason.
    uint runnable_count PERCPU_LINE;

    struct list_node run_queue[NUM_PRIORITIES] PERCPU_LINE;

    // The cpu's timer queue and the lock that protects it. Timers are queued
    // on the cpu that set them and record which (timer_t.cpu), so a cancel
    // from another cpu takes this lock rather than a global one. See
    // kernel/timer.c.
    spin_lock_t timer_lock PERCPU_LINE;
    struct list_node timer_queue;
#if PLATFORM_HAS_DYNAMIC_TIMER
    timer_t preempt_timer;
#endif

#if THREAD_STATS
    struct thread_stats stats PERCPU_LINE;
#endif

#if SPIN_LOCK_TRACK_HELD
    struct spin_lock_held_state held_locks PERCPU_LINE;
#endif
} PERCPU_LINE;

extern struct percpu percpu_array[SMP_MAX_CPUS];

static inline struct percpu *percpu_get(uint cpu) {
    return &percpu_array[cpu];
}

// The local cpu's state. Only meaningful with interrupts disabled (or
// preemption otherwise off), as the caller could be migrated right after
// reading the cpu number.
static inline struct percpu *percpu_local(void) {
    return &percpu_array[arch_curr_cpu_num()];
}

__END_CDECLS
