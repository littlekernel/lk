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

static const char *cause_to_string(long cause) {
    switch (cause) {
        case RISCV_EXCEPTION_IADDR_MISALIGN:
            return "Instruction address misaligned";
        case RISCV_EXCEPTION_IACCESS_FAULT:
            return "Instruction access fault";
        case RISCV_EXCEPTION_ILLEGAL_INS:
            return "Illegal instruction";
        case RISCV_EXCEPTION_BREAKPOINT:
            return "Breakpoint";
        case RISCV_EXCEPTION_LOAD_ADDR_MISALIGN:
            return "Load address misaligned";
        case RISCV_EXCEPTION_LOAD_ACCESS_FAULT:
            return "Load access fault";
        case RISCV_EXCEPTION_STORE_ADDR_MISALIGN:
            return "Store/AMO address misaligned";
        case RISCV_EXCEPTION_STORE_ACCESS_FAULT:
            return "Store/AMO access fault";
        case RISCV_EXCEPTION_ENV_CALL_U_MODE:
            return "Environment call from U-mode";
        case RISCV_EXCEPTION_ENV_CALL_S_MODE:
            return "Environment call from S-mode";
        case RISCV_EXCEPTION_ENV_CALL_M_MODE:
            return "Environment call from M-mode";
        case RISCV_EXCEPTION_INS_PAGE_FAULT:
            return "Instruction page fault";
        case RISCV_EXCEPTION_LOAD_PAGE_FAULT:
            return "Load page fault";
        case RISCV_EXCEPTION_STORE_PAGE_FAULT:
            return "Store/AMO page fault";
    }
    return "Unknown";
}

__NO_RETURN __NO_INLINE
static void fatal_exception(long cause, ulong epc, struct riscv_short_iframe *frame) {
    if (cause < 0) {
        panic("unhandled interrupt cause %#lx, epc %#lx, tval %#lx\n", cause, epc,
              riscv_csr_read(RISCV_CSR_XTVAL));
    } else {
        panic("unhandled exception cause %#lx (%s), epc %#lx, tval %#lx\n", cause,
              cause_to_string(cause), epc, riscv_csr_read(RISCV_CSR_XTVAL));
    }
}

void riscv_exception_handler(long cause, ulong epc, struct riscv_short_iframe *frame) {
    LTRACEF("hart %u cause %#lx epc %#lx status %#lx\n",
            riscv_current_hart(), cause, epc, frame->status);

    enum handler_return ret = INT_NO_RESCHEDULE;

    // top bit of the cause register determines if it's an interrupt or not
    if (cause < 0) {
        switch (cause & LONG_MAX) {
#if WITH_SMP
            case RISCV_INTERRUPT_XSWI: // machine software interrupt
                ret = riscv_software_exception();
                break;
#endif
            case RISCV_INTERRUPT_XTIM: // machine timer interrupt
                ret = riscv_timer_exception();
                break;
            case RISCV_INTERRUPT_XEXT: // machine external interrupt
                ret = riscv_platform_irq();
                break;
            default:
                fatal_exception(cause, epc, frame);
        }
    } else {
        // all synchronous traps go here
        switch (cause) {
            default:
                fatal_exception(cause, epc, frame);
        }
    }

    if (ret == INT_RESCHEDULE) {
        thread_preempt();
    }
}
