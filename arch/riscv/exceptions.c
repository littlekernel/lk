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
#include <platform.h>
#include <arch/riscv/iframe.h>

#define LOCAL_TRACE 0

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

static void dump_iframe(struct riscv_short_iframe *frame, bool kernel) {
    kprintf("a0 %#16lx a1 %#16lx a2 %#16lx a3 %#16lx\n", frame->a0, frame->a1, frame->a2, frame->a3);
    kprintf("a4 %#16lx a5 %#16lx a6 %#16lx a7 %#16lx\n", frame->a4, frame->a5, frame->a6, frame->a7);
    kprintf("t0 %#16lx t1 %#16lx t2 %#16lx t3 %#16lx\n", frame->t0, frame->t1, frame->t2, frame->t3);
    kprintf("t5 %#16lx t6 %#16lx\n", frame->t5, frame->t6);
    if (!kernel) {
        kprintf("gp %#16lx tp %#16lx sp %#lx\n", frame->gp, frame->tp, frame->sp);
    }
}

__NO_RETURN __NO_INLINE
static void fatal_exception(long cause, ulong epc, struct riscv_short_iframe *frame, bool kernel) {
    if (cause < 0) {
        kprintf("unhandled interrupt cause %#lx, epc %#lx, tval %#lx\n", cause, epc,
              riscv_csr_read(RISCV_CSR_XTVAL));
    } else {
        kprintf("unhandled exception cause %#lx (%s), epc %#lx, tval %#lx\n", cause,
              cause_to_string(cause), epc, riscv_csr_read(RISCV_CSR_XTVAL));
    }

    dump_iframe(frame, kernel);
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

// weak reference, can override this somewhere else
__WEAK
void riscv_syscall_handler(struct riscv_short_iframe *frame) {
    kprintf("unhandled syscall handler\n");
    dump_iframe(frame, false);
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void riscv_exception_handler(long cause, ulong epc, struct riscv_short_iframe *frame, bool kernel) {
    KLTRACEF("hart %u cause %#lx epc %#lx status %#lx kernel %d\n",
            riscv_current_hart(), cause, epc, frame->status, kernel);

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
                fatal_exception(cause, epc, frame, kernel);
        }
    } else {
        // all synchronous traps go here
        switch (cause) {
            case RISCV_EXCEPTION_ENV_CALL_U_MODE: // ecall from user mode
                riscv_syscall_handler(frame);
                break;
            default:
                fatal_exception(cause, epc, frame, kernel);
        }
    }

    if (ret == INT_RESCHEDULE) {
        thread_preempt();
    }
}
