/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <assert.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/x86.h>
#include <kernel/spinlock.h>
#include "platform_p.h"
#include <platform/pc.h>

#define LOCAL_TRACE 0

static spin_lock_t lock;

#define INTC_TYPE_INTERNAL  0
#define INTC_TYPE_PIC       1
#define INTC_TYPE_MSI       2

struct int_vector {
    int_handler handler;
    void *arg;
    struct {
        uint allocated : 1;
        uint type : 2; // INTC_TYPE
        uint edge : 1; // edge vs level
    } flags;
};

static struct int_vector int_table[INT_VECTORS];

void platform_init_interrupts(void) {
    pic_init();
    lapic_init();

    // initialize all of the vectors
    for (int i = 0; i < INT_VECTORS; i++) {
        if (i >= INT_PIC1_BASE && i <= INT_PIC2_BASE + 8) {
            int_table[i].flags.type = INTC_TYPE_PIC;
        }
        if (i >= INT_DYNAMIC_START && i <= INT_DYNAMIC_END) {
            int_table[i].flags.allocated = false;
        } else {
            int_table[i].flags.allocated = true;
        }
    }
}

status_t mask_interrupt(unsigned int vector) {
    if (vector >= INT_VECTORS)
        return ERR_INVALID_ARGS;

    LTRACEF("vector %#x\n", vector);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    if (int_table[vector].flags.type == INTC_TYPE_PIC) {
        pic_enable(vector, false);
    }

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}


status_t unmask_interrupt(unsigned int vector) {
    if (vector >= INT_VECTORS)
        return ERR_INVALID_ARGS;

    LTRACEF("vector %#x\n", vector);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    if (int_table[vector].flags.type == INTC_TYPE_PIC) {
        pic_enable(vector, true);
    }

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

enum handler_return platform_irq(x86_iframe_t *frame);
enum handler_return platform_irq(x86_iframe_t *frame) {
    // get the current vector
    unsigned int vector = frame->vector;

    DEBUG_ASSERT(vector >= 0x20);

    struct int_vector *handler = &int_table[vector];

    // edge triggered interrupts are acked beforehand
    if (handler->flags.edge) {
        if (handler->flags.type == INTC_TYPE_MSI) {
            lapic_eoi(vector);
        } else {
            pic_eoi(vector);
        }
    }

    // call the registered interrupt handler
    enum handler_return ret = INT_NO_RESCHEDULE;
    if (handler->handler) {
        ret = handler->handler(handler->arg);
    }

    // level triggered ack
    if (!handler->flags.edge) {
        if (handler->flags.type == INTC_TYPE_MSI) {
            lapic_eoi(vector);
        } else {
            pic_eoi(vector);
        }
    }

    return ret;
}

static void register_int_handler_etc(unsigned int vector, int_handler handler, void *arg, bool edge, uint type) {
    ASSERT(vector < INT_VECTORS);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    int_table[vector].arg = arg;
    int_table[vector].handler = handler;
    int_table[vector].flags.allocated = true;
    int_table[vector].flags.edge = edge;
    int_table[vector].flags.type = type;

    spin_unlock_irqrestore(&lock, state);
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    register_int_handler_etc(vector, handler, arg, false, INTC_TYPE_PIC);
}

void register_int_handler_msi(unsigned int vector, int_handler handler, void *arg, bool edge) {
    register_int_handler_etc(vector, handler, arg, edge, INTC_TYPE_MSI);
}

void platform_mask_irqs(void) {
    pic_mask_interrupts();
}

status_t platform_pci_int_to_vector(unsigned int pci_int, unsigned int *vector) {
    LTRACEF("pci_int %u\n", pci_int);

    // pci interrupts are relative to PIC style irq #s so simply add INT_BASE to it
    uint out_vector = pci_int + INT_BASE;
    if (out_vector > INT_VECTORS) {
        return ERR_INVALID_ARGS;
    }

    *vector = out_vector;
    return NO_ERROR;
}

status_t platform_allocate_interrupts(size_t count, uint align_log2, bool msi, unsigned int *vector) {
    LTRACEF("count %zu align %u msi %d\n", count, align_log2, msi);
    if (align_log2 > 0) {
        PANIC_UNIMPLEMENTED;
    }

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    // find a free interrupt
    status_t err = ERR_NOT_FOUND;
    for (unsigned int i = 0; i < INT_VECTORS; i++) {
        if (!int_table[i].flags.allocated) {
            int_table[i].flags.allocated = true;
            *vector = i;
            LTRACEF("found irq %#x\n", i);
            err = NO_ERROR;
            break;
        }
    }

    spin_unlock_irqrestore(&lock, state);

    return err;
}

status_t platform_compute_msi_values(unsigned int vector, unsigned int cpu, bool edge,
        uint64_t *msi_address_out, uint16_t *msi_data_out) {

    // only handle edge triggered at the moment
    DEBUG_ASSERT(edge);

    *msi_data_out = (vector & 0xff) | (0<<15); // edge triggered
    *msi_address_out = 0xfee00000 | (cpu << 12);

    return NO_ERROR;
}

