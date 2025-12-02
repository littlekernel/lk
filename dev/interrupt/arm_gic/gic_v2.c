/*
 * Copyright (c) 2019 LK Trusty Authors. All Rights Reserved.
 * Copyright (c) 2025 Travis Geiselbrecht
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

#include <arch/ops.h>
#include <assert.h>
#include <dev/interrupt/arm_gic.h>
#include <inttypes.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <lk/bits.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <lk/debug.h>
#include <stdint.h>

#define LOCAL_TRACE 0

#include "arm_gic_common.h"
#include "gic_v2.h"

/* main cpu regs */
#define GICC_CTLR     (0x0000)
#define GICC_PMR      (0x0004)
#define GICC_BPR      (0x0008)
#define GICC_IAR      (0x000c)
#define GICC_EOIR     (0x0010)
#define GICC_RPR      (0x0014)
#define GICC_HPPIR    (0x0018)
#define GICC_ABPR     (0x001c)
#define GICC_AIAR     (0x0020)
#define GICC_AEOIR    (0x0024)
#define GICC_AHPPIR   (0x0028)
#define GICC_APR(n)   (0x00d0 + (n) * 4)
#define GICC_NSAPR(n) (0x00e0 + (n) * 4)
#define GICC_IIDR     (0x00fc)
#if 0 /* GICC_DIR is not currently used by anything */
#define GICC_DIR (0x1000)
#endif

#define GICC_PRIMARY_HPPIR GICC_HPPIR
#define GICC_PRIMARY_IAR   GICC_IAR
#define GICC_PRIMARY_EOIR  GICC_EOIR

#define GICCREG_READ(gic, reg) ({                                           \
    ASSERT((gic) < NUM_ARM_GICS);                                           \
    mmio_read32((volatile uint32_t *)(arm_gics[(gic)].gicc_vaddr + (reg))); \
})
#define GICCREG_WRITE(gic, reg, val) ({                                             \
    ASSERT((gic) < NUM_ARM_GICS);                                                   \
    mmio_write32((volatile uint32_t *)(arm_gics[(gic)].gicc_vaddr + (reg)), (val)); \
})

void arm_gicv2_init_percpu(void) {
    GICCREG_WRITE(0, GICC_CTLR, 1); // enable GIC0
    GICCREG_WRITE(0, GICC_PMR, 0xFF); // unmask interrupts at all priority levels
}

void arm_gicv2_init(void) {
    // Are we a GICv2?
    uint32_t iidr = GICCREG_READ(0, GICC_IIDR);
    if (BITS_SHIFT(iidr, 19, 16) != 0x2) {
        dprintf(CRITICAL, "GIC: not a GICv2, IIDR 0x%x\n", iidr);
        panic("aborting GICv2 init\n");
        return;
    }
    dprintf(INFO, "GIC: version %lu\n", BITS_SHIFT(iidr, 19, 16));

    // Read how many cpus and interrupts we support
    uint32_t type = gicd_read(0, GICD_TYPER);
    uint32_t cpu_count = arm_gic_max_cpu();
    uint32_t it_lines = (type & 0x1f) + 1;
    if (it_lines > 6) {
        it_lines = 6;
    }
    int max_int = (int)it_lines * 32;
    if (max_int > MAX_INT) {
        max_int = MAX_INT;
    }
    dprintf(INFO, "GICv2: GICD_TYPER 0x%x, cpu_count %u, max_int %u\n", type, cpu_count + 1, max_int);

    for (int i = 0; i < max_int; i += 32) {
        gicd_write(0, GICD_ICENABLER(i / 32), ~0U);
        gicd_write(0, GICD_ICPENDR(i / 32), ~0U);
    }

    if (arm_gic_max_cpu() > 0) {
        /* Set external interrupts to target cpu 0 */
        for (int i = 32; i < MAX_INT; i += 4) {
            gicd_write(0, GICD_ITARGETSR(i / 4), 0x01010101);
        }
    }

    // Initialize all the SPIs to edge triggered
    for (int i = 32; i < max_int; i++) {
        gic_configure_interrupt(i, IRQ_TRIGGER_MODE_EDGE, IRQ_POLARITY_ACTIVE_HIGH);
    }

    gicd_write(0, GICD_CTLR, 1); // enable GIC0
}

status_t arm_gicv2_sgi(u_int irq, u_int flags, u_int cpu_mask) {

    u_int val =
        ((flags & ARM_GIC_SGI_FLAG_TARGET_FILTER_MASK) << 24) |
        ((cpu_mask & 0xff) << 16) |
        (irq & 0xf);

    LTRACEF("GICD_SGIR: %x\n", val);

    gicd_write(0, GICD_SGIR, val);

    return NO_ERROR;
}

enum handler_return arm_gicv2_platform_irq(struct iframe *frame) {
    // get the current vector
    uint32_t iar = GICCREG_READ(0, GICC_PRIMARY_IAR);
    unsigned int vector = iar & 0x3ff;

    if (vector >= 0x3fe) {
        // spurious
        return INT_NO_RESCHEDULE;
    }

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(vector);

    uint cpu = arch_curr_cpu_num();

    LTRACEF_LEVEL(2, "iar 0x%x cpu %u currthread %p vector %d pc 0x%" PRIxPTR "\n", iar, cpu,
                  get_current_thread(), vector, (uintptr_t)IFRAME_PC(frame));

    // deliver the interrupt
    enum handler_return ret;

    ret = INT_NO_RESCHEDULE;
    struct int_handler_struct *handler = get_int_handler(vector, cpu);
    if (handler->handler) {
        ret = handler->handler(handler->arg);
    }

    GICCREG_WRITE(0, GICC_PRIMARY_EOIR, iar);

    LTRACEF_LEVEL(2, "cpu %u exit %d\n", cpu, ret);

    KEVLOG_IRQ_EXIT(vector);

    return ret;
}

void arm_gicv2_platform_fiq(struct iframe *frame) {
    PANIC_UNIMPLEMENTED;
}
