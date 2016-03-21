/*
 * Copyright (c) 2015 MediaTek Inc.
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
#include <arch/arm.h>
#include <reg.h>
#include <debug.h>
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

uint64_t mt_irq_get_affinity(void)
{
    uint64_t mpidr, aff = 0;

    mpidr = (uint64_t) mt_mpidr_read();

    aff = (
              MPIDR_AFFINITY_LEVEL(mpidr, 2) << 16 |
              MPIDR_AFFINITY_LEVEL(mpidr, 1) << 8  |
              MPIDR_AFFINITY_LEVEL(mpidr, 0)
          );

    return aff;
}

uint32_t mt_interrupt_needed_for_secure(void)
{
    return 0;
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
    enum handler_return ret = INT_NO_RESCHEDULE;
    unsigned int irq = mt_irq_get();

    if (irq == MT_GPT_IRQ_ID)
        ret = lk_scheduler();

    return ret;
}

void platform_fiq(struct arm_iframe *frame)
{
}

