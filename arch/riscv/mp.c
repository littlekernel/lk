/*
 * Copyright (c) 2019 Elliot Berman
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/reg.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>

#include <arch/ops.h>
#include <arch/mp.h>
#include <arch/riscv/clint.h>

#if WITH_SMP

#define LOCAL_TRACE 0

int hart_cpu_map[SMP_MAX_CPUS] = { [0 ... SMP_MAX_CPUS-1] = -1 };

// bitmap of IPIs queued per cpu
static volatile int ipi_data[SMP_MAX_CPUS];

status_t arch_mp_send_ipi(mp_cpu_mask_t target, mp_ipi_t ipi) {
    LTRACEF("target 0x%x, ipi %u\n", target, ipi);

    mp_cpu_mask_t m = target;
    ulong hart_mask = 0;
    for (uint c = 0; c < SMP_MAX_CPUS && m; c++, m >>= 1) {
        int h = hart_cpu_map[c];
        if (m & 1) {
            hart_mask |= (1ul << h);
        }
        // set the ipi_data based on the incoming ipi
        atomic_or(&ipi_data[c], (1u << ipi));
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
    LTRACEF("reason %#x\n", reason);

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

void arch_mp_init_percpu(void) {
    dprintf(INFO, "RISCV: Booting hart%d (cpu%d)\n", riscv_current_hart(), arch_curr_cpu_num());
    riscv_csr_set(RISCV_CSR_XIE, RISCV_CSR_XIE_SIE);
}

#endif
