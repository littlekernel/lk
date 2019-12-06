/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define RISCV_STATUS_SIE        (1u << 2)
#define RISCV_STATUS_MIE        (1u << 3)
#define RISCV_STATUS_MPIE       (1u << 7)
#define RISCV_STATUS_MPP_MASK   (3u << 11)

#define RISCV_MIE_MSIE          (1u << 3)
#define RISCV_MIE_MTIE          (1u << 7)
#define RISCV_MIE_SEIE          (1u << 9)
#define RISCV_MIE_MEIE          (1u << 11)

#define RISCV_MIP_MSIP          (1u << 3)
#define RISCV_MIP_MTIP          (1u << 7)
#define RISCV_MIP_MEIP          (1u << 11)

#define RISCV_MCAUSE_INT        (1u << 31)

#define riscv_csr_clear(csr, bits) \
({ \
    ulong __val = bits; \
    __asm__ volatile( \
        "csrc   " #csr ", %0" \
        :: "rK" (__val) \
        : "memory"); \
})

#define riscv_csr_read_clear(csr, bits) \
({ \
    ulong __val = bits; \
    ulong __val_out; \
    __asm__ volatile( \
        "csrrc   %0, " #csr ", %1" \
        : "=r"(__val_out) \
        : "rK" (__val) \
        : "memory"); \
    __val_out; \
})

#define riscv_csr_set(csr, bits) \
({ \
    ulong __val = bits; \
    __asm__ volatile( \
        "csrs   " #csr ", %0" \
        :: "rK" (__val) \
        : "memory"); \
})

#define riscv_csr_read(csr) \
({ \
    ulong __val; \
    __asm__ volatile( \
        "csrr   %0, " #csr \
        : "=r" (__val) \
        :: "memory"); \
    __val; \
})

#define riscv_csr_write(csr, val) \
({ \
    ulong __val = (ulong)val; \
    __asm__ volatile( \
        "csrw   " #csr ", %0" \
        :: "rK" (__val) \
        : "memory"); \
    __val; \
})

void riscv_exception_entry(void);
enum handler_return riscv_timer_exception(void);
