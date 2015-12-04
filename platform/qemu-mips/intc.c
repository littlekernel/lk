/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
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
#include <sys/types.h>
#include <debug.h>
#include <trace.h>
#include <err.h>
#include <reg.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/mips.h>
#include <platform/qemu-mips.h>

#define LOCAL_TRACE 0

static spin_lock_t lock;

#define PIC1 0x20
#define PIC2 0xA0

#define ICW1 0x11
#define ICW4 0x01

#define PIC1_CMD                    0x20
#define PIC1_DATA                   0x21
#define PIC2_CMD                    0xA0
#define PIC2_DATA                   0xA1
#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */

#define ICW1_ICW4   0x01        /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02        /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL  0x08        /* Level triggered (edge) mode */
#define ICW1_INIT   0x10        /* Initialization */

#define ICW4_8086   0x01        /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO   0x02        /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM   0x10        /* Special fully nested (not) */

struct int_handler_struct {
    int_handler handler;
    void *arg;
};

#define INT_PIC2 2

static struct int_handler_struct int_handler_table[INT_VECTORS];

/*
 * Cached IRQ mask (enabled/disabled)
 */
static uint8_t irqMask[2];

/*
 * init the PICs and remap them
 */
static void map(uint32_t pic1, uint32_t pic2)
{
    /* send ICW1 */
    isa_write_8(PIC1, ICW1);
    isa_write_8(PIC2, ICW1);

    /* send ICW2 */
    isa_write_8(PIC1 + 1, pic1);   /* remap */
    isa_write_8(PIC2 + 1, pic2);   /*  pics */

    /* send ICW3 */
    isa_write_8(PIC1 + 1, 4);  /* IRQ2 -> connection to slave */
    isa_write_8(PIC2 + 1, 2);

    /* send ICW4 */
    isa_write_8(PIC1 + 1, 2|5);
    isa_write_8(PIC2 + 1, 2|1);

    /* disable all IRQs */
    isa_write_8(PIC1 + 1, 0xff);
    isa_write_8(PIC2 + 1, 0xff);

    irqMask[0] = 0xff;
    irqMask[1] = 0xff;
}

static void enable(unsigned int vector, bool enable)
{
    if (vector < 8) {
        uint8_t bit = 1 << vector;

        if (enable && (irqMask[0] & bit)) {
            irqMask[0] = isa_read_8(PIC1 + 1);
            irqMask[0] &= ~bit;
            isa_write_8(PIC1 + 1, irqMask[0]);
            irqMask[0] = isa_read_8(PIC1 + 1);
        } else if (!enable && !(irqMask[0] & bit)) {
            irqMask[0] = isa_read_8(PIC1 + 1);
            irqMask[0] |= bit;
            isa_write_8(PIC1 + 1, irqMask[0]);
            irqMask[0] = isa_read_8(PIC1 + 1);
        }
    } else if (vector < 16) {
        vector -= 8;

        uint8_t bit = 1 << vector;

        if (enable && (irqMask[1] & bit)) {
            irqMask[1] = isa_read_8(PIC2 + 1);
            irqMask[1] &= ~bit;
            isa_write_8(PIC2 + 1, irqMask[1]);
            irqMask[1] = isa_read_8(PIC2 + 1);
        } else if (!enable && !(irqMask[1] & bit)) {
            irqMask[1] = isa_read_8(PIC2 + 1);
            irqMask[1] |= bit;
            isa_write_8(PIC2 + 1, irqMask[1]);
            irqMask[1] = isa_read_8(PIC2 + 1);
        }

        bit = 1 << INT_PIC2;

        if (irqMask[1] != 0xff && (irqMask[0] & bit)) {
            irqMask[0] = isa_read_8(PIC1 + 1);
            irqMask[0] &= ~bit;
            isa_write_8(PIC1 + 1, irqMask[0]);
            irqMask[0] = isa_read_8(PIC1 + 1);
        } else if (irqMask[1] == 0 && !(irqMask[0] & bit)) {
            irqMask[0] = isa_read_8(PIC1 + 1);
            irqMask[0] |= bit;
            isa_write_8(PIC1 + 1, irqMask[0]);
            irqMask[0] = isa_read_8(PIC1 + 1);
        }
    }
}

static void issueEOI(unsigned int vector)
{
    if (vector < 8) {
        isa_write_8(PIC1, 0x20);
    } else if (vector < 16) {
        isa_write_8(PIC2, 0x20);
        isa_write_8(PIC1, 0x20);   // must issue both for the second PIC
    }
}

/* Helper func */
static uint16_t __pic_get_irq_reg(uint ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    isa_write_8(PIC1_CMD, ocw3);
    isa_write_8(PIC2_CMD, ocw3);
    return (isa_read_8(PIC2_CMD) << 8) | isa_read_8(PIC1_CMD);
}

/* Returns the combined value of the cascaded PICs irq request register */
static uint16_t pic_get_irr(void)
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}

/* Returns the combined value of the cascaded PICs in-service register */
static uint16_t pic_get_isr(void)
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}

void platform_init_interrupts(void)
{
    // rebase the PIC out of the way of processor exceptions
    map(0, 8);
}

status_t mask_interrupt(unsigned int vector)
{
    if (vector >= INT_VECTORS)
        return ERR_INVALID_ARGS;

    LTRACEF("vector %d\n", vector);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    enable(vector, false);

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

void platform_mask_irqs(void)
{
    irqMask[0] = isa_read_8(PIC1 + 1);
    irqMask[1] = isa_read_8(PIC2 + 1);

    isa_write_8(PIC1 + 1, 0xff);
    isa_write_8(PIC2 + 1, 0xff);

    irqMask[0] = isa_read_8(PIC1 + 1);
    irqMask[1] = isa_read_8(PIC2 + 1);
}

status_t unmask_interrupt(unsigned int vector)
{
    if (vector >= INT_VECTORS)
        return ERR_INVALID_ARGS;

    LTRACEF("vector %d\n", vector);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    enable(vector, true);

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

enum handler_return platform_irq(struct mips_iframe *iframe, uint vector)
{
    // figure out which irq is pending
    // issue OCW3 poll commands to PIC1 and (potentially) PIC2
    isa_write_8(PIC1_CMD, (1<<3) | (1<<2));
    uint8_t val = isa_read_8(PIC1_CMD);
    if ((val & 0x80) == 0) {
        // spurious?
        return INT_NO_RESCHEDULE;
    }
    val &= ~0x80;
    if (val == INT_PIC2) {
        isa_write_8(PIC2_CMD, (1<<3) | (1<<2));
        val = isa_read_8(PIC2_CMD);
        if ((val & 0x80) == 0) {
            // spurious?
            return INT_NO_RESCHEDULE;
        }
        val &= ~0x80;
    }
    vector = val;
    LTRACEF("poll vector 0x%x\n", vector);

    THREAD_STATS_INC(interrupts);

    // deliver the interrupt
    enum handler_return ret = INT_NO_RESCHEDULE;

    if (int_handler_table[vector].handler)
        ret = int_handler_table[vector].handler(int_handler_table[vector].arg);

    return ret;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
    if (vector >= INT_VECTORS)
        panic("register_int_handler: vector out of range %d\n", vector);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    int_handler_table[vector].arg = arg;
    int_handler_table[vector].handler = handler;

    spin_unlock_irqrestore(&lock, state);
}

