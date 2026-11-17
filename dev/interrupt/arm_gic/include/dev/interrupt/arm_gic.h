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

#include <lk/compiler.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

/**
 * arm_gic_init() - Legacy GIC initialization routine.
 *
 * This initializes the GIC using the %GICBASE and %GICx_OFFSET
 * macros as the virtual addresses of the GIC banks, and assumes
 * that the platform code has already mapped them into the
 * address space.
 * Users must also ensure that GIC_VERSION is defined to the
 * correct GIC version before calling this function.
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
#define ARM_GIC_MAX_MSI_FRAMES 4

struct arm_gic_init_info {
    int gic_revision; // GIC version: 2, 3, or 4
    paddr_t gicc_paddr;
    size_t gicc_size;
    paddr_t gicd_paddr;
    size_t gicd_size;
    paddr_t gicr_paddr;
    size_t gicr_size;

    // GICv2m MSI frames, beside a GICv2 or a GICv3 without an ITS
    size_t v2m_count;
    struct {
        paddr_t paddr;
        size_t size;
    } v2m[ARM_GIC_MAX_MSI_FRAMES];
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
void arm_gic_init_map(const struct arm_gic_init_info *init_info);

enum {
    /* Ignore cpu_mask and forward interrupt to all CPUs other than the current cpu */
    ARM_GIC_SGI_FLAG_TARGET_FILTER_NOT_SENDER = 0x1,
    /* Ignore cpu_mask and forward interrupt to current CPU only */
    ARM_GIC_SGI_FLAG_TARGET_FILTER_SENDER = 0x2,
    ARM_GIC_SGI_FLAG_TARGET_FILTER_MASK = 0x3,
};
status_t arm_gic_sgi(u_int irq, u_int flags, u_int cpu_mask);

/**
 * arm_gic_msi_alloc() - Allocate one MSI through a GIC MSI frame.
 * @ctrl_paddr: Physical base of the frame (a GICv2m frame) the device's MSIs go to,
 *              as named by the device tree's msi-map or msi-parent.
 * @device_id:  The device id the device's MSIs arrive with (unused by a GICv2m frame).
 * @vector:     Returns the interrupt vector to register a handler on.
 * @msg_addr:   Returns the address the device writes to raise it.
 * @msg_data:   Returns the data it writes.
 *
 * Returns ERR_NOT_FOUND if no frame lives at @ctrl_paddr, ERR_NO_RESOURCES when the frame
 * has no interrupts left.
 */
status_t arm_gic_msi_alloc(paddr_t ctrl_paddr, uint32_t device_id, unsigned int *vector,
                           uint64_t *msg_addr, uint32_t *msg_data);
status_t arm_gic_msi_free(paddr_t ctrl_paddr, uint32_t device_id, unsigned int vector);

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

__END_CDECLS

#endif
