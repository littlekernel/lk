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
#include <trace.h>
#include <assert.h>
#include <err.h>
#include <bits.h>
#include <arch/arm.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <kernel/mp.h>
#include <platform/interrupts.h>
#include <platform/bcm2835.h>

#define LOCAL_TRACE 0

/* global interrupt controller */
#define INTC_PEND0  (ARMCTRL_INTC_BASE + 0x0)
#define INTC_PEND1  (ARMCTRL_INTC_BASE + 0x4)
#define INTC_PEND2  (ARMCTRL_INTC_BASE + 0x8)
#define INTC_FAST   (ARMCTRL_INTC_BASE + 0xc)
#define INTC_ENABLE1   (ARMCTRL_INTC_BASE + 0x10)
#define INTC_ENABLE2   (ARMCTRL_INTC_BASE + 0x14)
#define INTC_ENABLE3   (ARMCTRL_INTC_BASE + 0x18)
#define INTC_DISABLE1   (ARMCTRL_INTC_BASE + 0x1c)
#define INTC_DISABLE2   (ARMCTRL_INTC_BASE + 0x20)
#define INTC_DISABLE3   (ARMCTRL_INTC_BASE + 0x24)

/* per-cpu local interrupt controller bits.
 * each is repeated 4 times, one per cpu.
 */
#define INTC_LOCAL_TIMER_INT_CONTROL0 (ARM_LOCAL_BASE + 0x40)
#define INTC_LOCAL_TIMER_INT_CONTROL1 (ARM_LOCAL_BASE + 0x44)
#define INTC_LOCAL_TIMER_INT_CONTROL2 (ARM_LOCAL_BASE + 0x48)
#define INTC_LOCAL_TIMER_INT_CONTROL3 (ARM_LOCAL_BASE + 0x4c)

#define INTC_LOCAL_MAILBOX_INT_CONTROL0 (ARM_LOCAL_BASE + 0x50)
#define INTC_LOCAL_MAILBOX_INT_CONTROL1 (ARM_LOCAL_BASE + 0x54)
#define INTC_LOCAL_MAILBOX_INT_CONTROL2 (ARM_LOCAL_BASE + 0x58)
#define INTC_LOCAL_MAILBOX_INT_CONTROL3 (ARM_LOCAL_BASE + 0x5c)

#define INTC_LOCAL_IRQ_PEND0 (ARM_LOCAL_BASE + 0x60)
#define INTC_LOCAL_IRQ_PEND1 (ARM_LOCAL_BASE + 0x64)
#define INTC_LOCAL_IRQ_PEND2 (ARM_LOCAL_BASE + 0x68)
#define INTC_LOCAL_IRQ_PEND3 (ARM_LOCAL_BASE + 0x6c)

#define INTC_LOCAL_FIQ_PEND0 (ARM_LOCAL_BASE + 0x70)
#define INTC_LOCAL_FIQ_PEND1 (ARM_LOCAL_BASE + 0x74)
#define INTC_LOCAL_FIQ_PEND2 (ARM_LOCAL_BASE + 0x78)
#define INTC_LOCAL_FIQ_PEND3 (ARM_LOCAL_BASE + 0x7c)

#define INTC_LOCAL_MAILBOX0_SET0 (ARM_LOCAL_BASE + 0x80)
#define INTC_LOCAL_MAILBOX0_SET1 (ARM_LOCAL_BASE + 0x90)
#define INTC_LOCAL_MAILBOX0_SET2 (ARM_LOCAL_BASE + 0xa0)
#define INTC_LOCAL_MAILBOX0_SET3 (ARM_LOCAL_BASE + 0xb0)

#define INTC_LOCAL_MAILBOX0_CLR0 (ARM_LOCAL_BASE + 0xc0)
#define INTC_LOCAL_MAILBOX0_CLR1 (ARM_LOCAL_BASE + 0xd0)
#define INTC_LOCAL_MAILBOX0_CLR2 (ARM_LOCAL_BASE + 0xe0)
#define INTC_LOCAL_MAILBOX0_CLR3 (ARM_LOCAL_BASE + 0xf0)

struct int_handler_struct {
    int_handler handler;
    void *arg;
};

static struct int_handler_struct int_handler_table[MAX_INT];

static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

