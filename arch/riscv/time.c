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
#include <lk/trace.h>

#include <arch/riscv/feature.h>

#include <arch/riscv.h>
#include <arch/ops.h>

#include <platform.h>
#include <platform/timer.h>

#define LOCAL_TRACE 0

static platform_timer_callback timer_cb;
static void *timer_arg;

status_t platform_set_oneshot_timer (platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("cb %p, arg %p, interval %u\n", callback, arg, interval);

    // disable timer
    riscv_csr_clear(RISCV_CSR_XIE, RISCV_CSR_XIE_TIE);

    timer_cb = callback;
    timer_arg = arg;

    // enable the timer
    riscv_csr_set(RISCV_CSR_XIE, RISCV_CSR_XIE_TIE);

    // convert interval to ticks
    uint64_t ticks = riscv_get_time() + ((interval * ARCH_RISCV_MTIME_RATE) / 1000u);
#if RISCV_M_MODE
    clint_set_timer(ticks);
#elif RISCV_S_MODE
    if (riscv_feature_test(RISCV_FEAT_SSTC)) {
#if __riscv_xlen == 64
        riscv_csr_write(RISCV_CSR_STIMECMP, ticks);
#else
        riscv_csr_write(RISCV_CSR_STIMECMPH, ticks >> 32);
        riscv_csr_write(RISCV_CSR_STIMECMP, ticks);
#endif
    } else {
        sbi_set_timer(ticks);
    }
#endif

    return NO_ERROR;
}


lk_bigtime_t current_time_hires(void) {
#if ARCH_RISCV_MTIME_RATE < 10000000
    return current_time() * 1000llu; // hack to deal with slow clocks
#else
    return riscv_get_time() / (ARCH_RISCV_MTIME_RATE / 1000000u);
#endif
}

lk_time_t current_time(void) {
    return riscv_get_time() / (ARCH_RISCV_MTIME_RATE / 1000u);
}

void platform_stop_timer(void) {
    riscv_csr_clear(RISCV_CSR_XIE, RISCV_CSR_XIE_TIE);
}

enum handler_return riscv_timer_exception(void) {
    LTRACEF("tick\n");

    riscv_csr_clear(RISCV_CSR_XIE, RISCV_CSR_XIE_TIE);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (timer_cb) {
        ret = timer_cb(timer_arg, current_time());
    }

    return ret;
}
