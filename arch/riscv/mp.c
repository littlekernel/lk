/*
 * Copyright (c) 2019 Elliot Berman
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/reg.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/main.h>

#include <arch/atomic.h>
#include <arch/ops.h>
#include <arch/mp.h>
#include <arch/riscv/clint.h>

#include "riscv_priv.h"

#if WITH_SMP

#define LOCAL_TRACE 0

// Highest supported HART has to at least be more than number of
// cpus we support. Generally they're the same, but some cpus may start
// at nonzero hart ids.
STATIC_ASSERT(RISCV_MAX_HARTS >= SMP_MAX_CPUS);

// boot hart has to be one of the valid ones
STATIC_ASSERT(RISCV_BOOT_HART < RISCV_MAX_HARTS);

// mapping of cpu -> hart
int cpu_to_hart_map[SMP_MAX_CPUS] = {
    [0] = RISCV_BOOT_HART,       // boot cpu is always logical 0
    [1 ... SMP_MAX_CPUS-1] = -1, // other hart cpus are assigned dynamically
};

// list of IPIs queued per cpu
static volatile int ipi_data[RISCV_MAX_HARTS];

static spin_lock_t boot_cpu_lock = 1;
volatile int secondaries_to_init = SMP_MAX_CPUS - 1;

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

enum handler_return riscv_software_exception(void) {
    uint ch = riscv_current_hart();

#if RISCV_M_MODE
    clint_ipi_clear(ch);
#else
    sbi_clear_ipi();
#endif

    rmb();
    int reason = atomic_swap(&ipi_data[ch], 0);
    LTRACEF("ch %u reason %#x\n", ch, reason);

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
        TRACEF("unhandled ipi cause %#x, hartid %#x\n", reason, ch);
        panic("stopping");
    }

    return ret;
}

void riscv_secondary_entry(void) {
    // basic bootstrapping of this cpu
    riscv_early_init_percpu();

    // assign secondary cpu an id, starting at cpu 1
    // cpu 0 is always the boot hart
    static volatile int secondary_cpu_id = 1;
    int myid = atomic_add(&secondary_cpu_id, 1);
    uint hart = riscv_current_hart();
    cpu_to_hart_map[myid] = hart;
    riscv_get_percpu()->cpu_num = myid;
    wmb();

    if (unlikely(arch_curr_cpu_num() >= SMP_MAX_CPUS)) {
        while (1) {
            arch_idle();
        }
    }

    // spin here waiting for the main cpu to release us
    spin_lock(&boot_cpu_lock);
    spin_unlock(&boot_cpu_lock);

    // run early secondary cpu init routines up to the threading level
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_THREADING - 1);

    // run threading level initialization on this cpu
    riscv_init_percpu();

    dprintf(INFO, "RISCV: secondary hart coming up: mvendorid %#lx marchid %#lx mimpid %#lx mhartid %#x\n",
            riscv_get_mvendorid(), riscv_get_marchid(),
            riscv_get_mimpid(), riscv_current_hart());

    // atomic_add(&secondaries_to_init, -1);
    // arch_mp_send_ipi(1 << 0, MP_IPI_GENERIC); // wake up hart0 to let it know this CPU has come up

    lk_secondary_cpu_entry();
}

// platform can detect and set the number of cores to boot (optional)
void riscv_set_secondary_count(int count) {
    if (count > SMP_MAX_CPUS - 1) {
        count = SMP_MAX_CPUS - 1;
    }
    secondaries_to_init = count;
}

// start any secondary cpus we are set to start. called on the boot processor
void riscv_boot_secondaries(void) {
    lk_init_secondary_cpus(secondaries_to_init);

    LTRACEF("RISCV: Waiting for %d secondary harts to come up\n", secondaries_to_init);
    /* release the secondary cpus */
    spin_unlock(&boot_cpu_lock);

    // wait a second while the secondaries start
    //spin(1000000);

    // while (secondaries_to_init) arch_idle();
    // spin_lock(&boot_cpu_lock);
}


#endif
