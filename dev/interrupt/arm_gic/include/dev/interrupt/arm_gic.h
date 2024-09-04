/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
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
#ifndef __DEV_INTERRUPT_ARM_GIC_H
#define __DEV_INTERRUPT_ARM_GIC_H

#include <sys/types.h>

/**
 * arm_gic_init() - Legacy GIC initialization routine.
 *
 * This initializes the GIC using the %GICBASE and %GICx_OFFSET
 * macros as the virtual addresses of the GIC banks, and assumes
 * that the platform code has already mapped them into the
 * address space.
 */
void arm_gic_init(void);

/**
 * struct arm_gic_init_info - Initialization information for the GIC.
 * @gicc_paddr: Physical address of GIC CPU interface registers.
 * @gicc_size: Total size of GIC CPU interface registers.
 * @gicd_paddr: Physical address of GIC Distributor registers.
 * @gicd_size: Total size of GIC Distributor registers.
 * @gicr_paddr: Physical address of GIC Redistributor registers.
 * @gicr_size: Total size of GIC Redistributor registers.
 */
struct arm_gic_init_info {
    paddr_t gicc_paddr;
    size_t gicc_size;
    paddr_t gicd_paddr;
    size_t gicd_size;
    paddr_t gicr_paddr;
    size_t gicr_size;
};

/**
 * arm_gic_init_map() - Map the GIC into the virtual address space and
 *                      initialize it.
 * @init_info: Pointer to a &struct arm_gic_init_info structure with the extra
 *             initialization information, e.g., the physical addresses and
 *             sizes of the GIC registers.
 *
 * This function maps the registers of the GICs then initializes the GIC.
 * If ASLR is enabled then the virtual addresses are randomized.
 *
 */
void arm_gic_init_map(struct arm_gic_init_info* init_info);

enum {
    /* Ignore cpu_mask and forward interrupt to all CPUs other than the current cpu */
    ARM_GIC_SGI_FLAG_TARGET_FILTER_NOT_SENDER = 0x1,
    /* Ignore cpu_mask and forward interrupt to current CPU only */
    ARM_GIC_SGI_FLAG_TARGET_FILTER_SENDER = 0x2,
    ARM_GIC_SGI_FLAG_TARGET_FILTER_MASK = 0x3,

    /* Only forward the interrupt to CPUs that has the interrupt configured as group 1 (non-secure) */
    ARM_GIC_SGI_FLAG_NS = 0x4,
};
status_t arm_gic_sgi(u_int irq, u_int flags, u_int cpu_mask);

struct arm_gic_affinities {
    uint8_t aff0;
    uint8_t aff1;
    uint8_t aff2;
    uint8_t aff3;
};

struct arm_gic_affinities arch_cpu_num_to_gic_affinities(size_t cpu_num);

#endif

