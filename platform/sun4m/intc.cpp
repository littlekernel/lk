//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <sys/types.h>
#include <lk/trace.h>

#include "platform_p.h"

#define LOCAL_TRACE 1

// slavio interrupt controller

// each cpu offset is 0x1000, only handle UP for now
constexpr uint64_t INTC_PERCPU_PHYS = 0xff1400000ULL;
constexpr uint64_t INTC_GLOBAL_PHYS = 0xff1410000ULL;

enum {
    INTC_PERCPU_PEND_REG = 0,
    INTC_PERCPU_CLR_PEND = 4,
    INTC_PERCPU_SET_SOFT_INT = 8,
};

enum {
    INTC_GLOBAL_PEN_REG = 0,
    INTC_GLOBAL_TARGET_MASK_REG = 4,
    INTC_GLOBAL_TARGET_SET_REG = 8,
    INTC_GLOBAL_TARGET_CLR_REG = 0xc,
    INTC_GLOBAL_TARGET_REG = 0x10,
};

void intc_dump(void) {
    printf("PERCPU PEND REG: 0x%x\n", sparc_read_physical_32(INTC_PERCPU_PHYS + INTC_PERCPU_PEND_REG));
    printf("PERCPU CLR PEND: 0x%x\n", sparc_read_physical_32(INTC_PERCPU_PHYS + INTC_PERCPU_CLR_PEND));
    printf("PERCPU SET SOFT INT: 0x%x\n", sparc_read_physical_32(INTC_PERCPU_PHYS + INTC_PERCPU_SET_SOFT_INT));
    printf("GLOBAL PEN REG: 0x%x\n", sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_PEN_REG));
    printf("GLOBAL TARGET MASK REG: 0x%x\n", sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_MASK_REG));
    printf("GLOBAL TARGET SET REG: 0x%x\n", sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_SET_REG));
    printf("GLOBAL TARGET CLR REG: 0x%x\n", sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_CLR_REG));
    printf("GLOBAL TARGET REG: 0x%x\n", sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_REG));
}

void sun4m_intc_early_init(void) {
    intc_dump();
    // mask everything
    sparc_write_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_SET_REG, 0xffffffff);
}

void sun4m_intc_init(void) {

}

extern "C" enum handler_return platform_irq(uint32_t irq) {
    LTRACEF("IRQ %u\n", irq);
    //intc_dump();

    handler_return ret = INT_NO_RESCHEDULE;
    uint32_t pending = sparc_read_physical_32(INTC_PERCPU_PHYS + INTC_PERCPU_PEND_REG);
    if (pending & (1<<14)) { // per cpu timer
        if (sun4m_timer_irq() == INT_RESCHEDULE) {
            ret = INT_RESCHEDULE;
        }
    }

    return ret;
}
