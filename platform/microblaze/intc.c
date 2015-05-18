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
#include <reg.h>
#include <err.h>
#include <trace.h>
#include <lk/init.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <sys/types.h>
#include <target/microblaze-config.h>

#define LOCAL_TRACE 0

#define R_ISR       0
#define R_IPR       1
#define R_IER       2
#define R_IAR       3
#define R_SIE       4
#define R_CIE       5
#define R_IVR       6
#define R_MER       7
#define R_MAX       8

#define INTC_REG(reg) (*REG32(INTC_BASEADDR + (reg) * 4))

static spin_lock_t lock;

struct int_handler_struct {
    int_handler handler;
    void *arg;
};

static struct int_handler_struct int_handler_table[MAX_INT];

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
    LTRACEF("vector %u, handler %p, arg %p\n", vector, handler, arg);

    if (vector >= MAX_INT)
        return;

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    int_handler_table[vector].handler = handler;
    int_handler_table[vector].arg = arg;

    spin_unlock_irqrestore(&lock, state);
}

status_t mask_interrupt(unsigned int vector)
{
    LTRACEF("vector %u\n", vector);

    INTC_REG(R_CIE) = 1 << vector;

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
    LTRACEF("vector %u\n", vector);

    INTC_REG(R_SIE) = 1 << vector;

    return NO_ERROR;
}

enum handler_return platform_irq_handler(void)
{
    enum handler_return ret = INT_NO_RESCHEDULE;

    uint irq = INTC_REG(R_IVR);
    LTRACEF("irq %u, IPR 0x%x, ISR 0x%x\n", irq, INTC_REG(R_IPR), INTC_REG(R_ISR));

    if (irq < MAX_INT && int_handler_table[irq].handler)
        ret = int_handler_table[irq].handler(int_handler_table[irq].arg);

    INTC_REG(R_IAR) = 1 << irq;

    return ret;
}

static void intc_init(uint level)
{
    LTRACE;

    INTC_REG(R_CIE) = 0xffffffff;
    INTC_REG(R_MER) = 0x3;
}

LK_INIT_HOOK(intc, intc_init, LK_INIT_LEVEL_PLATFORM_EARLY);

