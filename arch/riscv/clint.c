/*
 * Copyright (c) 2018 Travis Geiselbrecht
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
#include <reg.h>
#include <debug.h>
#include <trace.h>
#include <err.h>
#include <platform.h>
#include <platform/timer.h>
#include <arch/riscv.h>
#include <trace.h>

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
    return *REG64(CLINT_MTIME) / (ARCH_RISCV_MTIME_RATE / 1000000u);
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
    uint64_t ticks = interval * (ARCH_RISCV_MTIME_RATE / 1000u);
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

