/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <assert.h>
#include <lk/compiler.h>
#include <lk/trace.h>
#include <arch/riscv.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

// keep in sync with asm.S
struct riscv_short_iframe {
    ulong  epc;
    ulong  status;
    ulong  ra;
    ulong  a0;
    ulong  a1;
    ulong  a2;
    ulong  a3;
    ulong  a4;
    ulong  a5;
    ulong  a6;
    ulong  a7;
    ulong  t0;
    ulong  t1;
    ulong  t2;
    ulong  t3;
    ulong  t4;
    ulong  t5;
    ulong  t6;
};

extern enum handler_return riscv_platform_irq(void);
extern enum handler_return riscv_software_exception(void);

void riscv_exception_handler(ulong cause, ulong epc, struct riscv_short_iframe *frame) {
    LTRACEF("cause %#lx epc %#lx status %#lx\n", cause, epc, frame->status);

    // top bit of the cause register determines if it's an interrupt or not
    const ulong int_bit = (__riscv_xlen == 32) ? (1ul<<31) : (1ul<<63);

    enum handler_return ret = INT_NO_RESCHEDULE;
    switch (cause) {
#if WITH_SMP
        case int_bit | RISCV_EXCEPTION_XSWI: // machine software interrupt
            ret = riscv_software_exception();
            break;
#endif
        case int_bit | RISCV_EXCEPTION_XTIM: // machine timer interrupt
            ret = riscv_timer_exception();
            break;
        case int_bit | RISCV_EXCEPTION_XEXT: // machine external interrupt
            ret = riscv_platform_irq();
            break;
        default:
            TRACEF("unhandled cause %#lx, epc %#lx, tval %#lx\n", cause, epc, riscv_csr_read(RISCV_CSR_XTVAL));
            panic("stopping");
    }

    if (ret == INT_RESCHEDULE) {
        thread_preempt();
    }
}
