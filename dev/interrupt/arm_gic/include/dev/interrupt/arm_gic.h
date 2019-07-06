/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __DEV_INTERRUPT_ARM_GIC_H
#define __DEV_INTERRUPT_ARM_GIC_H

#include <sys/types.h>

void arm_gic_init(void);

#define GIC_BASE_SGI 0
#define GIC_BASE_PPI 16
#define GIC_BASE_SPI 32

enum interrupt_trigger_mode {
    IRQ_TRIGGER_MODE_EDGE = 0,
    IRQ_TRIGGER_MODE_LEVEL = 1,
};

enum interrupt_polarity {
    IRQ_POLARITY_ACTIVE_HIGH = 0,
    IRQ_POLARITY_ACTIVE_LOW = 1,
};

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

#endif

