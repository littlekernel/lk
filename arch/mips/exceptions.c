/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/trace.h>
#include <lk/debug.h>
#include <assert.h>
#include <stdint.h>
#include <lk/bits.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <arch/mips.h>

#define LOCAL_TRACE 0

extern enum handler_return platform_irq(struct mips_iframe *iframe, uint num);

void mips_gen_exception(struct mips_iframe *iframe);
void mips_gen_exception(struct mips_iframe *iframe) {
    uint32_t excode = BITS_SHIFT(iframe->cause, 6, 2);
    if (excode == 0x8) {
        LTRACEF("SYSCALL, EPC 0x%x\n", iframe->epc);
        iframe->epc += 4;
    } else {
        LTRACEF("status 0x%x\n", iframe->status);
        LTRACEF("cause 0x%x\n", iframe->cause);
        LTRACEF("\texcode 0x%x\n", excode);
        LTRACEF("epc 0x%x\n", iframe->epc);
        for (;;);
    }
}

void mips_irq(struct mips_iframe *iframe, uint num);
void mips_irq(struct mips_iframe *iframe, uint num) {
    // unset IE and clear EXL
    mips_write_c0_status(mips_read_c0_status() & ~(3<<0));

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(num);

    LTRACEF("IRQ %u, EPC 0x%x, old status 0x%x, status 0x%x\n",
            num, iframe->epc, iframe->status, mips_read_c0_status());

    enum handler_return ret = INT_NO_RESCHEDULE;

    // figure out which interrupt the timer is set to
    uint32_t ipti = BITS_SHIFT(mips_read_c0_intctl(), 31, 29);
    if (ipti >= 2 && ipti == num) {
        // builtin timer
        ret = mips_timer_irq();
#if PLATFORM_QEMU_MIPS
    } else if (num == 2) {
        ret = platform_irq(iframe, num);
#endif
    } else {
        panic("mips: unhandled irq\n");
    }

    KEVLOG_IRQ_EXIT(num);

    if (ret != INT_NO_RESCHEDULE)
        thread_preempt();
}

