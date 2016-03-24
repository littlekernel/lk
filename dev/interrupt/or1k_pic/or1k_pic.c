/*
 * Copyright (c) 2015 Stefan Kristiansson
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
#include <err.h>
#include <debug.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/pic.h>
#include <arch/or1k.h>

static spin_lock_t gicd_lock;
#if WITH_LIB_SM
#define GICD_LOCK_FLAGS SPIN_LOCK_FLAG_IRQ_FIQ
#else
#define GICD_LOCK_FLAGS SPIN_LOCK_FLAG_INTERRUPTS
#endif

struct int_handler_struct {
    int_handler handler;
    void *arg;
};

static struct int_handler_struct int_handler_table[MAX_INT];

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
    spin_lock_saved_state_t state;

    if (vector >= MAX_INT)
        panic("%s: vector out of range %d\n", __FUNCTION__, vector);

    spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

    int_handler_table[vector].handler = handler;
    int_handler_table[vector].arg = arg;

    spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);
}

status_t mask_interrupt(unsigned int vector)
{
    if (vector >= MAX_INT)
        return ERR_INVALID_ARGS;

    mtspr(OR1K_SPR_PIC_PICMR_ADDR, mfspr(OR1K_SPR_PIC_PICMR_ADDR) & ~(1 << vector));

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
    if (vector >= MAX_INT)
        return ERR_INVALID_ARGS;

    mtspr(OR1K_SPR_PIC_PICMR_ADDR, mfspr(OR1K_SPR_PIC_PICMR_ADDR) | (1 << vector));

    return NO_ERROR;
}

enum handler_return platform_irq(void)
{
    enum handler_return ret = INT_NO_RESCHEDULE;

    uint irq = __builtin_ffs(mfspr(OR1K_SPR_PIC_PICSR_ADDR)) - 1;

    if (irq < MAX_INT && int_handler_table[irq].handler)
        ret = int_handler_table[irq].handler(int_handler_table[irq].arg);

    return ret;
}