status_t mask_interrupt(unsigned int vector)
{
    LTRACEF("vector %u\n", vector);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    if (vector >= INTERRUPT_ARM_LOCAL_CNTPSIRQ && vector <= INTERRUPT_ARM_LOCAL_CNTVIRQ) {
        // local timer interrupts, mask on all cpus
        for (uint cpu = 0; cpu < 4; cpu++) {
            uintptr_t reg = INTC_LOCAL_TIMER_INT_CONTROL0 + cpu * 4;

            *REG32(reg) &= (1 << (vector - INTERRUPT_ARM_LOCAL_CNTPSIRQ));
        }
    } else if (/* vector >= ARM_IRQ1_BASE && */ vector < (ARM_IRQ0_BASE + 32)) {
        uintptr_t reg;
        if (vector >= ARM_IRQ0_BASE)
            reg = INTC_DISABLE3;
        else if (vector >= ARM_IRQ2_BASE)
            reg = INTC_DISABLE2;
        else
            reg = INTC_DISABLE1;

        *REG32(reg) = 1 << (vector % 32);
    } else {
        PANIC_UNIMPLEMENTED;
    }

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
    LTRACEF("vector %u\n", vector);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    if (vector >= INTERRUPT_ARM_LOCAL_CNTPSIRQ && vector <= INTERRUPT_ARM_LOCAL_CNTVIRQ) {
        // local timer interrupts, unmask for all cpus
        for (uint cpu = 0; cpu < 4; cpu++) {
            uintptr_t reg = INTC_LOCAL_TIMER_INT_CONTROL0 + cpu * 4;

            *REG32(reg) |= (1 << (vector - INTERRUPT_ARM_LOCAL_CNTPSIRQ));
        }
    } else if (/* vector >= ARM_IRQ1_BASE && */ vector < (ARM_IRQ0_BASE + 32)) {
        uintptr_t reg;
        if (vector >= ARM_IRQ0_BASE)
            reg = INTC_ENABLE3;
        else if (vector >= ARM_IRQ2_BASE)
            reg = INTC_ENABLE2;
        else
            reg = INTC_ENABLE1;

        *REG32(reg) = 1 << (vector % 32);
    } else {
        PANIC_UNIMPLEMENTED;
    }

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
    if (vector >= MAX_INT)
        panic("register_int_handler: vector out of range %d\n", vector);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    int_handler_table[vector].handler = handler;
    int_handler_table[vector].arg = arg;

    spin_unlock_irqrestore(&lock, state);
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
    uint vector;
    uint cpu = arch_curr_cpu_num();

    THREAD_STATS_INC(interrupts);

    // see what kind of irq it is
    uint32_t pend = *REG32(INTC_LOCAL_IRQ_PEND0 + cpu * 4);

    pend &= ~(1 << (INTERRUPT_ARM_LOCAL_GPU_FAST % 32)); // mask out gpu interrupts

    if (pend != 0) {
        // it's a local interrupt
        LTRACEF("local pend 0x%x\n", pend);
        vector = ARM_IRQ_LOCAL_BASE + ctz(pend);
        goto decoded;
    }

    // XXX disable for now, since all of the interesting irqs are mirrored into the other banks
#if 0
    // look in bank 0 (ARM interrupts)
    pend = *REG32(INTC_PEND0);
    LTRACEF("pend0 0x%x\n", pend);
    pend &= ~((1<<8)|(1<<9)); // mask out bit 8 and 9
    if (pend != 0) {
        // it's a bank 0 interrupt
        vector = ARM_IRQ0_BASE + ctz(pend);
        goto decoded;
    }
#endif

    // look for VC interrupt bank 1
    pend = *REG32(INTC_PEND1);
    LTRACEF("pend1 0x%x\n", pend);
    if (pend != 0) {
        // it's a bank 1 interrupt
        vector = ARM_IRQ1_BASE + ctz(pend);
        goto decoded;
    }

    // look for VC interrupt bank 2
    pend = *REG32(INTC_PEND2);
    LTRACEF("pend2 0x%x\n", pend);
    if (pend != 0) {
        // it's a bank 2 interrupt
        vector = ARM_IRQ2_BASE + ctz(pend);
        goto decoded;
    }

    vector = 0xffffffff;

decoded:
    LTRACEF("cpu %u vector %u\n", cpu, vector);

    // dispatch the irq
    enum handler_return ret = INT_NO_RESCHEDULE;

#if WITH_SMP
    if (vector == INTERRUPT_ARM_LOCAL_MAILBOX0) {
        pend = *REG32(INTC_LOCAL_MAILBOX0_CLR0 + 0x10 * cpu);
        LTRACEF("mailbox0 clr 0x%x\n", pend);

        // ack it
        *REG32(INTC_LOCAL_MAILBOX0_CLR0 + 0x10 * cpu) = pend;

        if (pend & (1 << MP_IPI_GENERIC)) {
            PANIC_UNIMPLEMENTED;
        }
        if (pend & (1 << MP_IPI_RESCHEDULE)) {
            ret = mp_mbx_reschedule_irq();
        }
    } else
#endif // WITH_SMP
    if (vector == 0xffffffff) {
        ret = INT_NO_RESCHEDULE;
    } else if (int_handler_table[vector].handler) {
        ret = int_handler_table[vector].handler(int_handler_table[vector].arg);
    } else {
        panic("irq %u fired on cpu %u but no handler set!\n", vector, cpu);
    }

    return ret;
}

enum handler_return platform_fiq(struct arm_iframe *frame)
{
    PANIC_UNIMPLEMENTED;
}

void bcm2835_send_ipi(uint irq, uint cpu_mask)
{
    LTRACEF("irq %u, cpu_mask 0x%x\n", irq, cpu_mask);

    for (uint i = 0; i < 4; i++) {
        if (cpu_mask & (1<<i)) {
            LTRACEF("sending to cpu %u\n", i);
            *REG32(INTC_LOCAL_MAILBOX0_SET0 + 0x10 * i) = (1 << irq);
        }
    }
}

void intc_init(void)
{
    // mask everything
    *REG32(INTC_DISABLE1) = 0xffffffff;
    *REG32(INTC_DISABLE2) = 0xffffffff;
    *REG32(INTC_DISABLE3) = 0xffffffff;

#if WITH_SMP
    // unable mailbox irqs on all cores
    for (uint i = 0; i < 4; i++) {
        *REG32(INTC_LOCAL_MAILBOX_INT_CONTROL0 + 0x4 * i) = 0x1;
    }
#endif
}

