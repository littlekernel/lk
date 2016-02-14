/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
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

#include <arch/ops.h>
#include <assert.h>
#include <lk/init.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <trace.h>

#define LOCAL_TRACE 0

#include <lib/fixed_point.h>

#if ARCH_ARM64

/* CNTFRQ AArch64 register */
#define TIMER_REG_CNTFRQ    cntfrq_el0

/* CNTP AArch64 registers */
#define TIMER_REG_CNTP_CTL  cntp_ctl_el0
#define TIMER_REG_CNTP_CVAL cntp_cval_el0
#define TIMER_REG_CNTP_TVAL cntp_tval_el0
#define TIMER_REG_CNTPCT    cntpct_el0

/* CNTPS AArch64 registers */
#define TIMER_REG_CNTPS_CTL cntps_ctl_el1
#define TIMER_REG_CNTPS_CVAL    cntps_cval_el1
#define TIMER_REG_CNTPS_TVAL    cntps_tval_el1
#define TIMER_REG_CNTPSCT   cntpct_el0

/* CNTV AArch64 registers */
#define TIMER_REG_CNTV_CTL  cntv_ctl_el0
#define TIMER_REG_CNTV_CVAL cntv_cval_el0
#define TIMER_REG_CNTV_TVAL cntv_tval_el0
#define TIMER_REG_CNTVCT    cntvct_el0

#define READ_TIMER_REG32(reg) ARM64_READ_SYSREG(reg)
#define READ_TIMER_REG64(reg) ARM64_READ_SYSREG(reg)
#define WRITE_TIMER_REG32(reg, val) ARM64_WRITE_SYSREG(reg, val)
#define WRITE_TIMER_REG64(reg, val) ARM64_WRITE_SYSREG(reg, val)

#else

/* CNTFRQ AArch32 register */
#define TIMER_REG_CNTFRQ    "c0, 0"

/* CNTP AArch32 registers */
#define TIMER_REG_CNTP_CTL  "c2, 1"
#define TIMER_REG_CNTP_CVAL "2"
#define TIMER_REG_CNTP_TVAL "c2, 0"
#define TIMER_REG_CNTPCT    "0"

/* CNTPS AArch32 registers are banked and accessed though CNTP */
#define CNTPS CNTP

/* CNTV AArch32 registers */
#define TIMER_REG_CNTV_CTL  "c3, 1"
#define TIMER_REG_CNTV_CVAL "3"
#define TIMER_REG_CNTV_TVAL "c3, 0"
#define TIMER_REG_CNTVCT    "1"

#define READ_TIMER_REG32(reg) \
({ \
    uint32_t _val; \
    __asm__ volatile("mrc p15, 0, %0, c14, " reg : "=r" (_val)); \
    _val; \
})

#define READ_TIMER_REG64(reg) \
({ \
    uint64_t _val; \
    __asm__ volatile("mrrc p15, " reg ", %0, %H0, c14" : "=r" (_val)); \
    _val; \
})

#define WRITE_TIMER_REG32(reg, val) \
({ \
    __asm__ volatile("mcr p15, 0, %0, c14, " reg :: "r" (val)); \
    ISB; \
})

#define WRITE_TIMER_REG64(reg, val) \
({ \
    __asm__ volatile("mcrr p15, " reg ", %0, %H0, c14" :: "r" (val)); \
    ISB; \
})

#endif

#ifndef TIMER_ARM_GENERIC_SELECTED
#define TIMER_ARM_GENERIC_SELECTED CNTP
#endif

#define COMBINE3(a,b,c) a ## b ## c
#define XCOMBINE3(a,b,c) COMBINE3(a, b, c)

#define SELECTED_TIMER_REG(reg) XCOMBINE3(TIMER_REG_, TIMER_ARM_GENERIC_SELECTED, reg)
#define TIMER_REG_CTL       SELECTED_TIMER_REG(_CTL)
#define TIMER_REG_CVAL      SELECTED_TIMER_REG(_CVAL)
#define TIMER_REG_TVAL      SELECTED_TIMER_REG(_TVAL)
#define TIMER_REG_CT        SELECTED_TIMER_REG(CT)


static platform_timer_callback t_callback;
static int timer_irq;

struct fp_32_64 cntpct_per_ms;
struct fp_32_64 ms_per_cntpct;
struct fp_32_64 us_per_cntpct;

static uint64_t lk_time_to_cntpct(lk_time_t lk_time)
{
    return u64_mul_u32_fp32_64(lk_time, cntpct_per_ms);
}

static lk_time_t cntpct_to_lk_time(uint64_t cntpct)
{
    return u32_mul_u64_fp32_64(cntpct, ms_per_cntpct);
}

static lk_bigtime_t cntpct_to_lk_bigtime(uint64_t cntpct)
{
    return u64_mul_u64_fp32_64(cntpct, us_per_cntpct);
}

