/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "arch/arm64/mp.h"

#include <arch/atomic.h>
#include <arch/mp.h>
#include <arch/ops.h>
#include <assert.h>
#include <inttypes.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/main.h>
#include <lk/trace.h>
#include <platform/interrupts.h>
#include <stdlib.h>
#include <string.h>

#include "arm64_priv.h"

#if WITH_DEV_INTERRUPT_ARM_GIC
#include <dev/interrupt/arm_gic.h>
#elif PLATFORM_BCM28XX
/* bcm28xx has a weird custom interrupt controller for MP */
extern void bcm28xx_send_ipi(uint irq, uint cpu_mask);
#else
#error need other implementation of interrupt controller that can ipi
#endif

#define LOCAL_TRACE 0

#define GIC_IPI_BASE (14)

// percpu structures for the boot cpu and secondaries
static struct arm64_percpu boot_percpu;

#if WITH_SMP
static struct arm64_percpu *secondary_percpu;
static uint secondaries_to_init = 0;

status_t arch_mp_send_ipi(mp_cpu_mask_t target, mp_ipi_t ipi) {
    LTRACEF("target 0x%x, ipi %u\n", target, ipi);

#if WITH_DEV_INTERRUPT_ARM_GIC
    uint gic_ipi_num = ipi + GIC_IPI_BASE;

    /* filter out targets outside of the range of cpus we care about */
    target &= ((1UL << SMP_MAX_CPUS) - 1);
    if (target != 0) {
        LTRACEF("target 0x%x, gic_ipi %u\n", target, gic_ipi_num);
        arm_gic_sgi(gic_ipi_num, 0, target);
    }
#elif PLATFORM_BCM28XX
    /* filter out targets outside of the range of cpus we care about */
    target &= ((1UL << SMP_MAX_CPUS) - 1);
    if (target != 0) {
        bcm28xx_send_ipi(ipi, target);
    }
#endif

    return NO_ERROR;
}

static enum handler_return arm_ipi_generic_handler(void *arg) {
    LTRACEF("cpu %u, arg %p\n", arch_curr_cpu_num(), arg);

    return INT_NO_RESCHEDULE;
}

static enum handler_return arm_ipi_reschedule_handler(void *arg) {
    LTRACEF("cpu %u, arg %p\n", arch_curr_cpu_num(), arg);

    return mp_mbx_reschedule_irq();
}

static void arm64_mp_init_percpu(void) {
    register_int_handler(MP_IPI_GENERIC + GIC_IPI_BASE, &arm_ipi_generic_handler, 0);
    register_int_handler(MP_IPI_RESCHEDULE + GIC_IPI_BASE, &arm_ipi_reschedule_handler, 0);

    // unmask_interrupt(MP_IPI_GENERIC + GIC_IPI_BASE);
    // unmask_interrupt(MP_IPI_RESCHEDULE + GIC_IPI_BASE);
}

static void arm64_init_secondary_percpu(uint cpu_num) {
    // If we're out of range, just hang
    if (cpu_num > secondaries_to_init) {
        for (;;) {
            __asm__ volatile("wfi");
        }
    }

    struct arm64_percpu *percpu = &secondary_percpu[cpu_num - 1];
    arm64_set_percpu(percpu);
    percpu->cpu_num = cpu_num;
    percpu->mpidr = ARM64_READ_SYSREG(mpidr_el1);
}

void arm64_set_secondary_cpu_count(int count) {
    secondaries_to_init = count;

    DEBUG_ASSERT(secondary_percpu == NULL);

    // clamp the secondary cpu count to SMP_MAX_CPUS - 1
    if (secondaries_to_init > (SMP_MAX_CPUS - 1)) {
        dprintf(INFO, "ARM64: clamping secondary cpu count from %d to %d\n", secondaries_to_init, SMP_MAX_CPUS - 1);
        secondaries_to_init = SMP_MAX_CPUS - 1;
    }

    // Allocate percpu structures for the secondaries
    if (secondaries_to_init > 0) {
        const size_t len = sizeof(struct arm64_percpu) * secondaries_to_init;
        secondary_percpu = memalign(CACHE_LINE, len);
        DEBUG_ASSERT(secondary_percpu);
        memset(secondary_percpu, 0, len);
    }
}

/* called from assembly */
void arm64_secondary_entry(ulong);
void arm64_secondary_entry(ulong asm_cpu_num) {
    arm64_init_secondary_percpu(asm_cpu_num);

    const uint cpu = arch_curr_cpu_num();
    if (cpu != asm_cpu_num) {
        return;
    }

    arm64_early_init_percpu();

    // Get us into thread context and run the initial secondary cpu init routines
    lk_secondary_cpu_entry_early();

    arm64_mp_init_percpu();

    dprintf(INFO, "ARM64: secondary cpu %u started, mpidr %#" PRIx64 "\n", arch_curr_cpu_num(), ARM64_READ_SYSREG(mpidr_el1));

    // Finish secondary cpu initialization and enter the scheduler
    lk_secondary_cpu_entry();
}
#endif // WITH_SMP

uint64_t arm64_cpu_num_to_mpidr(uint cpu_num) {
#if WITH_SMP
    if (cpu_num == 0) {
        return boot_percpu.mpidr;
    } else if (unlikely(cpu_num > secondaries_to_init)) {
        return UINT64_MAX;
    } else {
        return secondary_percpu[cpu_num - 1].mpidr;
    }
#else
    return boot_percpu.mpidr;
#endif
}

void arm64_mp_init(void) {
#if WITH_SMP
    arm64_mp_init_percpu();
#endif
}

// Special case, called from start.S code on the boot cpu, which will always be numbered 0
// called from assembly
void arm64_init_boot_percpu(void);
void arm64_init_boot_percpu(void) {
    arm64_set_percpu(&boot_percpu);
    boot_percpu.cpu_num = 0;
    boot_percpu.mpidr = ARM64_READ_SYSREG(mpidr_el1);
}
