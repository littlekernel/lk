/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "platform_p.h"

#include <assert.h>
#include <lk/bits.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/virt.h>

#define LOCAL_TRACE 0

// implementation of PIC at
// https://github.com/qemu/qemu/blob/master/hw/intc/goldfish_pic.c

enum {
    REG_STATUS          = 0x00,
    REG_IRQ_PENDING     = 0x04,
    REG_IRQ_DISABLE_ALL = 0x08,
    REG_DISABLE         = 0x0c,
    REG_ENABLE          = 0x10,
};

volatile uint32_t * const goldfish_pic_base = (void *)VIRT_GF_PIC_MMIO_BASE;

static struct int_handlers {
    int_handler handler;
    void *arg;
} handlers[NUM_IRQS];

static void write_reg(unsigned int pic, unsigned int reg, uint32_t val) {
    goldfish_pic_base[(0x1000 * pic + reg) / 4] = val;
}

static uint32_t read_reg(unsigned int pic, unsigned int reg) {
    return goldfish_pic_base[(0x1000 * pic + reg) / 4];
}

static void dump_pic(unsigned int i) {
    dprintf(INFO, "PIC %d: status %u pending %#x\n", i, read_reg(i, REG_STATUS), read_reg(i, REG_IRQ_PENDING));
}

static void dump_all_pics(void) {
    for (int i = 0; i < VIRT_GF_PIC_NB; i++) {
        dump_pic(i);
    }
}

static unsigned int irq_to_pic_num(unsigned int vector) {
    return vector / 32;
}

static unsigned int irq_to_pic_vec(unsigned int vector) {
    return vector % 32;
}

void pic_early_init(void) {
}

void pic_init(void) {
    dump_all_pics();
}

status_t mask_interrupt(unsigned int vector) {
    LTRACEF("vector %u\n", vector);
    write_reg(irq_to_pic_num(vector), REG_DISABLE, 1U << irq_to_pic_vec(vector));
    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector) {
    LTRACEF("vector %u\n", vector);
    write_reg(irq_to_pic_num(vector), REG_ENABLE, 1U << irq_to_pic_vec(vector));
    return NO_ERROR;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    LTRACEF("vector %u handler %p arg %p\n", vector, handler, arg);

    DEBUG_ASSERT(vector < NUM_IRQS);

    handlers[vector].handler = handler;
    handlers[vector].arg = arg;
}

enum handler_return m68k_platform_irq(uint8_t m68k_irq) {
    LTRACEF("m68k irq vector %d\n", m68k_irq);

    // translate m68k irqs to pic numbers
    // incoming IRQs are from 0x19-0x1f (autovectored 1 - 7 on the cpu)
    int pic_num;
    if (likely(m68k_irq >= 0x19 && m68k_irq <= 0x1f)) {
        pic_num = m68k_irq - 0x19;
    } else {
        panic("unhandled irq %d from cpu\n", m68k_irq);
    }

    // see what is pending
    uint32_t pending = read_reg(pic_num, REG_IRQ_PENDING);
    if (pending == 0) {
        // spurious
        return INT_NO_RESCHEDULE;
    }

    // find the lowest numbered bit set
    uint vector = ctz(pending) + pic_num * 32;
    LTRACEF("pic %d pending %#x vector %u\n", pic_num, pending, vector);

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(vector);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (handlers[vector].handler) {
        ret = handlers[vector].handler(handlers[vector].arg);
    }

    // no need to ack the interrupt controller since all irqs are implicitly level
    KEVLOG_IRQ_EXIT(vector);

    return ret;
}

