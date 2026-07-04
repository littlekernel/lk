//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#pragma once

// bits in the PSR register
#define SPARC_PSR_CWP_MASK  (0x1f)      // current window pointer
#define SPARC_PSR_ET        (0x1 << 5)  // enbale traps
#define SPARC_PSR_PS        (0x1 << 6)  // previous supervisor
#define SPARC_PSR_S         (0x1 << 7)  // supervisor
#define SPARC_PSR_PIL_MASK  (0x7 << 8)  // processor interrupt level
#define SPARC_PSR_EF        (0x1 << 12) // enable floating point
#define SPARC_PSR_EC        (0x1 << 13) // enable co-processor
#define SPARC_PSR_CC_MASK   (0xf << 20) // NZVC condition codes
#define SPARC_PSR_VER_MASK  (0xf << 24) // implementation version
#define SPARC_PSR_IMPL_MASK (0xf << 28) // implementation ID

#if !__ASSEMBLER__

#include <stdint.h>

enum sparc_trap_type {
    SPARC_TRAP_RESET                        = 0x00,
    SPARC_TRAP_INSTRUCTION_ACCESS_EXCEPTION = 0x01,
    SPARC_TRAP_ILLEGAL_INSTRUCTION          = 0x02,
    SPARC_TRAP_PRIVILEGED_INSTRUCTION       = 0x03,
    SPARC_TRAP_FP_DISABLED                  = 0x04,
    SPARC_TRAP_WINDOW_OVERFLOW              = 0x05,
    SPARC_TRAP_WINDOW_UNDERFLOW             = 0x06,
    SPARC_TRAP_MEM_ADDRESS_NOT_ALIGNED      = 0x07,
    SPARC_TRAP_FP_EXCEPTION                 = 0x08,
    SPARC_TRAP_DATA_ACCESS_EXCEPTION        = 0x09,
    SPARC_TRAP_TAG_OVERFLOW                 = 0x0a,
    SPARC_TRAP_WATCHPOINT_DETECTED          = 0x0b,
    SPARC_TRAP_IRQ_1                        = 0x11,
    SPARC_TRAP_IRQ_2                        = 0x12,
    SPARC_TRAP_IRQ_3                        = 0x13,
    SPARC_TRAP_IRQ_4                        = 0x14,
    SPARC_TRAP_IRQ_5                        = 0x15,
    SPARC_TRAP_IRQ_6                        = 0x16,
    SPARC_TRAP_IRQ_7                        = 0x17,
    SPARC_TRAP_IRQ_8                        = 0x18,
    SPARC_TRAP_IRQ_9                        = 0x19,
    SPARC_TRAP_IRQ_10                       = 0x1a,
    SPARC_TRAP_IRQ_11                       = 0x1b,
    SPARC_TRAP_IRQ_12                       = 0x1c,
    SPARC_TRAP_IRQ_13                       = 0x1d,
    SPARC_TRAP_IRQ_14                       = 0x1e,
    SPARC_TRAP_IRQ_15                       = 0x1f,
    SPARC_TRAP_CP_DISABLED                  = 0x24,
    SPARC_TRAP_CP_EXCEPTION                 = 0x28,
};

static inline uint32_t sparc_read_psr(void) {
    uint32_t psr;
    __asm__ volatile("rd %%psr, %0" : "=r"(psr));
    return psr;
}

static inline void sparc_write_psr(uint32_t psr) {
    __asm__ volatile("wr %0, 0, %%psr\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     :
                     : "r"(psr)
                     : "memory");
}

#endif
