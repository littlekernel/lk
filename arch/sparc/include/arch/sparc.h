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
    SPARC_TRAP_RESET = 0x00,
    SPARC_TRAP_INSTRUCTION_ACCESS_EXCEPTION = 0x01,
    SPARC_TRAP_ILLEGAL_INSTRUCTION = 0x02,
    SPARC_TRAP_PRIVILEGED_INSTRUCTION = 0x03,
    SPARC_TRAP_FP_DISABLED = 0x04,
    SPARC_TRAP_WINDOW_OVERFLOW = 0x05,
    SPARC_TRAP_WINDOW_UNDERFLOW = 0x06,
    SPARC_TRAP_MEM_ADDRESS_NOT_ALIGNED = 0x07,
    SPARC_TRAP_FP_EXCEPTION = 0x08,
    SPARC_TRAP_DATA_ACCESS_EXCEPTION = 0x09,
    SPARC_TRAP_TAG_OVERFLOW = 0x0a,
    SPARC_TRAP_WATCHPOINT_DETECTED = 0x0b,
    SPARC_TRAP_IRQ_1 = 0x11,
    SPARC_TRAP_IRQ_2 = 0x12,
    SPARC_TRAP_IRQ_3 = 0x13,
    SPARC_TRAP_IRQ_4 = 0x14,
    SPARC_TRAP_IRQ_5 = 0x15,
    SPARC_TRAP_IRQ_6 = 0x16,
    SPARC_TRAP_IRQ_7 = 0x17,
    SPARC_TRAP_IRQ_8 = 0x18,
    SPARC_TRAP_IRQ_9 = 0x19,
    SPARC_TRAP_IRQ_10 = 0x1a,
    SPARC_TRAP_IRQ_11 = 0x1b,
    SPARC_TRAP_IRQ_12 = 0x1c,
    SPARC_TRAP_IRQ_13 = 0x1d,
    SPARC_TRAP_IRQ_14 = 0x1e,
    SPARC_TRAP_IRQ_15 = 0x1f,
    SPARC_TRAP_CP_DISABLED = 0x24,
    SPARC_TRAP_CP_EXCEPTION = 0x28,
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

// Macros to generate ASI switch statements for control space access
#define ASI_SWITCH_READ(instr)                                                                     \
    switch (asi) {                                                                                 \
        case 0x20:                                                                                 \
            __asm__ volatile(instr " [%1] 0x20, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x21:                                                                                 \
            __asm__ volatile(instr " [%1] 0x21, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x22:                                                                                 \
            __asm__ volatile(instr " [%1] 0x22, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x23:                                                                                 \
            __asm__ volatile(instr " [%1] 0x23, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x24:                                                                                 \
            __asm__ volatile(instr " [%1] 0x24, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x25:                                                                                 \
            __asm__ volatile(instr " [%1] 0x25, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x26:                                                                                 \
            __asm__ volatile(instr " [%1] 0x26, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x27:                                                                                 \
            __asm__ volatile(instr " [%1] 0x27, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x28:                                                                                 \
            __asm__ volatile(instr " [%1] 0x28, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x29:                                                                                 \
            __asm__ volatile(instr " [%1] 0x29, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2a:                                                                                 \
            __asm__ volatile(instr " [%1] 0x2a, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2b:                                                                                 \
            __asm__ volatile(instr " [%1] 0x2b, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2c:                                                                                 \
            __asm__ volatile(instr " [%1] 0x2c, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2d:                                                                                 \
            __asm__ volatile(instr " [%1] 0x2d, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2e:                                                                                 \
            __asm__ volatile(instr " [%1] 0x2e, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2f:                                                                                 \
            __asm__ volatile(instr " [%1] 0x2f, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
    }

#define ASI_SWITCH_WRITE(instr)                                                                    \
    switch (asi) {                                                                                 \
        case 0x20:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x20" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x21:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x21" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x22:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x22" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x23:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x23" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x24:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x24" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x25:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x25" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x26:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x26" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x27:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x27" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x28:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x28" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x29:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x29" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2a:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x2a" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2b:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x2b" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2c:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x2c" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2d:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x2d" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2e:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x2e" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
        case 0x2f:                                                                                 \
            __asm__ volatile(instr " %0, [%1] 0x2f" : : "r"(val), "r"((uint32_t)addr) : "memory"); \
            break;                                                                                 \
    }

// read/write physical memory with ASI-based address space selection
static inline uint8_t sparc_read_physical_8(uint64_t addr) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    const uint8_t asi = 0x20 + ((addr >> 32) & 0xf);
    uint8_t val;

    ASI_SWITCH_READ("lduba")

    return val;
}

static inline void sparc_write_physical_8(uint64_t addr, uint8_t val) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    const uint8_t asi = 0x20 + ((addr >> 32) & 0xf);

    ASI_SWITCH_WRITE("stba")
}

static inline uint32_t sparc_read_physical_32(uint64_t addr) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    const uint8_t asi = 0x20 + ((addr >> 32) & 0xf);
    uint32_t val;

    ASI_SWITCH_READ("lda")

    return val;
}

static inline void sparc_write_physical_32(uint64_t addr, uint32_t val) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    const uint8_t asi = 0x20 + ((addr >> 32) & 0xf);

    ASI_SWITCH_WRITE("sta")
}


#endif
