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
void sun4m_scc_early_init(void);
void sun4m_scc_init(void);
void sun4m_timer_early_init(void);
void sun4m_timer_init(void);

handler_return sun4m_timer_irq(void *arg);

// Global interrupt sources on sun4m platform
// NOTE: not the same as the interrupt levels 0-15 on the cpu
enum {
    // VME interrupt levels, 0 - 6,
    SUN4M_VME_IRQ_0 = 0,
    // SBUS interrupt levels, 7 - 13
    SUN4M_SBUS_IRQ_0 = 7,

    // other global source
    SUN4M_KEYBOARD_IRQ = 14,
    SUN4M_SCC_IRQ = 15,
    SUN4M_ETH_IRQ = 16,
    SUN4M_AUDIO_IRQ = 17,
    SUN4M_SCSI_IRQ = 18,
    SUN4M_TIMER_GLOBAL_IRQ = 19,
    SUN4M_VIDEO_IRQ = 20,
    SUN4M_MODULE_IRQ = 21,
    SUN4M_FLOPPY_IRQ = 22,
    // 23-26 reserved
    SUN4M_VME_ASYNC_ERROR_IRQ = 27,
    SUN4M_ECC_ERROR_IRQ = 28,
    SUN4M_MS_BUFFER_ERROR_IRQ = 29,
    SUN4M_MODULE_ERROR_IRQ = 30,
    // 31 reserved

};

// when we get an irq on level 14 it's a local percpu timer interrupt.
const uint32_t SUN4M_TIMER_PERCPU_IRQ_LEVEL = 14;

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
inline uint8_t sparc_read_physical_8(uint64_t addr) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    const uint8_t asi = 0x20 + ((addr >> 32) & 0xf);
    uint8_t val;

    ASI_SWITCH_READ("lduba")

    return val;
}

inline void sparc_write_physical_8(uint64_t addr, uint8_t val) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    const uint8_t asi = 0x20 + ((addr >> 32) & 0xf);

    ASI_SWITCH_WRITE("stba")
}

inline uint32_t sparc_read_physical_32(uint64_t addr) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    const uint8_t asi = 0x20 + ((addr >> 32) & 0xf);
    uint32_t val;

    ASI_SWITCH_READ("lda")

    return val;
}

inline void sparc_write_physical_32(uint64_t addr, uint32_t val) {
    // Select ASI prefix (0x20-0x2f) based on top 4 bits of 36-bit address
    const uint8_t asi = 0x20 + ((addr >> 32) & 0xf);

    ASI_SWITCH_WRITE("sta")
}
