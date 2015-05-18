/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <dev/cache/pl310.h>

#include <assert.h>
#include <trace.h>
#include <err.h>
#include <reg.h>
#include <stdlib.h>
#include <arch.h>
#include <arch/arm/mmu.h>
#include <dev/cache/pl310_config.h>
#include <lk/init.h>

/* configuration of the pl310 comes from #define space */
#ifndef PL310_BASE
#error need to define PL310_BASE
#endif

#define LOCAL_TRACE 0

#define PL310_REG(reg) (*REG32(PL310_BASE + (reg)))

/* registers */
#define REG0_CACHE_ID              0x000
#define REG0_CACHE_TYPE            0x004
#define REG1_CONTROL               0x100
#define REG1_AUX_CONTROL           0x104
#define REG1_TAG_RAM_CONTROL       0x108
#define REG1_DATA_RAM_CONTROL      0x10c
#define REG2_EV_COUNTER_CTRL       0x200
#define REG2_EV_COUNTER1_CFG       0x204
#define REG2_EV_COUNTER0_CFG       0x208
#define REG2_EV_COUNTER1           0x20c
#define REG2_EV_COUNTER0           0x210
#define REG2_INT_MASK              0x214
#define REG2_INT_MASK_STATUS       0x218
#define REG2_INT_RAW_STATUS        0x21c
#define REG2_INT_CLEAR             0x220
#define REG7_CACHE_SYNC            0x730
#define REG7_INV_PA                0x770
#define REG7_INV_WAY               0x77c
#define REG7_CLEAN_PA              0x7b0
#define REG7_CLEAN_INDEX           0x7b8
#define REG7_CLEAN_WAY             0x7bc
#define REG7_CLEAN_INV_PA          0x7f0
#define REG7_CLEAN_INV_INDEX       0x7f8
#define REG7_CLEAN_INV_WAY         0x7fc
#define REG9_D_LOCKDOWN0           0x900
#define REG9_I_LOCKDOWN0           0x904
#define REG9_D_LOCKDOWN1           0x908
#define REG9_I_LOCKDOWN1           0x90c
#define REG9_D_LOCKDOWN2           0x910
#define REG9_I_LOCKDOWN2           0x914
#define REG9_D_LOCKDOWN3           0x918
#define REG9_I_LOCKDOWN3           0x91c
#define REG9_D_LOCKDOWN4           0x920
#define REG9_I_LOCKDOWN4           0x924
#define REG9_D_LOCKDOWN5           0x928
#define REG9_I_LOCKDOWN5           0x92c
#define REG9_D_LOCKDOWN6           0x930
#define REG9_I_LOCKDOWN6           0x934
#define REG9_D_LOCKDOWN7           0x938
#define REG9_I_LOCKDOWN7           0x93c
#define REG9_LOCK_LINE_EN          0x950
#define REG9_UNLOCK_WAY            0x954
#define REG12_ADDR_FILTERING_START 0xc00
#define REG12_ADDR_FILTERING_END   0xc04
#define REG15_DEBUG_CTRL           0xf40
#define REG15_PREFETCH_CTRL        0xf60
#define REG15_POWER_CTRL           0xf80

static inline bool pl310_enabled(void)
{
    return !!(PL310_REG(REG1_CONTROL) & 1);
}

static void pl310_init(uint level)
{
    /* make sure it's already disabled */
    DEBUG_ASSERT(!pl310_enabled());

    /* set tag and data ram latency */
    PL310_REG(REG1_TAG_RAM_CONTROL) = PL310_TAG_RAM_LATENCY;
    PL310_REG(REG1_DATA_RAM_CONTROL) = PL310_DATA_RAM_LATENCY;

    /* configure */
    /* early BRESP enable, instruction/data prefetch, exclusive cache, full line of zero */
    PL310_REG(REG1_AUX_CONTROL) |= (1<<30)|(1<<29)|(1<<28)|(1<<12)|(1<<0);

    /* flush all the ways */
    PL310_REG(REG7_INV_WAY) = 0xffff;
}

/* run just before arch_early_init so the L2 is ready to go when
 * the arch code starts up the caching system.
 */
LK_INIT_HOOK(pl310_init, pl310_init, LK_INIT_LEVEL_ARCH_EARLY - 1);

status_t pl310_set_enable(bool enable)
{
    LTRACEF("enable %d\n", enable);

    if (enable) {
        if ((PL310_REG(REG1_CONTROL) & 1) == 0) {
            /* if disabled */
            pl310_invalidate();
            PL310_REG(REG1_CONTROL) = 1;
        }
    } else {
        if ((PL310_REG(REG1_CONTROL) & 1) == 1) {
            /* if enabled */
            pl310_flush_invalidate();
            PL310_REG(REG1_CONTROL) = 0;
            /* this seems to not always latch on the first try */
            while (PL310_REG(REG1_CONTROL) & 1) {
                PL310_REG(REG1_CONTROL) = 0;
            }
        }
    }

    return NO_ERROR;
}

void pl310_invalidate(void)
{
    if (unlikely(!pl310_enabled()))
        return;
    PL310_REG(REG7_INV_WAY) = 0xffff;
    while (PL310_REG(REG7_INV_WAY) != 0)
        ;
}

void pl310_flush_invalidate(void)
{
    if (unlikely(!pl310_enabled()))
        return;
    PL310_REG(REG7_CLEAN_INV_WAY) = 0xffff;
    while (PL310_REG(REG7_CLEAN_INV_WAY) != 0)
        ;
}

void pl310_sync_range(void)
{
    if (unlikely(!pl310_enabled()))
        return;

    PL310_REG(REG7_CACHE_SYNC) = 1;
}

#define PL310_LOOP_BODY(reg) \
    if (unlikely(!pl310_enabled())) \
        return; \
 \
    addr_t pa = 0; \
    uint32_t last_pa_page = 1; \
    addr_t last_va = start + len; \
    start &= ~(CACHE_LINE - 1); \
    while (start < last_va) { \
        if (unlikely(pa / PAGE_SIZE != last_pa_page)) { \
            /* get the physical address */ \
            if (unlikely(arm_vtop(start, &pa))) \
                return; \
            last_pa_page = pa / PAGE_SIZE; \
        } \
        PL310_REG(reg) = pa; \
 \
        pa += CACHE_LINE; \
        start += CACHE_LINE; \
    } \
\
    PL310_REG(REG7_CACHE_SYNC) = 1;

void pl310_clean_range(addr_t start, size_t len)
{
    LTRACEF("start 0x%lx, len %zd\n", start, len);
    PL310_LOOP_BODY(REG7_CLEAN_PA);
}

void pl310_clean_invalidate_range(addr_t start, size_t len)
{
    LTRACEF("start 0x%lx, len %zd\n", start, len);
    PL310_LOOP_BODY(REG7_CLEAN_INV_PA);
}

void pl310_invalidate_range(addr_t start, size_t len)
{
    LTRACEF("start 0x%lx, len %zd\n", start, len);
    PL310_LOOP_BODY(REG7_INV_PA);
}

void pl310_pin_cache_range(addr_t start, size_t len)
{
    len = ROUNDUP(len, CACHE_LINE);

    arch_disable_ints();

    arch_clean_invalidate_cache_range(start, len);

    PL310_REG(REG9_LOCK_LINE_EN) = 1;
    DSB;

    while (len > 0) {
        asm volatile("pld [%0]" :: "r"(start) : "memory");
        start += CACHE_LINE;
        len -= CACHE_LINE;
    }

    DSB;
    PL310_REG(REG9_LOCK_LINE_EN) = 0;

    arch_enable_ints();
}

