/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

// Low level macros for accessing memory mapped hardware registers

// Each architecture has an ability to provide their own specific accessors, or use the default
#include <arch/reg.h>

// Newer style accessors, generally preferred due to virtual machine virtualization.
//
// If the arch didn't override it, use the default
#if !ARCH_MMIO_READ_WRITE_OVERRIDE
#define _MMIO_READ_DEFAULT(ptr) *ptr
#define _MMIO_WRITE_DEFAULT(ptr, val) *ptr = val

#define _ARCH_MMIO_READ8 _MMIO_READ_DEFAULT
#define _ARCH_MMIO_READ16 _MMIO_READ_DEFAULT
#define _ARCH_MMIO_READ32 _MMIO_READ_DEFAULT
#define _ARCH_MMIO_READ64 _MMIO_READ_DEFAULT
#define _ARCH_MMIO_WRITE8 _MMIO_WRITE_DEFAULT
#define _ARCH_MMIO_WRITE16 _MMIO_WRITE_DEFAULT
#define _ARCH_MMIO_WRITE32 _MMIO_WRITE_DEFAULT
#define _ARCH_MMIO_WRITE64 _MMIO_WRITE_DEFAULT
#endif

static inline uint8_t mmio_read8(volatile uint8_t *ptr) {
    return _ARCH_MMIO_READ8(ptr);
}

static inline uint16_t mmio_read16(volatile uint16_t *ptr) {
    return _ARCH_MMIO_READ16(ptr);
}

static inline uint32_t mmio_read32(volatile uint32_t *ptr) {
    return _ARCH_MMIO_READ32(ptr);
}

#if _LP64
static inline uint64_t mmio_read64(volatile uint64_t *ptr) {
    return _ARCH_MMIO_READ64(ptr);
}
#endif

// For architectures that do not need stricter accessor instructions
static inline void mmio_write8(volatile uint8_t *ptr, uint8_t val) {
    _ARCH_MMIO_WRITE8(ptr, val);
}

static inline void mmio_write16(volatile uint16_t *ptr, uint16_t val) {
    _ARCH_MMIO_WRITE16(ptr, val);
}

static inline void mmio_write32(volatile uint32_t *ptr, uint32_t val) {
    _ARCH_MMIO_WRITE32(ptr, val);
}

#if _LP64
static inline void mmio_write64(volatile uint64_t *ptr, uint64_t val) {
    _ARCH_MMIO_WRITE64(ptr, val);
}
#endif

// TODO: define C++ accessors here

// Older style accessors:
// NOTE: These are not generally VM safe, since some architectures require specific instruction
// forms to work properly in a trap-and-emulate situation.
#define REG64(addr) ((volatile uint64_t *)(uintptr_t)(addr))
#define REG32(addr) ((volatile uint32_t *)(uintptr_t)(addr))
#define REG16(addr) ((volatile uint16_t *)(uintptr_t)(addr))
#define REG8(addr) ((volatile uint8_t *)(uintptr_t)(addr))

#define RMWREG64(addr, startbit, width, val) mmio_write64((volatile void *)(addr), (mmio_read64((volatile void *)(addr)) & ~(((1<<(width)) - 1) << (startbit))) | ((val) << (startbit)))
#define RMWREG32(addr, startbit, width, val) mmio_write32((volatile void *)(addr), (mmio_read32((volatile void *)(addr)) & ~(((1<<(width)) - 1) << (startbit))) | ((val) << (startbit)))
#define RMWREG16(addr, startbit, width, val) mmio_write16((volatile void *)(addr), (mmio_read16((volatile void *)(addr)) & ~(((1<<(width)) - 1) << (startbit))) | ((val) << (startbit)))
#define RMWREG8(addr, startbit, width, val) mmio_write8((volatile void *)(addr), (mmio_read8((volatile void *)(addr)) & ~(((1<<(width)) - 1) << (startbit))) | ((val) << (startbit)))

// Linux-style accessors
#define readb(a) mmio_read8((volatile uint8_t *)(a))
#define readw(a) mmio_read16((volatile uint16_t *)(a))
#define readl(a) mmio_read32((volatile uint32_t *)(a))
#if _LP64
#define readq(a) mmio_read64((volatile uint64_t *)(a))
#endif
#define writeb(v, a) mmio_write8((volatile uint8_t *)(a), v)
#define writew(v, a) mmio_write16((volatile uint16_t *)(a), v)
#define writel(v, a) mmio_write32((volatile uint32_t *)(a), v)
#if _LP64
#define writeq(v, a) mmio_write64((volatile uint64_t *)(a), v)
#endif


