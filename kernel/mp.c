/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <kernel/mp.h>

#include <arch/atomic.h>
#include <arch/mp.h>
#include <arch/ops.h>
#include <assert.h>
#include <kernel/init.h>
#include <kernel/spinlock.h>
#include <lk/debug.h>
#include <lk/list.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

#if WITH_SMP
/* a global state structure, aligned on cpu cache line to minimize aliasing */
struct mp_state mp __CPU_ALIGN;

/* One queued mp_sync_exec() request for one cpu. Lives on the caller's stack, so
 * a target must not touch it after decrementing outstanding. */
struct mp_sync_task {
    struct list_node node;
    mp_sync_task_t fn;
    void *context;
    int *outstanding;
};

static spin_lock_t mp_sync_lock = SPIN_LOCK_INITIAL_VALUE;
static struct list_node mp_sync_queue[SMP_MAX_CPUS];

void mp_init(void) {
    for (uint i = 0; i < SMP_MAX_CPUS; i++) {
        list_initialize(&mp_sync_queue[i]);
    }
}

/* Expand a target into the cpus it names. Callers still filter by active_cpus. */
static mp_cpu_mask_t mp_target_to_mask(mp_ipi_target_t target, mp_cpu_mask_t mask, uint local_cpu) {
    switch (target) {
        case MP_IPI_TARGET_MASK:
            return mask;
        case MP_IPI_TARGET_ALL:
            return ~0U;
        case MP_IPI_TARGET_ALL_BUT_LOCAL:
            return ~(1U << local_cpu);
    }
    panic("bad ipi target %d\n", target);
}

void mp_reschedule(mp_ipi_target_t target, mp_cpu_mask_t mask, uint flags) {
    uint local_cpu = arch_curr_cpu_num();

    LTRACEF("local %d, target %d mask 0x%x\n", local_cpu, target, mask);

    /* mask out cpus that are not active and the local cpu */
    mp_cpu_mask_t cpus = mp_target_to_mask(target, mask, local_cpu) & mp.active_cpus;

    /* mask out cpus that are currently running realtime code */
    if ((flags & MP_RESCHEDULE_FLAG_REALTIME) == 0) {
        cpus &= ~mp.realtime_cpus;
    }
    cpus &= ~(1U << local_cpu);
    if (cpus == 0) {
        return;
    }

    LTRACEF("local %d, post mask cpus now 0x%x\n", local_cpu, cpus);

    arch_mp_send_ipi(cpus, MP_IPI_RESCHEDULE);
}

void mp_set_curr_cpu_active(bool active) {
    uint cpu = arch_curr_cpu_num();

    if (active) {
        atomic_or((volatile int *)&mp.active_cpus, 1U << cpu);
    } else {
        atomic_and((volatile int *)&mp.active_cpus, ~(1U << cpu));
    }
}

/* The body of mp_sync_exec() and mp_sync_exec_cpus(); only_active limits the
 * targets to the cpus in mp.active_cpus. */
static void mp_sync_exec_etc(mp_ipi_target_t target, mp_cpu_mask_t mask, bool only_active,
                             mp_sync_task_t fn, void *context) {
    struct mp_sync_task tasks[SMP_MAX_CPUS];
    int outstanding = 0;

    LTRACEF("target %d mask 0x%x only_active %d, fn %p\n", target, mask, only_active, fn);

    DEBUG_ASSERT(fn);

    /* Choose the cpu set and dispatch with interrupts off, so this thread cannot
     * migrate between deciding which cpu is local and running fn there. */
    bool ints_were_disabled = arch_ints_disabled();
    arch_interrupt_saved_state_t state = arch_interrupt_save();

    uint local_cpu = arch_curr_cpu_num();
    mp_cpu_mask_t cpus = mp_target_to_mask(target, mask, local_cpu);
    if (only_active) {
        cpus &= mp.active_cpus;
    }
    mp_cpu_mask_t remote = cpus & ~(1U << local_cpu);

    if (remote) {
        /* waiting on another cpu needs interrupts on: it may be waiting on us */
        DEBUG_ASSERT(!ints_were_disabled);

        outstanding = __builtin_popcount(remote);

        spin_lock(&mp_sync_lock);
        mp_cpu_mask_t m = remote;
        while (m) {
            uint cpu = __builtin_ctz(m);
            m &= ~(1U << cpu);

            tasks[cpu].fn = fn;
            tasks[cpu].context = context;
            tasks[cpu].outstanding = &outstanding;
            list_add_tail(&mp_sync_queue[cpu], &tasks[cpu].node);
        }
        spin_unlock(&mp_sync_lock);

        arch_mp_send_ipi(remote, MP_IPI_GENERIC);
    }

    if (cpus & (1U << local_cpu)) {
        fn(context);
    }

    arch_interrupt_restore(state);

    /* the tasks live on this stack: do not return until every target has let go */
    while (__atomic_load_n(&outstanding, __ATOMIC_ACQUIRE) != 0) {
        arch_spinloop_pause();
    }
}

void mp_sync_exec(mp_ipi_target_t target, mp_cpu_mask_t mask, mp_sync_task_t fn, void *context) {
    mp_sync_exec_etc(target, mask, true, fn, context);
}

void mp_sync_exec_cpus(mp_cpu_mask_t cpus, mp_sync_task_t fn, void *context) {
    mp_sync_exec_etc(MP_IPI_TARGET_MASK, cpus, false, fn, context);
}

enum handler_return mp_mbx_generic_irq(void) {
    uint cpu = arch_curr_cpu_num();

    LTRACEF("cpu %u\n", cpu);

    DEBUG_ASSERT(arch_ints_disabled());

    for (;;) {
        spin_lock(&mp_sync_lock);
        struct mp_sync_task *task =
            list_remove_head_type(&mp_sync_queue[cpu], struct mp_sync_task, node);
        spin_unlock(&mp_sync_lock);
        if (!task) {
            break;
        }

        int *outstanding = task->outstanding;
        task->fn(task->context);
        /* the caller may return and reuse the task's stack as soon as this hits zero */
        __atomic_fetch_sub(outstanding, 1, __ATOMIC_RELEASE);
    }

    return INT_NO_RESCHEDULE;
}

enum handler_return mp_mbx_reschedule_irq(void) {
    uint cpu = arch_curr_cpu_num();

    LTRACEF("cpu %u\n", cpu);

    THREAD_STATS_INC(reschedule_ipis);

    return (mp.active_cpus & (1U << cpu)) ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}
#endif
