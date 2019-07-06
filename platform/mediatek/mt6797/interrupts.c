/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/arm.h>
#include <lk/reg.h>
#include <lk/debug.h>
#include <kernel/thread.h>
#include <mt_gic.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_gpt.h>
#include <platform/mt_irq.h>

#define MPIDR_LEVEL_BITS 8
#define MPIDR_LEVEL_MASK    ((1 << MPIDR_LEVEL_BITS) - 1)
#define MPIDR_AFFINITY_LEVEL(mpidr, level) \
        ((mpidr >> (MPIDR_LEVEL_BITS * level)) & MPIDR_LEVEL_MASK)


extern enum handler_return lk_scheduler(void);
extern uint32_t mt_mpidr_read(void);

uint64_t mt_irq_get_affinity(void) {
    uint64_t mpidr, aff = 0;

    mpidr = (uint64_t) mt_mpidr_read();

    aff = (
              MPIDR_AFFINITY_LEVEL(mpidr, 2) << 16 |
              MPIDR_AFFINITY_LEVEL(mpidr, 1) << 8  |
              MPIDR_AFFINITY_LEVEL(mpidr, 0)
          );

    return aff;
}

uint32_t mt_interrupt_needed_for_secure(void) {
    return 0;
}

enum handler_return platform_irq(struct arm_iframe *frame) {
    enum handler_return ret = INT_NO_RESCHEDULE;
    unsigned int irq = mt_irq_get();

    if (irq == MT_GPT_IRQ_ID)
        ret = lk_scheduler();

    return ret;
}

void platform_fiq(struct arm_iframe *frame) {
}

