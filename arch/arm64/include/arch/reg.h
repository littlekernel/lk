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
// Simple load/store instructions with a basic addressing mode of either
// [reg], [reg, #imm] or [reg, reg], with no writeback are guaranteed to be
// fully decoded into ESR_EL2 for hypervisor assist.

#define ARCH_MMIO_READ_WRITE_OVERRIDE 1

#define _ARCH_MMIO_READ8(addr) ({ \
    uint8_t val; \
    __asm__ volatile("ldrb %w0, %1" : "=r"(val) : "m"(*(addr)) : "memory"); \
    val; \
})
#define _ARCH_MMIO_READ16(addr) ({ \
    uint16_t val; \
    __asm__ volatile("ldrh %w0, %1" : "=r"(val) : "m"(*(addr)) : "memory"); \
    val; \
})
#define _ARCH_MMIO_READ32(addr) ({ \
    uint32_t val; \
    __asm__ volatile("ldr %w0, %1" : "=r"(val) : "m"(*(addr)) : "memory"); \
    val; \
})
#define _ARCH_MMIO_READ64(addr) ({ \
    uint64_t val; \
    __asm__ volatile("ldr %0, %1" : "=r"(val) : "m"(*(addr)) : "memory"); \
    val; \
})

#define _ARCH_MMIO_WRITE8(addr, val) \
    __asm__ volatile("strb %w1, %0" : "=m"(*(addr)) : "r"(val) : "memory")
#define _ARCH_MMIO_WRITE16(addr, val) \
    __asm__ volatile("strh %w1, %0" : "=m"(*(addr)): "r"(val) : "memory")
#define _ARCH_MMIO_WRITE32(addr, val) \
    __asm__ volatile("str %w1, %0" : "=m"(*(addr)) : "r"(val) : "memory")
#define _ARCH_MMIO_WRITE64(addr, val) \
    __asm__ volatile("str %1, %0" : "=m"(*(addr)) : "r"(val) : "memory")

