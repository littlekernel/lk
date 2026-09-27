// Copyright (c) 2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <kernel/thread.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <stdint.h>

__BEGIN_CDECLS

// This file defines apis that deal with high level multiprocessor (MP)
// support for the kernel.
// May be used in both SMP and UP configurations, but in UP
// configurations, the APIs are no-ops and the mp_state structure
// is not used.
// WITH_SMP is defined in the build system to indicate that
// the kernel is built with SMP support.
//
// Most of thesse APIs are used to manage the state of CPUs in a multiprocessor system,
// including their active status, idle status, and whether they are running realtime threads.

// A bitmap representing active CPUs.
typedef uint32_t mp_cpu_mask_t;

// Which cpus an ipi is aimed at. The mask that accompanies it is only read for
// MP_IPI_TARGET_MASK; the other two need no mask at all, which keeps them
// usable if the mask type ever becomes too narrow for SMP_MAX_CPUS. Every
// target is filtered against the active cpus.
typedef enum {
    MP_IPI_TARGET_MASK,           // the cpus in the mask argument
    MP_IPI_TARGET_ALL,            // every cpu, the caller included
    MP_IPI_TARGET_ALL_BUT_LOCAL,  // every cpu except the caller
} mp_ipi_target_t;

// Function run by mp_sync_exec() on each target cpu.
typedef void (*mp_sync_task_t)(void *context);

// By default, mp_mbx_reschedule does not signal to cpus that are running realtime
// threads. Override this behavior.
#define MP_RESCHEDULE_FLAG_REALTIME (0x1)

// Interprocessor Interrupt (IPI) types
typedef enum {
    MP_IPI_GENERIC,
    MP_IPI_RESCHEDULE,
} mp_ipi_t;

#ifdef WITH_SMP
// Trigger a reschedule on the target cpus. The calling cpu is never signalled,
// it reschedules itself, so MP_IPI_TARGET_ALL and MP_IPI_TARGET_ALL_BUT_LOCAL
// mean the same thing here.
void mp_reschedule(mp_ipi_target_t target, mp_cpu_mask_t mask, uint flags);
void mp_set_curr_cpu_active(bool active);

// Run fn(context) on every active target cpu and return once all of them have
// finished. The calling cpu, if targeted, runs fn inline; the others run it from
// the generic IPI handler with interrupts disabled, so fn must not block and
// should be short. Interrupts must be enabled on entry when any other cpu is
// targeted: that cpu may be waiting on this one the same way.
void mp_sync_exec(mp_ipi_target_t target, mp_cpu_mask_t mask, mp_sync_task_t fn, void *context);

// mp_sync_exec() on exactly the cpus in the mask, whether or not they are active
// yet. A cpu that is started but not yet active runs fn when it first enables
// interrupts, and the caller waits until then, so the caller must know every
// cpu in the mask gets there without waiting on the caller. For work that must
// also reach a cpu before it joins the scheduler, such as a TLB shootdown on a
// cpu that already translates through the kernel's page tables.
void mp_sync_exec_cpus(mp_cpu_mask_t cpus, mp_sync_task_t fn, void *context);

// Called from arch code during reschedule irq
enum handler_return mp_mbx_reschedule_irq(void);

// Called from arch code when the generic IPI arrives. Runs the tasks queued for
// this cpu by mp_sync_exec().
enum handler_return mp_mbx_generic_irq(void);

// Global mp state to track what the cpus are up to.
struct mp_state {
    volatile mp_cpu_mask_t active_cpus;

    // only safely accessible with thread lock held
    mp_cpu_mask_t idle_cpus;
    mp_cpu_mask_t realtime_cpus;
};

extern struct mp_state mp;

// Active cpus are currently running any sort of thread, including idle threads.
static inline bool mp_is_cpu_active(uint cpu) {
    return mp.active_cpus & (1UL << cpu);
}

static inline mp_cpu_mask_t mp_get_active_mask(void) {
    return mp.active_cpus;
}

// Idle cpus are currently running the idle thread.
static inline bool mp_is_cpu_idle(uint cpu) {
    return mp.idle_cpus & (1UL << cpu);
}

// Must be called with the thread lock held.
static inline void mp_set_cpu_idle(uint cpu) {
    mp.idle_cpus |= 1UL << cpu;
}

static inline void mp_set_cpu_busy(uint cpu) {
    mp.idle_cpus &= ~(1UL << cpu);
}

static inline mp_cpu_mask_t mp_get_idle_mask(void) {
    return mp.idle_cpus;
}

// Realtime cpus are currently running realtime threads.
static inline void mp_set_cpu_realtime(uint cpu) {
    mp.realtime_cpus |= 1UL << cpu;
}

static inline void mp_set_cpu_non_realtime(uint cpu) {
    mp.realtime_cpus &= ~(1UL << cpu);
}

static inline mp_cpu_mask_t mp_get_realtime_mask(void) {
    return mp.realtime_cpus;
}
#else
static inline void mp_reschedule(mp_ipi_target_t target, mp_cpu_mask_t mask, uint flags) {}
static inline void mp_set_curr_cpu_active(bool active) {}

// the only cpu is cpu 0; run the task here if it was asked for
static inline void mp_sync_exec(mp_ipi_target_t target, mp_cpu_mask_t mask, mp_sync_task_t fn, void *context) {
    if (target == MP_IPI_TARGET_ALL || (target == MP_IPI_TARGET_MASK && (mask & 1))) {
        fn(context);
    }
}

static inline void mp_sync_exec_cpus(mp_cpu_mask_t cpus, mp_sync_task_t fn, void *context) {
    if (cpus & 1) {
        fn(context);
    }
}

static inline enum handler_return mp_mbx_reschedule_irq(void) { return INT_NO_RESCHEDULE; }
static inline enum handler_return mp_mbx_generic_irq(void) { return INT_NO_RESCHEDULE; }

// only one cpu exists in UP and if you're calling these functions, it's active...
static inline int mp_is_cpu_active(uint cpu) { return 1; }
static inline mp_cpu_mask_t mp_get_active_mask(void) { return 1; }
static inline int mp_is_cpu_idle(uint cpu) { return (get_current_thread()->flags & THREAD_FLAG_IDLE) != 0; }

static inline void mp_set_cpu_idle(uint cpu) {}
static inline void mp_set_cpu_busy(uint cpu) {}

static inline mp_cpu_mask_t mp_get_idle_mask(void) { return 0; }

static inline void mp_set_cpu_realtime(uint cpu) {}
static inline void mp_set_cpu_non_realtime(uint cpu) {}

static inline mp_cpu_mask_t mp_get_realtime_mask(void) { return 0; }
#endif

__END_CDECLS
