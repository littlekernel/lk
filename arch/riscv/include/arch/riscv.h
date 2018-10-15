/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
