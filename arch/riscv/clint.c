/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <platform.h>
#include <platform/timer.h>
#include <arch/riscv.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

// platform must define these
#ifndef ARCH_RISCV_CLINT_BASE
#error Platform must define ARCH_RISCV_CLINT_BASE
#endif
#ifndef ARCH_RISCV_MTIME_RATE
#error Platform must define ARCH_RISCV_MTIME_RATE
#endif

#define CLINT_MSIP(x) (ARCH_RISCV_CLINT_BASE + 4 * (x))
#define CLINT_MTIMECMP(x) (ARCH_RISCV_CLINT_BASE + 0x4000 + 4 * (x))
#define CLINT_MTIME (ARCH_RISCV_CLINT_BASE + 0xbff8)

lk_bigtime_t current_time_hires(void) {
#if ARCH_RISCV_MTIME_RATE < 10000000
    return current_time() * 1000llu; // hack to deal with slow clocks
#else
    return *REG64(CLINT_MTIME) / (ARCH_RISCV_MTIME_RATE / 1000000u);
#endif
}

lk_time_t current_time(void) {
    return *REG64(CLINT_MTIME) / (ARCH_RISCV_MTIME_RATE / 1000u);
}

static platform_timer_callback timer_cb;
static void *timer_arg;

status_t platform_set_oneshot_timer (platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("cb %p, arg %p, interval %u\n", callback, arg, interval);

    // disable timer
    riscv_csr_clear(mie, RISCV_MIE_MTIE);

    timer_cb = callback;
    timer_arg = arg;

    // convert interval to ticks
    uint64_t ticks = (interval * ARCH_RISCV_MTIME_RATE) / 1000u;
    *REG64(CLINT_MTIMECMP(0)) = *REG64(CLINT_MTIME) + ticks;

    // enable the timer
    riscv_csr_set(mie, RISCV_MIE_MTIE);

    return NO_ERROR;
}

void platform_stop_timer(void) {
    riscv_csr_clear(mie, RISCV_MIE_MTIE);
}

enum handler_return riscv_timer_exception(void) {
    LTRACEF("tick\n");

    riscv_csr_clear(mie, RISCV_MIE_MTIE);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (timer_cb) {
        ret = timer_cb(timer_arg, current_time());
    }

    return ret;
}

