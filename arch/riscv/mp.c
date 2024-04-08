/*
 * Copyright (c) 2019 Elliot Berman
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/riscv.h>

#include <lk/reg.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/main.h>
#include <stdlib.h>
#include <string.h>

#include <arch/atomic.h>
#include <arch/ops.h>
#include <arch/mp.h>
#include <arch/riscv/clint.h>

#include "riscv_priv.h"

#if WITH_SMP

#define LOCAL_TRACE 0

// mapping of cpu -> hart
static uint cpu_to_hart_map[SMP_MAX_CPUS];

// list of IPIs queued per cpu
static volatile int ipi_data[SMP_MAX_CPUS];

static spin_lock_t boot_cpu_lock = 1;

// list of cpus to boot, passed from platform
static uint harts_to_boot[SMP_MAX_CPUS];
static uint harts_to_boot_count = 0;

// modified in start.S to save the physical address of _start as the first cpu boots
uintptr_t _start_physical;

status_t arch_mp_send_ipi(mp_cpu_mask_t target, mp_ipi_t ipi) {
    LTRACEF("target 0x%x, ipi %u\n", target, ipi);

    mp_cpu_mask_t m = target;
    ulong hart_mask = 0;
    for (uint c = 0; c < SMP_MAX_CPUS && m; c++, m >>= 1) {
        if (m & 1) {
            int h = cpu_to_hart_map[c];
            LTRACEF("c %u h %d m %#x\n", c, h, m);

            // record a pending hart to notify
            hart_mask |= (1ul << h);

            // set the ipi_data based on the incoming ipi
            atomic_or(&ipi_data[h], (1u << ipi));
        }
    }

    mb();
#if RISCV_M_MODE
    clint_send_ipis(&hart_mask);
#else
    sbi_send_ipis(&hart_mask);
#endif

    return NO_ERROR;
}

// software triggered exceptions, used for cross-cpu calls
enum handler_return riscv_software_exception(void) {
    uint curr_cpu = arch_curr_cpu_num();

#if RISCV_M_MODE
    uint ch = riscv_current_hart();
    clint_ipi_clear(ch);
#else
    sbi_clear_ipi();
#endif

    rmb();
    int reason = atomic_swap(&ipi_data[curr_cpu], 0);
    LTRACEF("cpu %u reason %#x\n", curr_cpu, reason);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (reason & (1u << MP_IPI_RESCHEDULE)) {
        ret = mp_mbx_reschedule_irq();
        reason &= ~(1u << MP_IPI_RESCHEDULE);
    }
    if (reason & (1u << MP_IPI_GENERIC)) {
        panic("unimplemented MP_IPI_GENERIC\n");
        reason &= ~(1u << MP_IPI_GENERIC);
    }

    if (unlikely(reason)) {
        TRACEF("unhandled ipi cause %#x, cpu %u\n", reason, curr_cpu);
        panic("stopping");
    }

    return ret;
}

// called in very early percpu bringup
void riscv_configure_percpu_mp_early(uint hart_id, uint cpu_num) {
    cpu_to_hart_map[cpu_num] = hart_id;
    wmb();
}

// called from assembly
void riscv_secondary_entry(uint hart_id, uint __unused, uint cpu_id);
void riscv_secondary_entry(uint hart_id, uint __unused, uint cpu_id) {
    // basic bootstrapping of this cpu
    riscv_early_init_percpu();

    if (unlikely(arch_curr_cpu_num() >= SMP_MAX_CPUS)) {
        while (1) {
            arch_idle();
        }
    }

    // spin here waiting for the main cpu to release us
    spin_lock(&boot_cpu_lock);
    spin_unlock(&boot_cpu_lock);

#if RISCV_MMU
    // let the mmu code configure per cpu bits
    riscv_mmu_init_secondaries();
#endif

    // run early secondary cpu init routines up to the threading level
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_THREADING - 1);

    // run threading level initialization on this cpu
    riscv_init_percpu();

    dprintf(INFO, "RISCV: secondary hart coming up: mvendorid %#lx marchid %#lx mimpid %#lx mhartid %#x\n",
            riscv_get_mvendorid(), riscv_get_marchid(),
            riscv_get_mimpid(), riscv_current_hart());

    lk_secondary_cpu_entry();
}

// platform hands the arch layer a list of harts to start, these will be
// started later in riscv_boot_secondaries()
void riscv_set_secondary_harts_to_start(const uint *harts, size_t count) {
    harts_to_boot_count = MIN(count, SMP_MAX_CPUS);
    memcpy(harts_to_boot, harts, harts_to_boot_count * sizeof(harts[0]));
}

// start any secondary cpus we are set to start. called on the boot processor
void riscv_boot_secondaries(void) {
    // if theres nothing to start, abort here
    if (harts_to_boot_count == 0) {
        dprintf(INFO, "RISCV: No secondary harts to start\n");
        return;
    }

    lk_init_secondary_cpus(harts_to_boot_count);

#if RISCV_M_MODE
    dprintf(INFO, "RISCV: Releasing all secondary harts from purgatory\n");

    // For machine mode, we simply unblock all the trapped cpus in start.S by releasing
    // the boot_cpu_lock. Via some mechanism or other, all of the cpus should have already entered
    // start.S via the main entry point for this to work. This is the default behavior when
    // running on QEMU, for example.
    spin_unlock(&boot_cpu_lock);
#else
    dprintf(INFO, "RISCV: Going to try to start %d secondary harts\n", harts_to_boot_count);

    // May as well release the boot cpu lock now, since there's no reason to hold back secondaries we
    // haven't started yet.
    spin_unlock(&boot_cpu_lock);

    // use SBI HSM to boot the secondaries
    uint boot_hart = riscv_current_hart();
    for (uint i = 0; i < harts_to_boot_count; i++) {
        // skip the boot hart
        if (harts_to_boot[i] != boot_hart) {
            dprintf(INFO, "RISCV: using SBI to start hart %u at %#lx\n", harts_to_boot[i], _start_physical);
            sbi_boot_hart(harts_to_boot[i], _start_physical, 0);

            // Pause a little bit here to give the secondary an opportunity to start. Not strictly
            // needed but helps the boot output flow without much of a boot penalty. Remove
            // if it's a problem.
            thread_sleep(50);
        }
    }
#endif
}


#endif
