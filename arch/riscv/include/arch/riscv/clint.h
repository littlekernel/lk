/*
 * Copyright (c) 2019 Elliot Berman
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <config.h>

#include <lk/reg.h>
#include <arch/arch_ops.h>

#if RISCV_M_MODE

// platform must define these
#ifndef ARCH_RISCV_CLINT_BASE
#error Platform must define ARCH_RISCV_CLINT_BASE
#endif
#ifndef ARCH_RISCV_MTIME_RATE
#error Platform must define ARCH_RISCV_MTIME_RATE
#endif

#define CLINT_MSIP(h)   (ARCH_RISCV_CLINT_BASE + (4 * (h)))
#define CLINT_MTIMECMP(h)   (ARCH_RISCV_CLINT_BASE + 0x4000 + (8 * (h)))
#define CLINT_MTIME (ARCH_RISCV_CLINT_BASE + 0xbff8)

static inline void clint_ipi_send(unsigned long target_hart) {
    if (target_hart >= SMP_MAX_CPUS)
        return;

    *REG32(CLINT_MSIP(target_hart)) = 1;
}

static inline void clint_ipi_clear(unsigned long target_hart) {
    if (target_hart >= SMP_MAX_CPUS)
        return;

    *REG32(CLINT_MSIP(target_hart)) = 0;
}

static inline void clint_set_timer(uint64_t ticks) {
    *REG64(CLINT_MTIMECMP(riscv_current_hart())) = ticks;
}


static inline uint64_t riscv_get_time(void) {
    return *REG64(CLINT_MTIME);
}

static inline void clint_send_ipis(const unsigned long *hart_mask) {
    unsigned long cur_hart = riscv_current_hart(), h, m = *hart_mask;
    for (h = 0; h < SMP_MAX_CPUS && m; h++, m >>= 1) {
        if ((m & 1) && (h != cur_hart)) {
            clint_ipi_send(h);
        }
    }

    if (*hart_mask & (1 << riscv_current_hart())) {
        clint_ipi_send(cur_hart);
    }
}

#endif
