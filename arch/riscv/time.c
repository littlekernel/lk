/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifdef RISCV_VARIANT_NUCLEI
#include <nuclei_sdk_soc.h>
#endif
#include <lk/reg.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <lk/trace.h>

#include <arch/riscv.h>
#include <arch/ops.h>

#include <platform.h>
#include <platform/timer.h>

#define LOCAL_TRACE 0

static platform_timer_callback timer_cb;
static void *timer_arg;

#ifdef RISCV_VARIANT_NUCLEI
#define configKERNEL_INTERRUPT_PRIORITY     0
status_t platform_set_oneshot_timer (platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("cb %p, arg %p, interval %u\n", callback, arg, interval);

    // disable timer irq
    ECLIC_DisableIRQ(SysTimer_IRQn);

    timer_cb = callback;
    timer_arg = arg;

    // enable the timer irq
    ECLIC_EnableIRQ(SysTimer_IRQn);

    // convert interval to ticks

    uint64_t ticks = ((interval * ARCH_RISCV_MTIME_RATE) / 1000u);
    SysTick_Reload(ticks);

    return NO_ERROR;
}

lk_bigtime_t current_time_hires(void) {
#if ARCH_RISCV_MTIME_RATE < 10000000
    return current_time() * 1000llu; // hack to deal with slow clocks
#else
    return SysTimer_GetLoadValue() / (ARCH_RISCV_MTIME_RATE / 1000000u);
#endif
}

lk_time_t current_time(void) {
    return SysTimer_GetLoadValue() / (ARCH_RISCV_MTIME_RATE / 1000u);
}

void platform_stop_timer(void) {
    ECLIC_DisableIRQ(SysTimer_IRQn);
}

void platform_init_timer(void)
{
    ECLIC_DisableIRQ(SysTimer_IRQn);
    ECLIC_SetLevelIRQ(SysTimer_IRQn, configKERNEL_INTERRUPT_PRIORITY);
    ECLIC_SetShvIRQ(SysTimer_IRQn, ECLIC_NON_VECTOR_INTERRUPT);

    /* Set SWI interrupt level to lowest level/priority, SysTimerSW as Vector Interrupt */
    ECLIC_SetShvIRQ(SysTimerSW_IRQn, ECLIC_VECTOR_INTERRUPT);
    ECLIC_SetLevelIRQ(SysTimerSW_IRQn, configKERNEL_INTERRUPT_PRIORITY);
    SysTimer_ClearSWIRQ();
    ECLIC_EnableIRQ(SysTimerSW_IRQn);
}

enum handler_return riscv_mtip_handler(void) {
    LTRACEF("tick\n");

    ECLIC_DisableIRQ(SysTimer_IRQn);

    enum handler_return ret = INT_NO_RESCHEDULE;
    unsigned long state;
    state = riscv_csr_read_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE) & RISCV_CSR_XSTATUS_IE;
    if (timer_cb) {
        ret = timer_cb(timer_arg, current_time());
    }
    riscv_clic_irq_exit(ret);
    riscv_csr_set(RISCV_CSR_XSTATUS, state);

    return ret;
}

#else
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
    extern void clint_set_timer(uint64_t ticks);
    clint_set_timer(ticks);
#elif RISCV_S_MODE
    sbi_set_timer(ticks);
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
#endif