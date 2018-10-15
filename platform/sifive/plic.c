/*
 * Copyright (c) 2018 Travis Geiselbrecht
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
#include "platform_p.h"

#include <assert.h>
#include <err.h>
#include <debug.h>
#include <reg.h>
#include <trace.h>
#include <platform/interrupts.h>
#include <platform/sifive.h>

#define LOCAL_TRACE 0

// Driver for PLIC implementation in SiFive E and U boards

#define PLIC_PRIORITY(x) (PLIC_BASE + 4 * (x))
#define PLIC_PENDING(x)  (PLIC_BASE + 0x1000 + 4 * ((x) / 32))
#define PLIC_ENABLE(x)   (PLIC_BASE + 0x2000 + 4 * ((x) / 32))
#define PLIC_THRESHOLD   (PLIC_BASE + 0x200000)
#define PLIC_COMPLETE    (PLIC_BASE + 0x200004)
#define PLIC_CLAIM       PLIC_COMPLETE

static struct int_handlers {
    int_handler handler;
    void *arg;
} handlers[SIFIVE_NUM_IRQS];

void plic_early_init(void) {
    // mask all irqs and set their priority to 1
    for (int i = 1; i < SIFIVE_NUM_IRQS; i++) {
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

    DEBUG_ASSERT(vector < SIFIVE_NUM_IRQS);

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

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (handlers[vector].handler) {
        ret = handlers[vector].handler(handlers[vector].arg);
    }

    // ack the interrupt
    *REG32(PLIC_COMPLETE) = vector;

    return ret;
}

