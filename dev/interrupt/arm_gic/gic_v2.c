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

#if WITH_LIB_SM
#define GICC_PRIMARY_HPPIR GICC_AHPPIR
#define GICC_PRIMARY_IAR   GICC_AIAR
#define GICC_PRIMARY_EOIR  GICC_AEOIR
#else
#define GICC_PRIMARY_HPPIR GICC_HPPIR
#define GICC_PRIMARY_IAR   GICC_IAR
#define GICC_PRIMARY_EOIR  GICC_EOIR
#endif

void arm_gicv2_init_percpu(void) {
#if WITH_LIB_SM
    GICCREG_WRITE(0, GICC_CTLR, 0xb);       // enable GIC0 and select fiq mode for secure
    GICDREG_WRITE(0, GICD_IGROUPR(0), ~0U); /* GICD_IGROUPR0 is banked */
#else
    GICCREG_WRITE(0, GICC_CTLR, 1); // enable GIC0
#endif
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
    uint32_t type = GICDREG_READ(0, GICD_TYPER);
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
        GICDREG_WRITE(0, GICD_ICENABLER(i / 32), ~0U);
        GICDREG_WRITE(0, GICD_ICPENDR(i / 32), ~0U);
    }

    if (arm_gic_max_cpu() > 0) {
        /* Set external interrupts to target cpu 0 */
        for (int i = 32; i < MAX_INT; i += 4) {
            GICDREG_WRITE(0, GICD_ITARGETSR(i / 4), 0x01010101);
        }
    }

    // Initialize all the SPIs to edge triggered
    for (int i = 32; i < max_int; i++) {
        gic_configure_interrupt(i, IRQ_TRIGGER_MODE_EDGE, IRQ_POLARITY_ACTIVE_HIGH);
    }

    GICDREG_WRITE(0, GICD_CTLR, 1); // enable GIC0
#if WITH_LIB_SM
    GICDREG_WRITE(0, GICD_CTLR, 3); // enable GIC0 ns interrupts
    /*
     * Iterate through all IRQs and set them to non-secure
     * mode. This will allow the non-secure side to handle
     * all the interrupts we don't explicitly claim.
     */
    for (int i = 32; i < max_int; i += 32) {
        u_int reg = i / 32;
        GICDREG_WRITE(0, GICD_IGROUPR(reg), gicd_igroupr[reg]);
    }
#endif
}

status_t arm_gicv2_sgi(u_int irq, u_int flags, u_int cpu_mask) {

    u_int val =
        ((flags & ARM_GIC_SGI_FLAG_TARGET_FILTER_MASK) << 24) |
        ((cpu_mask & 0xff) << 16) |
        ((flags & ARM_GIC_SGI_FLAG_NS) ? (1U << 15) : 0) |
        (irq & 0xf);

    LTRACEF("GICD_SGIR: %x\n", val);

    GICDREG_WRITE(0, GICD_SGIR, val);

    return NO_ERROR;
}

static enum handler_return __platform_irq(struct iframe *frame) {
    // get the current vector
    uint32_t iar = GICCREG_READ(0, GICC_PRIMARY_IAR);
    unsigned int vector = iar & 0x3ff;

    if (vector >= 0x3fe) {
#if WITH_LIB_SM && ARM_GIC_USE_DOORBELL_NS_IRQ
        // spurious or non-secure interrupt
        return sm_handle_irq();
#else
        // spurious
        return INT_NO_RESCHEDULE;
#endif
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

enum handler_return arm_gicv2_platform_irq(struct iframe *frame) {
#if WITH_LIB_SM && !ARM_GIC_USE_DOORBELL_NS_IRQ
    uint32_t ahppir = GICCREG_READ(0, GICC_PRIMARY_HPPIR);
    uint32_t pending_irq = ahppir & 0x3ff;
    struct int_handler_struct *h;
    uint cpu = arch_curr_cpu_num();

#if ARM_MERGE_FIQ_IRQ
    {
        uint32_t hppir = GICCREG_READ(0, GICC_HPPIR);
        uint32_t pending_fiq = hppir & 0x3ff;
        if (pending_fiq < MAX_INT) {
            platform_fiq(frame);
            return INT_NO_RESCHEDULE;
        }
    }
#endif

    LTRACEF("ahppir %d\n", ahppir);
    if (pending_irq < MAX_INT && get_int_handler(pending_irq, cpu)->handler) {
        enum handler_return ret = 0;
        uint32_t irq;
        uint8_t old_priority;
        spin_lock_saved_state_t state;

        spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

        /* Temporarily raise the priority of the interrupt we want to
         * handle so another interrupt does not take its place before
         * we can acknowledge it.
         */
        old_priority = arm_gic_get_priority(pending_irq);
        arm_gic_set_priority_locked(pending_irq, 0);
        DSB;
        irq = GICCREG_READ(0, GICC_PRIMARY_IAR) & 0x3ff;
        arm_gic_set_priority_locked(pending_irq, old_priority);

        spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);

        LTRACEF("irq %d\n", irq);
        if (irq < MAX_INT && (h = get_int_handler(pending_irq, cpu))->handler) {
            ret = h->handler(h->arg);
        } else {
            TRACEF("unexpected irq %d != %d may get lost\n", irq, pending_irq);
        }
        GICCREG_WRITE(0, GICC_PRIMARY_EOIR, irq);
        return ret;
    }
    return sm_handle_irq();
#else
    return __platform_irq(frame);
#endif
}

void arm_gicv2_platform_fiq(struct iframe *frame) {
#if WITH_LIB_SM
    sm_handle_fiq();
#else
    PANIC_UNIMPLEMENTED;
#endif
}