static uint32_t read_cntfrq(void)
{
    uint32_t cntfrq;

    cntfrq = READ_TIMER_REG32(TIMER_REG_CNTFRQ);
    LTRACEF("cntfrq: 0x%08x, %u\n", cntfrq, cntfrq);
    return cntfrq;
}

static uint32_t read_cntp_ctl(void)
{
    uint32_t cntp_ctl;

    cntp_ctl = READ_TIMER_REG32(TIMER_REG_CTL);
    return cntp_ctl;
}

static void write_cntp_ctl(uint32_t cntp_ctl)
{
    LTRACEF_LEVEL(3, "cntp_ctl: 0x%x %x\n", cntp_ctl, read_cntp_ctl());
    WRITE_TIMER_REG32(TIMER_REG_CTL, cntp_ctl);
}

static void write_cntp_cval(uint64_t cntp_cval)
{
    LTRACEF_LEVEL(3, "cntp_cval: 0x%016llx, %llu\n", cntp_cval, cntp_cval);
    WRITE_TIMER_REG64(TIMER_REG_CVAL, cntp_cval);
}

static void write_cntp_tval(int32_t cntp_tval)
{
    LTRACEF_LEVEL(3, "cntp_tval: 0x%08x, %d\n", cntp_tval, cntp_tval);
    WRITE_TIMER_REG32(TIMER_REG_TVAL, cntp_tval);
}

static uint64_t read_cntpct(void)
{
    uint64_t cntpct;

    cntpct = READ_TIMER_REG64(TIMER_REG_CT);
    LTRACEF_LEVEL(3, "cntpct: 0x%016llx, %llu\n", cntpct, cntpct);
    return cntpct;
}

static enum handler_return platform_tick(void *arg)
{
    write_cntp_ctl(0);
    if (t_callback) {
        return t_callback(arg, current_time());
    } else {
        return INT_NO_RESCHEDULE;
    }
}

status_t platform_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
    uint64_t cntpct_interval = lk_time_to_cntpct(interval);

    ASSERT(arg == NULL);

    t_callback = callback;
    if (cntpct_interval <= INT_MAX)
        write_cntp_tval(cntpct_interval);
    else
        write_cntp_cval(read_cntpct() + cntpct_interval);
    write_cntp_ctl(1);

    return 0;
}

void platform_stop_timer(void)
{
    write_cntp_ctl(0);
}

lk_bigtime_t current_time_hires(void)
{
    return cntpct_to_lk_bigtime(read_cntpct());
}

lk_time_t current_time(void)
{
    return cntpct_to_lk_time(read_cntpct());
}

static uint32_t abs_int32(int32_t a)
{
    return (a > 0) ? a : -a;
}

static uint64_t abs_int64(int64_t a)
{
    return (a > 0) ? a : -a;
}

static void test_time_conversion_check_result(uint64_t a, uint64_t b, uint64_t limit, bool is32)
{
    if (a != b) {
        uint64_t diff = is32 ? abs_int32(a - b) : abs_int64(a - b);
        if (diff <= limit)
            LTRACEF("ROUNDED by %llu (up to %llu allowed)\n", diff, limit);
        else
            TRACEF("FAIL, off by %llu\n", diff);
    }
}

static void test_lk_time_to_cntpct(uint32_t cntfrq, lk_time_t lk_time)
{
    uint64_t cntpct = lk_time_to_cntpct(lk_time);
    uint64_t expected_cntpct = ((uint64_t)cntfrq * lk_time + 500) / 1000;

    test_time_conversion_check_result(cntpct, expected_cntpct, 1, false);
    LTRACEF_LEVEL(2, "lk_time_to_cntpct(%u): got %llu, expect %llu\n", lk_time, cntpct, expected_cntpct);
}

static void test_cntpct_to_lk_time(uint32_t cntfrq, lk_time_t expected_lk_time, uint32_t wrap_count)
{
    lk_time_t lk_time;
    uint64_t cntpct;

    cntpct = (uint64_t)cntfrq * expected_lk_time / 1000;
    if ((uint64_t)cntfrq * wrap_count > UINT_MAX)
        cntpct += (((uint64_t)cntfrq << 32) / 1000) * wrap_count;
    else
        cntpct += (((uint64_t)(cntfrq * wrap_count) << 32) / 1000);
    lk_time = cntpct_to_lk_time(cntpct);

    test_time_conversion_check_result(lk_time, expected_lk_time, (1000 + cntfrq - 1) / cntfrq, true);
    LTRACEF_LEVEL(2, "cntpct_to_lk_time(%llu): got %u, expect %u\n", cntpct, lk_time, expected_lk_time);
}

static void test_cntpct_to_lk_bigtime(uint32_t cntfrq, uint64_t expected_s)
{
    lk_bigtime_t expected_lk_bigtime = expected_s * 1000 * 1000;
    uint64_t cntpct = (uint64_t)cntfrq * expected_s;
    lk_bigtime_t lk_bigtime = cntpct_to_lk_bigtime(cntpct);

    test_time_conversion_check_result(lk_bigtime, expected_lk_bigtime, (1000 * 1000 + cntfrq - 1) / cntfrq, false);
    LTRACEF_LEVEL(2, "cntpct_to_lk_bigtime(%llu): got %llu, expect %llu\n", cntpct, lk_bigtime, expected_lk_bigtime);
}

