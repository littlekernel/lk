/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY
#include <lk/compiler.h>
#include <stdint.h>
#include <sys/types.h>

#define GEN_CP_REG_FUNCS(regname, regnum, sel) \
static inline __ALWAYS_INLINE uint32_t mips_read_##regname(void) { \
    uint32_t val; \
    __asm__ volatile("mfc0 %0, $" #regnum ", " #sel : "=r" (val)); \
    return val; \
} \
\
static inline __ALWAYS_INLINE uint32_t mips_read_##regname##_relaxed(void) { \
    uint32_t val; \
    __asm__("mfc0 %0, $" #regnum ", " #sel : "=r" (val)); \
    return val; \
} \
\
static inline __ALWAYS_INLINE void mips_write_##regname(uint32_t val) { \
    __asm__ volatile("mtc0 %0, $" #regnum ", " #sel :: "r" (val)); \
} \
\
static inline __ALWAYS_INLINE void mips_write_##regname##_relaxed(uint32_t val) { \
    __asm__ volatile("mtc0 %0, $" #regnum ", " #sel :: "r" (val)); \
}

GEN_CP_REG_FUNCS(c0_count, 9, 0)
GEN_CP_REG_FUNCS(c0_compare, 11, 0)
GEN_CP_REG_FUNCS(c0_status, 12, 0)
GEN_CP_REG_FUNCS(c0_intctl, 12, 1)
GEN_CP_REG_FUNCS(c0_srsctl, 12, 2)
GEN_CP_REG_FUNCS(c0_srsmap1, 12, 3)
GEN_CP_REG_FUNCS(c0_view_ipl, 12, 4)
GEN_CP_REG_FUNCS(c0_srsmap2, 12, 5)
GEN_CP_REG_FUNCS(c0_cause, 13, 0)
GEN_CP_REG_FUNCS(c0_epc, 14, 0)
GEN_CP_REG_FUNCS(c0_prid, 15, 0)
GEN_CP_REG_FUNCS(c0_ebase, 15, 1)
GEN_CP_REG_FUNCS(c0_cdmmbase, 15, 2)
GEN_CP_REG_FUNCS(c0_config, 16, 0)
GEN_CP_REG_FUNCS(c0_config1, 16, 1)
GEN_CP_REG_FUNCS(c0_config2, 16, 2)
GEN_CP_REG_FUNCS(c0_config3, 16, 3)
GEN_CP_REG_FUNCS(c0_config4, 16, 4)
GEN_CP_REG_FUNCS(c0_config5, 16, 5)
GEN_CP_REG_FUNCS(c0_config6, 16, 6)
GEN_CP_REG_FUNCS(c0_config7, 16, 7)
GEN_CP_REG_FUNCS(c0_config8, 16, 8)

struct mips_iframe {
    uint32_t at;
    uint32_t v0;
    uint32_t v1;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t t7;
    uint32_t t8;
    uint32_t t9;
    uint32_t gp;
    uint32_t ra;
    uint32_t status;
    uint32_t cause;
    uint32_t epc;
};
STATIC_ASSERT(sizeof(struct mips_iframe) == 88);

void mips_init_timer(uint32_t freq);
enum handler_return mips_timer_irq(void);

void mips_enable_irq(uint num);
void mips_disable_irq(uint num);

#endif // !ASSEMBLY

#define VECTORED_OFFSET_SHIFT 32

