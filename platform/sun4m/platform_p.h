//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <stdint.h>

#include <lk/compiler.h>
#include <platform/interrupts.h>

void sun4m_intc_early_init(void);
void sun4m_intc_init(void);
void sun4m_timer_early_init(void);
void sun4m_timer_init(void);
handler_return sun4m_timer_irq();

// Macros to generate ASI switch statements for control space access
#define ASI_SWITCH_READ(instr) \
    switch (asi) { \
        case 0x20: __asm__ volatile(instr " [%1] 0x20, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x21: __asm__ volatile(instr " [%1] 0x21, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x22: __asm__ volatile(instr " [%1] 0x22, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x23: __asm__ volatile(instr " [%1] 0x23, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x24: __asm__ volatile(instr " [%1] 0x24, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x25: __asm__ volatile(instr " [%1] 0x25, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x26: __asm__ volatile(instr " [%1] 0x26, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x27: __asm__ volatile(instr " [%1] 0x27, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x28: __asm__ volatile(instr " [%1] 0x28, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x29: __asm__ volatile(instr " [%1] 0x29, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x2a: __asm__ volatile(instr " [%1] 0x2a, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x2b: __asm__ volatile(instr " [%1] 0x2b, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x2c: __asm__ volatile(instr " [%1] 0x2c, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x2d: __asm__ volatile(instr " [%1] 0x2d, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x2e: __asm__ volatile(instr " [%1] 0x2e, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
        case 0x2f: __asm__ volatile(instr " [%1] 0x2f, %0" : "=r"(val) : "r"((uint32_t)addr) : "memory"); break; \
    }

#define ASI_SWITCH_WRITE(instr) \
    switch (asi) { \
        case 0x20: __asm__ volatile(instr " %0, [%1] 0x20" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x21: __asm__ volatile(instr " %0, [%1] 0x21" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x22: __asm__ volatile(instr " %0, [%1] 0x22" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x23: __asm__ volatile(instr " %0, [%1] 0x23" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x24: __asm__ volatile(instr " %0, [%1] 0x24" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x25: __asm__ volatile(instr " %0, [%1] 0x25" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x26: __asm__ volatile(instr " %0, [%1] 0x26" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x27: __asm__ volatile(instr " %0, [%1] 0x27" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x28: __asm__ volatile(instr " %0, [%1] 0x28" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x29: __asm__ volatile(instr " %0, [%1] 0x29" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x2a: __asm__ volatile(instr " %0, [%1] 0x2a" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x2b: __asm__ volatile(instr " %0, [%1] 0x2b" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x2c: __asm__ volatile(instr " %0, [%1] 0x2c" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x2d: __asm__ volatile(instr " %0, [%1] 0x2d" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x2e: __asm__ volatile(instr " %0, [%1] 0x2e" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
        case 0x2f: __asm__ volatile(instr " %0, [%1] 0x2f" : : "r"(val), "r"((uint32_t)addr) : "memory"); break; \
    }

// read/write physical memory with ASI-based address space selection
inline uint8_t sparc_read_physical_8(uint64_t addr) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    uint8_t asi = 0x20 + ((addr >> 32) & 0xf);
    uint8_t val;

    ASI_SWITCH_READ("lduba")

    return val;
}

inline void sparc_write_physical_8(uint64_t addr, uint8_t val) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    uint8_t asi = 0x20 + ((addr >> 32) & 0xf);

    ASI_SWITCH_WRITE("stba")
}

inline uint32_t sparc_read_physical_32(uint64_t addr) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    uint8_t asi = 0x20 + ((addr >> 32) & 0xf);
    uint32_t val;

    ASI_SWITCH_READ("lda")

    return val;
}

inline void sparc_write_physical_32(uint64_t addr, uint32_t val) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    uint8_t asi = 0x20 + ((addr >> 32) & 0xf);

    ASI_SWITCH_WRITE("sta")
}
