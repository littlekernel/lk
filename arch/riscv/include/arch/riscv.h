/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <config.h>
#include <arch/defines.h>

#define RISCV_USER_OFFSET   (0u)
#define RISCV_SUPER_OFFSET  (1u)
#define RISCV_HYPER_OFFSET  (2u)
#define RISCV_MACH_OFFSET   (3u)

#if RISCV_M_MODE
# define RISCV_XMODE_OFFSET     (RISCV_MACH_OFFSET)
# define RISCV_XRET             mret
#elif RISCV_S_MODE
# define RISCV_XMODE_OFFSET     (RISCV_SUPER_OFFSET)
# define RISCV_XRET             sret
#else
# error Unrecognized RISC-V privilege level selected
#endif

#define RISCV_CSR_XMODE_BITS     (RISCV_XMODE_OFFSET << 8)

// These CSRs are only in user CSR space (still readable by all modes though)
#define RISCV_CSR_CYCLE     (0xc00)
#define RISCV_CSR_TIME      (0xc01)
#define RISCV_CSR_INSRET    (0xc02)
#define RISCV_CSR_CYCLEH    (0xc80)
#define RISCV_CSR_TIMEH     (0xc81)
#define RISCV_CSR_INSRETH   (0xc82)

#define RISCV_CSR_XSTATUS   (0x000 | RISCV_CSR_XMODE_BITS)
#define RISCV_CSR_XIE       (0x004 | RISCV_CSR_XMODE_BITS)
#define RISCV_CSR_XTVEC     (0x005 | RISCV_CSR_XMODE_BITS)
#define RISCV_CSR_XSCRATCH  (0x040 | RISCV_CSR_XMODE_BITS)
#define RISCV_CSR_XEPC      (0x041 | RISCV_CSR_XMODE_BITS)
#define RISCV_CSR_XCAUSE    (0x042 | RISCV_CSR_XMODE_BITS)
#define RISCV_CSR_XTVAL     (0x043 | RISCV_CSR_XMODE_BITS)
#define RISCV_CSR_XIP       (0x044 | RISCV_CSR_XMODE_BITS)

#if RISCV_M_MODE // Machine-mode only CSRs
#define RISCV_CSR_MCYCLE    (0xb00)
#define RISCV_CSR_MVENDORID (0xf11)
#define RISCV_CSR_MARCHID   (0xf12)
#define RISCV_CSR_MIMPID    (0xf13)
#define RISCV_CSR_MHARTID   (0xf14)
#define RISCV_CSR_MISA      (0x301)
#endif /* RISCV_M_MODE */

#define RISCV_CSR_XSTATUS_IE    (1u << (RISCV_XMODE_OFFSET + 0))
#define RISCV_CSR_XSTATUS_PIE   (1u << (RISCV_XMODE_OFFSET + 4))

#define RISCV_CSR_XIE_SIE       (1u << (RISCV_XMODE_OFFSET + 0))
#define RISCV_CSR_XIE_TIE       (1u << (RISCV_XMODE_OFFSET + 4))
#define RISCV_CSR_XIE_EIE       (1u << (RISCV_XMODE_OFFSET + 8))

#define RISCV_CSR_XIP_SIP       (1u << (RISCV_XMODE_OFFSET + 0))
#define RISCV_CSR_XIP_TIP       (1u << (RISCV_XMODE_OFFSET + 4))
#define RISCV_CSR_XIP_EIP       (1u << (RISCV_XMODE_OFFSET + 8))

#define RISCV_EXCEPTION_XSWI        (RISCV_XMODE_OFFSET)
#define RISCV_EXCEPTION_XTIM        (4 + RISCV_XMODE_OFFSET)
#define RISCV_EXCEPTION_XEXT        (8 + RISCV_XMODE_OFFSET)

#ifndef ASSEMBLY
#define __ASM_STR(x)    #x

#define riscv_csr_clear(csr, bits) \
({ \
    ulong __val = bits; \
    __asm__ volatile( \
        "csrc   " __ASM_STR(csr) ", %0" \
        :: "rK" (__val) \
        : "memory"); \
})

#define riscv_csr_read_clear(csr, bits) \
({ \
    ulong __val = bits; \
    ulong __val_out; \
    __asm__ volatile( \
        "csrrc   %0, " __ASM_STR(csr) ", %1" \
        : "=r"(__val_out) \
        : "rK" (__val) \
        : "memory"); \
    __val_out; \
})

#define riscv_csr_set(csr, bits) \
({ \
    ulong __val = bits; \
    __asm__ volatile( \
        "csrs   " __ASM_STR(csr) ", %0" \
        :: "rK" (__val) \
        : "memory"); \
})

#define riscv_csr_read(csr) \
({ \
    ulong __val; \
    __asm__ volatile( \
        "csrr   %0, " __ASM_STR(csr) \
        : "=r" (__val) \
        :: "memory"); \
    __val; \
})

#define riscv_csr_write(csr, val) \
({ \
    ulong __val = (ulong)val; \
    __asm__ volatile( \
        "csrw   " __ASM_STR(csr) ", %0" \
        :: "rK" (__val) \
        : "memory"); \
    __val; \
})

extern int cpu_to_hart_map[];

struct riscv_percpu {
    uint cpu_num;
    uint hart_id;
} __ALIGNED(CACHE_LINE);

extern struct riscv_percpu percpu[RISCV_MAX_HARTS];

static inline struct riscv_percpu *riscv_get_percpu(void) {
    return (struct riscv_percpu *)riscv_csr_read(RISCV_CSR_XSCRATCH);
}

static inline uint riscv_current_hart(void) {
#if RISCV_M_MODE
    return riscv_csr_read(RISCV_CSR_MHARTID);
#else
    return riscv_get_percpu()->hart_id;
#endif
}

void riscv_set_secondary_count(int count);

void riscv_exception_entry(void);
enum handler_return riscv_timer_exception(void);

#endif /* ASSEMBLY */