static void test_time_conversions(uint32_t cntfrq)
{
    test_lk_time_to_cntpct(cntfrq, 0);
    test_lk_time_to_cntpct(cntfrq, 1);
    test_lk_time_to_cntpct(cntfrq, INT_MAX);
    test_lk_time_to_cntpct(cntfrq, INT_MAX + 1U);
    test_lk_time_to_cntpct(cntfrq, ~0);
    test_cntpct_to_lk_time(cntfrq, 0, 0);
    test_cntpct_to_lk_time(cntfrq, INT_MAX, 0);
    test_cntpct_to_lk_time(cntfrq, INT_MAX + 1U, 0);
    test_cntpct_to_lk_time(cntfrq, ~0, 0);
    test_cntpct_to_lk_time(cntfrq, 0, 1);
    test_cntpct_to_lk_time(cntfrq, 0, 7);
    test_cntpct_to_lk_time(cntfrq, 0, 70);
    test_cntpct_to_lk_time(cntfrq, 0, 700);
    test_cntpct_to_lk_bigtime(cntfrq, 0);
    test_cntpct_to_lk_bigtime(cntfrq, 1);
    test_cntpct_to_lk_bigtime(cntfrq, 60 * 60 * 24);
    test_cntpct_to_lk_bigtime(cntfrq, 60 * 60 * 24 * 365);
    test_cntpct_to_lk_bigtime(cntfrq, 60 * 60 * 24 * (365 * 10 + 2));
    test_cntpct_to_lk_bigtime(cntfrq, 60ULL * 60 * 24 * (365 * 100 + 2));
}

static void arm_generic_timer_init_conversion_factors(uint32_t cntfrq)
{
    fp_32_64_div_32_32(&cntpct_per_ms, cntfrq, 1000);
    fp_32_64_div_32_32(&ms_per_cntpct, 1000, cntfrq);
    fp_32_64_div_32_32(&us_per_cntpct, 1000 * 1000, cntfrq);
    LTRACEF("cntpct_per_ms: %08x.%08x%08x\n", cntpct_per_ms.l0, cntpct_per_ms.l32, cntpct_per_ms.l64);
    LTRACEF("ms_per_cntpct: %08x.%08x%08x\n", ms_per_cntpct.l0, ms_per_cntpct.l32, ms_per_cntpct.l64);
    LTRACEF("us_per_cntpct: %08x.%08x%08x\n", us_per_cntpct.l0, us_per_cntpct.l32, us_per_cntpct.l64);
}

void arm_generic_timer_init(int irq, uint32_t freq_override)
{
    uint32_t cntfrq;

    if (freq_override == 0) {
        cntfrq = read_cntfrq();

        if (!cntfrq) {
            TRACEF("Failed to initialize timer, frequency is 0\n");
            return;
        }
    } else {
        cntfrq = freq_override;
    }

#if LOCAL_TRACE
    LTRACEF("Test min cntfrq\n");
    arm_generic_timer_init_conversion_factors(1);
    test_time_conversions(1);
    LTRACEF("Test max cntfrq\n");
    arm_generic_timer_init_conversion_factors(~0);
    test_time_conversions(~0);
    LTRACEF("Set actual cntfrq\n");
#endif
    arm_generic_timer_init_conversion_factors(cntfrq);
    test_time_conversions(cntfrq);

    LTRACEF("register irq %d on cpu %d\n", irq, arch_curr_cpu_num());
    register_int_handler(irq, &platform_tick, NULL);
    unmask_interrupt(irq);

    timer_irq = irq;
}

static void arm_generic_timer_init_secondary_cpu(uint level)
{
    LTRACEF("register irq %d on cpu %d\n", timer_irq, arch_curr_cpu_num());
    register_int_handler(timer_irq, &platform_tick, NULL);
    unmask_interrupt(timer_irq);
}

/* secondary cpu initialize the timer just before the kernel starts with interrupts enabled */
LK_INIT_HOOK_FLAGS(arm_generic_timer_init_secondary_cpu,
                   arm_generic_timer_init_secondary_cpu,
                   LK_INIT_LEVEL_THREADING - 1, LK_INIT_FLAG_SECONDARY_CPUS);

static void arm_generic_timer_resume_cpu(uint level)
{
    /* Always trigger a timer interrupt on each cpu for now */
    write_cntp_tval(0);
    write_cntp_ctl(1);
}

LK_INIT_HOOK_FLAGS(arm_generic_timer_resume_cpu, arm_generic_timer_resume_cpu,
                   LK_INIT_LEVEL_PLATFORM, LK_INIT_FLAG_CPU_RESUME);
