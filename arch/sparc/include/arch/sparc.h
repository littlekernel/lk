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
