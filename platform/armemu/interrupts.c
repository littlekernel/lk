/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/reg.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <platform/interrupts.h>
#include <platform/armemu.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include "platform_p.h"

struct int_handler_struct {
    int_handler handler;
    void *arg;
};

static struct int_handler_struct int_handler_table[PIC_MAX_INT];

void platform_init_interrupts(void) {
    // mask all the interrupts
    *REG32(PIC_MASK_LATCH) = 0xffffffff;
}

status_t mask_interrupt(unsigned int vector) {
    if (vector >= PIC_MAX_INT)
        return ERR_INVALID_ARGS;

//  dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

    *REG32(PIC_MASK_LATCH) = 1 << vector;

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector) {
    if (vector >= PIC_MAX_INT)
        return ERR_INVALID_ARGS;

//  dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

    *REG32(PIC_UNMASK_LATCH) = 1 << vector;

    return NO_ERROR;
}

enum handler_return platform_irq(struct arm_iframe *frame) {
    // get the current vector
    unsigned int vector = *REG32(PIC_CURRENT_NUM);
    if (vector == 0xffffffff)
        return INT_NO_RESCHEDULE;

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(vector);

//  printf("platform_irq: spsr 0x%x, pc 0x%x, currthread %p, vector %d\n", frame->spsr, frame->pc, current_thread, vector);

    // deliver the interrupt
    enum handler_return ret;

    ret = INT_NO_RESCHEDULE;
    if (int_handler_table[vector].handler)
        ret = int_handler_table[vector].handler(int_handler_table[vector].arg);

//  dprintf("platform_irq: exit %d\n", ret);

    KEVLOG_IRQ_EXIT(vector);

    return ret;
}

void platform_fiq(struct arm_iframe *frame) {
    panic("FIQ: unimplemented\n");
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    if (vector >= PIC_MAX_INT)
        panic("register_int_handler: vector out of range %d\n", vector);

    int_handler_table[vector].handler = handler;
    int_handler_table[vector].arg = arg;
}

