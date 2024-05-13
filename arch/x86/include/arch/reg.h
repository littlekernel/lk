/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

// Accessor routines for this architecture, used in include/reg.h
//
// Simple mov instructions that directly access the mmio address using the
// precise width needed in one instruction.
//
// NOTE: this may not be strict enough, and it may be necessary to load the
// address into a register and use a simple (reg) dereference, instead of a more
// complicated #imm(reg, reg, #imm) addressing mode.

#define ARCH_MMIO_READ_WRITE_OVERRIDE 1

#define _ARCH_MMIO_READ8(addr) ({ \
    uint8_t val; \
    __asm__ volatile("movb %1, %0" : "=q"(val) : "m"(*(addr)) : "memory"); \
    val; \
})
#define _ARCH_MMIO_READ16(addr) ({ \
    uint16_t val; \
    __asm__ volatile("movw %1, %0" : "=r"(val) : "m"(*(addr)) : "memory"); \
    val; \
})
#define _ARCH_MMIO_READ32(addr) ({ \
    uint32_t val; \
    __asm__ volatile("movl %1, %0" : "=r"(val) : "m"(*(addr)) : "memory"); \
    val; \
})
#if _LP64
#define _ARCH_MMIO_READ64(addr) ({ \
    uint64_t val; \
    __asm__ volatile("movq %1, %0" : "=r"(val) : "m"(*(addr)) : "memory"); \
    val; \
})
#endif

#define _ARCH_MMIO_WRITE8(addr, val) \
    __asm__ volatile("movb %1, %0" : "=m"(*(addr)) : "iq"(val) : "memory")
#define _ARCH_MMIO_WRITE16(addr, val) \
    __asm__ volatile("movw %1, %0" : "=m"(*(addr)) : "ir"(val) : "memory")
#define _ARCH_MMIO_WRITE32(addr, val) \
    __asm__ volatile("movl %1, %0" : "=m"(*(addr)) : "ir"(val) : "memory")
#if _LP64
#define _ARCH_MMIO_WRITE64(addr, val) \
    __asm__ volatile("movq %1, %0" : "=m"(*(addr)) : "er"(val) : "memory")
#endif
