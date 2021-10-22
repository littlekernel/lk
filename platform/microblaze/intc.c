/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <lk/err.h>
#include <lk/trace.h>
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

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    LTRACEF("vector %u, handler %p, arg %p\n", vector, handler, arg);

    if (vector >= MAX_INT)
        return;

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    int_handler_table[vector].handler = handler;
    int_handler_table[vector].arg = arg;

    spin_unlock_irqrestore(&lock, state);
}

status_t mask_interrupt(unsigned int vector) {
    LTRACEF("vector %u\n", vector);

    INTC_REG(R_CIE) = 1 << vector;

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector) {
    LTRACEF("vector %u\n", vector);

    INTC_REG(R_SIE) = 1 << vector;

    return NO_ERROR;
}

enum handler_return platform_irq_handler(void);
enum handler_return platform_irq_handler(void) {
    enum handler_return ret = INT_NO_RESCHEDULE;

    uint irq = INTC_REG(R_IVR);
    LTRACEF("irq %u, IPR 0x%x, ISR 0x%x\n", irq, INTC_REG(R_IPR), INTC_REG(R_ISR));

    if (irq < MAX_INT && int_handler_table[irq].handler)
        ret = int_handler_table[irq].handler(int_handler_table[irq].arg);

    INTC_REG(R_IAR) = 1 << irq;

    return ret;
}

static void intc_init(uint level) {
    LTRACE;

    INTC_REG(R_CIE) = 0xffffffff;
    INTC_REG(R_MER) = 0x3;
}

LK_INIT_HOOK(intc, intc_init, LK_INIT_LEVEL_PLATFORM_EARLY);

