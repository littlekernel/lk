/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "platform_p.h"

#include <assert.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/virt.h>

#define LOCAL_TRACE 0

// Driver for PLIC implementation for qemu riscv virt machine

#define PLIC_PRIORITY(x) (PLIC_BASE + 4 * (x))
#define PLIC_PENDING(x)  (PLIC_BASE + 0x1000 + 4 * ((x) / 32))
#define PLIC_ENABLE(x)   (PLIC_BASE + 0x2000 + 4 * ((x) / 32))
#define PLIC_THRESHOLD   (PLIC_BASE + 0x200000)
#define PLIC_COMPLETE    (PLIC_BASE + 0x200004)
#define PLIC_CLAIM       PLIC_COMPLETE

static struct int_handlers {
    int_handler handler;
    void *arg;
} handlers[NUM_IRQS];

void plic_early_init(void) {
    // mask all irqs and set their priority to 1
    for (int i = 1; i < NUM_IRQS; i++) {
        *REG32(PLIC_ENABLE(i)) &= ~(1 << (i % 32));
        *REG32(PLIC_PRIORITY(i)) = 1;
    }

    // set global priority threshold to 0
    *REG32(PLIC_THRESHOLD) = 0;
}

void plic_init(void) {
}

status_t mask_interrupt(unsigned int vector) {
    *REG32(PLIC_ENABLE(vector)) &= ~(1 << (vector % 32));
    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector) {
    *REG32(PLIC_ENABLE(vector)) |= (1 << (vector % 32));
    return NO_ERROR;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    LTRACEF("vector %u handler %p arg %p\n", vector, handler, arg);

    DEBUG_ASSERT(vector < NUM_IRQS);

    handlers[vector].handler = handler;
    handlers[vector].arg = arg;
}

enum handler_return riscv_platform_irq(void) {
    // see what irq triggered it
    uint32_t vector = *REG32(PLIC_CLAIM);
    LTRACEF("vector %u\n", vector);

    if (unlikely(vector == 0)) {
        // nothing pending
        return INT_NO_RESCHEDULE;
    }

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(vector);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (handlers[vector].handler) {
        ret = handlers[vector].handler(handlers[vector].arg);
    }

    // ack the interrupt
    *REG32(PLIC_COMPLETE) = vector;

    KEVLOG_IRQ_EXIT(vector);

    return ret;
}

