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

#pragma once

#include <lk/reg.h>
#include <stdbool.h>
#include <sys/types.h>

void arm_gicv2_init(void);
void arm_gicv2_init_percpu(void);
status_t arm_gicv2_sgi(u_int irq, u_int flags, u_int cpu_mask);
enum handler_return arm_gicv2_platform_irq(struct iframe *frame);
void arm_gicv2_platform_fiq(struct iframe *frame);

// void arm_gicv3_configure_irq_locked(unsigned int cpu, unsigned int vector);
// void arm_gicv3_suspend_cpu(unsigned int cpu);
// void arm_gicv3_resume_cpu_locked(unsigned int cpu, bool gicd);
// uint64_t arm_gicv3_sgir_val(u_int irq, size_t cpu_num);
// void arm_gicv3_wait_for_write_complete(void);

#define GICCREG_READ(gic, reg) ({                                           \
    ASSERT((gic) < NUM_ARM_GICS);                                           \
    mmio_read32((volatile uint32_t *)(arm_gics[(gic)].gicc_vaddr + (reg))); \
})
#define GICCREG_WRITE(gic, reg, val) ({                                             \
    ASSERT((gic) < NUM_ARM_GICS);                                                   \
    mmio_write32((volatile uint32_t *)(arm_gics[(gic)].gicc_vaddr + (reg)), (val)); \
})

#define GICC_LIMIT    (0x1000)
#define GICC_MIN_SIZE (0x1000)
