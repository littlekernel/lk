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

#define LOCAL_TRACE 1

/* PIC information */
/*
 * Cached IRQ mask (enabled/disabled)
 */
static uint8_t irqMask[2];

#define PIC1 0x20
#define PIC2 0xA0

#define ICW1 0x11
#define ICW4 0x01

/*
 * init the PICs and remap them
 */
static void map(uint32_t pic1, uint32_t pic2) {
    /* send ICW1 */
    outp(PIC1, ICW1);
    outp(PIC2, ICW1);

    /* send ICW2 */
    outp(PIC1 + 1, pic1);   /* remap */
    outp(PIC2 + 1, pic2);   /*  pics */

    /* send ICW3 */
    outp(PIC1 + 1, 4);  /* IRQ2 -> connection to slave */
    outp(PIC2 + 1, 2);

    /* send ICW4 */
    outp(PIC1 + 1, 5);
    outp(PIC2 + 1, 1);

    /* disable all IRQs */
    outp(PIC1 + 1, 0xff);
    outp(PIC2 + 1, 0xff);

    irqMask[0] = 0xff;
    irqMask[1] = 0xff;
}

void pic_init(void) {
    // rebase the PIC out of the way of processor exceptions
    map(INT_PIC1_BASE, INT_PIC2_BASE);
}

void pic_enable(unsigned int vector, bool enable) {
    if (vector >= INT_PIC1_BASE && vector < INT_PIC1_BASE + 8) {
        vector -= INT_PIC1_BASE;

        uint8_t bit = 1 << vector;

        if (enable && (irqMask[0] & bit)) {
            irqMask[0] = inp(PIC1 + 1);
            irqMask[0] &= ~bit;
            outp(PIC1 + 1, irqMask[0]);
            irqMask[0] = inp(PIC1 + 1);
        } else if (!enable && !(irqMask[0] & bit)) {
            irqMask[0] = inp(PIC1 + 1);
            irqMask[0] |= bit;
            outp(PIC1 + 1, irqMask[0]);
            irqMask[0] = inp(PIC1 + 1);
        }
    } else if (vector >= INT_PIC2_BASE && vector < INT_PIC2_BASE + 8) {
        vector -= INT_PIC2_BASE;

        uint8_t bit = 1 << vector;

        if (enable && (irqMask[1] & bit)) {
            irqMask[1] = inp(PIC2 + 1);
            irqMask[1] &= ~bit;
            outp(PIC2 + 1, irqMask[1]);
            irqMask[1] = inp(PIC2 + 1);
        } else if (!enable && !(irqMask[1] & bit)) {
            irqMask[1] = inp(PIC2 + 1);
            irqMask[1] |= bit;
            outp(PIC2 + 1, irqMask[1]);
            irqMask[1] = inp(PIC2 + 1);
        }

        bit = 1 << (INT_PIC2 - INT_PIC1_BASE);

        if (irqMask[1] != 0xff && (irqMask[0] & bit)) {
            irqMask[0] = inp(PIC1 + 1);
            irqMask[0] &= ~bit;
            outp(PIC1 + 1, irqMask[0]);
            irqMask[0] = inp(PIC1 + 1);
        } else if (irqMask[1] == 0 && !(irqMask[0] & bit)) {
            irqMask[0] = inp(PIC1 + 1);
            irqMask[0] |= bit;
            outp(PIC1 + 1, irqMask[0]);
            irqMask[0] = inp(PIC1 + 1);
        }
    }
}

void pic_eoi(unsigned int vector) {
    if (vector >= INT_PIC1_BASE && vector <= INT_PIC1_BASE + 7) {
        outp(PIC1, 0x20);
    } else if (vector >= INT_PIC2_BASE && vector <= INT_PIC2_BASE + 7) {
        outp(PIC2, 0x20);
        outp(PIC1, 0x20);   // must issue both for the second PIC
    }
}

void pic_mask_interrupts(void) {
    irqMask[0] = inp(PIC1 + 1);
    irqMask[1] = inp(PIC2 + 1);

    outp(PIC1 + 1, 0xff);
    outp(PIC2 + 1, 0xff);

    irqMask[0] = inp(PIC1 + 1);
    irqMask[1] = inp(PIC2 + 1);
}
