//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <stdint.h>

void sun4m_intc_early_init(void);
void sun4m_intc_init(void);
void sun4m_timer_early_init(void);
void sun4m_timer_init(void);

// read/write to control space (0xf.0000.0000+)
static inline uint8_t read_control_space_8(uint64_t base, uint32_t reg) {
    // use ASI 0x2f to access control space (0xf.0000.0000+)
    uint8_t val;
    __asm__ volatile("lduba [%1] 0x2f, %0" : "=r"(val) : "r"((uint32_t)base + reg) : "memory");

    return val;
}

static inline void write_control_space_8(uint64_t base, uint32_t reg, uint8_t val) {
    __asm__ volatile("stba %0, [%1] 0x2f" : : "r"(val), "r"((uint32_t)base + reg) : "memory");
}


static inline uint32_t read_control_space_32(uint64_t base, uint32_t reg) {
    // use ASI 0x2f to access control space (0xf.0000.0000+)
    uint32_t val;
    __asm__ volatile("lda [%1] 0x2f, %0" : "=r"(val) : "r"((uint32_t)base + reg) : "memory");

    return val;
}

static inline void write_control_space_32(uint64_t base, uint32_t reg, uint32_t val) {
    __asm__ volatile("sta %0, [%1] 0x2f" : : "r"(val), "r"((uint32_t)base + reg) : "memory");
}
