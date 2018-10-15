/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <compiler.h>
#include <trace.h>
#include <arch/riscv.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

// keep in sync with asm.S
struct riscv_short_iframe {
    ulong  mepc;
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

void riscv_exception_handler(ulong cause, ulong epc, struct riscv_short_iframe *frame) {
    LTRACEF("cause %#lx epc %#lx\n", cause, epc);

    enum handler_return ret = INT_NO_RESCHEDULE;
    switch (cause) {
        case 0x80000007: // machine timer interrupt
            ret = riscv_timer_exception();
            break;
        case 0x8000000b: // machine external interrupt
            ret = riscv_platform_irq();
            break;
        default:
            TRACEF("unhandled cause %#lx, epc %#lx\n", cause, epc);
            panic("stopping");
    }

    if (ret == INT_RESCHEDULE) {
        thread_preempt();
    }
}
