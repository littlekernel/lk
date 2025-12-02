/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 * Copyright (c) 2019 LK Trusty Authors. All Rights Reserved.
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
#include <kernel/vm.h>
#include <lk/bits.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform/gic.h>
#include <platform/interrupts.h>
#include <sys/types.h>

#define LOCAL_TRACE 0

#include "arm_gic_common.h"

#include "gic_v2.h"
#include "gic_v3.h"

// Common routines shared between GICv2 and GICv3/v4 implementations.
// Acts as a shim layer between the generic arm_gic interface and the
// version-specific implementations and allows both v2 and v3/v4 to
// be built into the same kernel image.

static void gic_set_enable(uint vector, bool enable);
static void arm_gic_init_hw(void);

static spin_lock_t gicd_lock;
#define GICD_LOCK_FLAGS SPIN_LOCK_FLAG_INTERRUPTS
#define GIC_MAX_PER_CPU_INT 32
#define GIC_MAX_SGI_INT     16

#if ARM_GIC_USE_DOORBELL_NS_IRQ
static bool doorbell_enabled;
#endif

struct arm_gic arm_gics[NUM_ARM_GICS];

static bool arm_gic_check_init(int irq) {
    /* check if we have a vaddr for gicd, both gicv2 and gicv3/4 use this */
    if (!arm_gics[0].gicd_vaddr) {
        TRACEF("change to interrupt %d ignored before init\n", irq);
        return false;
    }
    return true;
}

static bool arm_gic_interrupt_change_allowed(int irq) {
    return arm_gic_check_init(irq);
}

static struct int_handler_struct int_handler_table_per_cpu[GIC_MAX_PER_CPU_INT][SMP_MAX_CPUS];
static struct int_handler_struct int_handler_table_shared[MAX_INT - GIC_MAX_PER_CPU_INT];

struct int_handler_struct *get_int_handler(unsigned int vector, uint cpu) {
    if (vector < GIC_MAX_PER_CPU_INT) {
        return &int_handler_table_per_cpu[vector][cpu];
    } else {
        return &int_handler_table_shared[vector - GIC_MAX_PER_CPU_INT];
    }
}

#if ARM_GIC_USE_DOORBELL_NS_IRQ
static status_t arm_gic_set_priority_locked(u_int irq, uint8_t priority);
#endif

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    struct int_handler_struct *h;
    uint cpu = arch_curr_cpu_num();

    spin_lock_saved_state_t state;

    if (vector >= MAX_INT) {
        panic("register_int_handler: vector out of range %d\n", vector);
    }

    spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

    if (arm_gic_interrupt_change_allowed(vector)) {
        if (arm_gics[0].gic_revision > 2) {
            arm_gicv3_configure_irq_locked(cpu, vector);
        }
        h = get_int_handler(vector, cpu);
        h->handler = handler;
        h->arg = arg;
#if ARM_GIC_USE_DOORBELL_NS_IRQ
        /*
         * Use lowest priority Linux does not mask to allow masking the entire
         * group while still allowing other interrupts to be delivered.
         */
        arm_gic_set_priority_locked(vector, 0xf7);
#endif

        /*
         * For GICv3, SGIs are maskable, and on GICv2, whether they are
         * maskable is implementation defined. As a result, the caller cannot
         * rely on them being maskable, so we enable all registered SGIs as if
         * they were non-maskable.
         */
        if (vector < GIC_MAX_SGI_INT) {
            gic_set_enable(vector, true);
        }
    }

    spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);
}

void register_int_handler_msi(unsigned int vector, int_handler handler, void *arg, bool edge) {
    // only can deal with edge triggered at the moment
    DEBUG_ASSERT(edge);

    register_int_handler(vector, handler, arg);
}

static void gic_set_enable(uint vector, bool enable) {
    int reg = vector / 32;
    uint32_t mask = 1ULL << (vector % 32);

    LTRACEF("%s: vector %u, reg %d, mask 0x%x, enable %d\n", __func__, vector, reg, mask, enable);

    if (arm_gics[0].gic_revision > 2) {
        if (reg == 0) {
            uint32_t cpu = arch_curr_cpu_num();

            /* On GICv3/v4 these are on GICR */
            if (enable) {
                gicr_write(0, cpu, GICR_ISENABLER0, mask);
            } else {
                gicr_write(0, cpu, GICR_ICENABLER0, mask);
            }
            return;
        }
    }

    if (enable) {
        gicd_write(0, GICD_ISENABLER(reg), mask);
    } else {
        gicd_write(0, GICD_ICENABLER(reg), mask);
    }

    if (arm_gics[0].gic_revision > 2) {
        /* for GIC V3, make sure write is complete */
        arm_gicv3_wait_for_write_complete();
    }
}

static void arm_gic_init_percpu(uint level) {
    if (arm_gics[0].gic_revision > 2) {
        /* GICv3/v4 */
        arm_gicv3_init_percpu();
    } else {
        /* GICv2 */
        arm_gicv2_init_percpu();
    }
}

LK_INIT_HOOK_FLAGS(arm_gic_init_percpu,
                   arm_gic_init_percpu,
                   LK_INIT_LEVEL_PLATFORM_EARLY, LK_INIT_FLAG_SECONDARY_CPUS);

static void arm_gic_suspend_cpu(uint level) {
    if (arm_gics[0].gic_revision > 2) {
        arm_gicv3_suspend_cpu(arch_curr_cpu_num());
    }
}

LK_INIT_HOOK_FLAGS(arm_gic_suspend_cpu, arm_gic_suspend_cpu,
                   LK_INIT_LEVEL_PLATFORM, LK_INIT_FLAG_CPU_OFF);

static void arm_gic_resume_cpu(uint level) {
    spin_lock_saved_state_t state;
    __UNUSED bool resume_gicd = false;

    spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

    bool distributor_enabled = true;
    if (arm_gics[0].gic_revision > 2) {
        // Check if both group 0 and secure group 1 are enabled
        if (!(gicd_read(0, GICD_CTLR) & 0b101)) {
            distributor_enabled = false;
        }
    } else {
        // Check if group 0 are enabled
        if (!(gicd_read(0, GICD_CTLR) & 1)) {
            distributor_enabled = false;
        }
    }

    if (!distributor_enabled) {
        dprintf(SPEW, "%s: distibutor is off, calling arm_gic_init instead\n", __func__);
        arm_gic_init_hw();
        resume_gicd = true;
    } else {
        arm_gic_init_percpu(0);
    }

    if (arm_gics[0].gic_revision > 2) {
        uint cpu = arch_curr_cpu_num();
        uint max_irq = resume_gicd ? MAX_INT : GIC_MAX_PER_CPU_INT;

        for (uint v = 0; v < max_irq; v++) {
            struct int_handler_struct *h = get_int_handler(v, cpu);
            if (h->handler) {
                arm_gicv3_configure_irq_locked(cpu, v);
            }
        }
        arm_gicv3_resume_cpu_locked(cpu, resume_gicd);
    }
    spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);
}

LK_INIT_HOOK_FLAGS(arm_gic_resume_cpu, arm_gic_resume_cpu,
                   LK_INIT_LEVEL_PLATFORM, LK_INIT_FLAG_CPU_RESUME);

status_t gic_configure_interrupt(unsigned int vector,
                                 enum interrupt_trigger_mode tm,
                                 enum interrupt_polarity pol) {
    // Only configurable for SPI interrupts
    if ((vector >= MAX_INT) || (vector < GIC_BASE_SPI)) {
        return ERR_INVALID_ARGS;
    }

    if (pol != IRQ_POLARITY_ACTIVE_HIGH) {
        // TODO: polarity should actually be configure through a GPIO controller
        return ERR_NOT_SUPPORTED;
    }

    // type is encoded with two bits, MSB of the two determine type
    // 16 irqs encoded per ICFGR register
    uint32_t reg_ndx = vector >> 4;
    uint32_t bit_shift = ((vector & 0xf) << 1) + 1;
    uint32_t reg_val = gicd_read(0, GICD_ICFGR(reg_ndx));
    if (tm == IRQ_TRIGGER_MODE_EDGE) {
        reg_val |= (1U << bit_shift);
    } else {
        reg_val &= ~(1U << bit_shift);
    }
    gicd_write(0, GICD_ICFGR(reg_ndx), reg_val);

    return NO_ERROR;
}

static void arm_gic_init_hw(void) {
    if (arm_gics[0].gic_revision > 2) {
        /* GICv3/v4 */
        arm_gicv3_init();
    } else {
        arm_gicv2_init();
    }
    arm_gic_init_percpu(0);
}

static bool arm_gic_is_initialized(void) {
    return arm_gics[0].gicd_vaddr != 0;
}

// Initialize the GIC hardware with hard coded #defines by the platform.
// TODO: move all users of this to arm_gic_init_map.
void arm_gic_init(void) {
    ASSERT(!arm_gic_is_initialized());
#ifdef GICBASE
#ifndef GIC_VERSION
#error "GIC_VERSION must be defined if GICBASE is defined"
#endif
#ifndef GICR_OFFSET
#define GICR_OFFSET 0x200000
#endif
    arm_gics[0].gic_revision = GIC_VERSION;

    arm_gics[0].gicd_vaddr = GICBASE(0) + GICD_OFFSET;
    arm_gics[0].gicd_size = GICD_MIN_SIZE;
    TRACEF("GICD base %#lx, size %#zx\n", arm_gics[0].gicd_vaddr, arm_gics[0].gicd_size);
    if (GIC_VERSION > 2) {
        arm_gics[0].gicr_cpu_stride = 0x20000; // default to GICv3 stride
        arm_gics[0].gicr_vaddr = GICBASE(0) + GICR_OFFSET;
        arm_gics[0].gicr_size = GICR_CPU_OFFSET(0, SMP_MAX_CPUS);
        TRACEF("GICR base %#lx, size %#zx\n", arm_gics[0].gicr_vaddr, arm_gics[0].gicr_size);
    } else {
        arm_gics[0].gicc_vaddr = GICBASE(0) + GICC_OFFSET;
        arm_gics[0].gicc_size = GICC_MIN_SIZE;
        TRACEF("GICC base %#lx, size %#zx\n", arm_gics[0].gicc_vaddr, arm_gics[0].gicc_size);
    }
#else
    /* Platforms should define GICBASE if they want to call this */
    panic("%s: GICBASE not defined\n", __func__);
#endif /* GICBASE */

    arm_gic_init_hw();
}

static status_t arm_map_regs(const char *name, vaddr_t *vaddr, paddr_t paddr, size_t size) {
    if (!size) {
        return ERR_INVALID_ARGS;
    }

    void *ptr;
    status_t ret = vmm_alloc_physical(vmm_get_kernel_aspace(), "gic", size, &ptr, 0,
                                      paddr, 0, ARCH_MMU_FLAG_UNCACHED_DEVICE | ARCH_MMU_FLAG_PERM_NO_EXECUTE);
    if (ret != NO_ERROR) {
        panic("%s: failed %d\n", __func__, ret);
    }

    DEBUG_ASSERT(ptr != NULL);

    *vaddr = (vaddr_t)ptr;

    return ret;
}

void arm_gic_init_map(const struct arm_gic_init_info *init_info) {
    ASSERT(!arm_gic_is_initialized());

    arm_gics[0].gic_revision = init_info->gic_revision;

    if (init_info->gicd_size < GICD_MIN_SIZE) {
        panic("%s: gicd mapping too small %zu\n", __func__,
              init_info->gicd_size);
    }
    arm_map_regs("gicd", &arm_gics[0].gicd_vaddr, init_info->gicd_paddr,
                 init_info->gicd_size);
    arm_gics[0].gicd_size = init_info->gicd_size;
    TRACEF("GICD mapped to vaddr %#" PRIxPTR "\n", arm_gics[0].gicd_vaddr);

    if (init_info->gic_revision > 2) {
        arm_map_regs("gicr", &arm_gics[0].gicr_vaddr, init_info->gicr_paddr,
                     init_info->gicr_size);
        arm_gics[0].gicr_size = init_info->gicr_size;
        TRACEF("GICR mapped to vaddr %#" PRIxPTR "\n", arm_gics[0].gicr_vaddr);
    } else {
        arm_map_regs("gicc", &arm_gics[0].gicc_vaddr, init_info->gicc_paddr,
                     init_info->gicc_size);
        arm_gics[0].gicc_size = init_info->gicc_size;
        TRACEF("GICC mapped to vaddr %#" PRIxPTR "\n", arm_gics[0].gicc_vaddr);
    }

    arm_gic_init_hw();
}

static status_t arm_gic_get_priority(u_int irq) {
    u_int reg = irq / 4;
    u_int shift = 8 * (irq % 4);
    return (status_t)((gicd_read(0, GICD_IPRIORITYR(reg)) >> shift) & 0xff);
}

static status_t arm_gic_set_priority_locked(u_int irq, uint8_t priority) {
    u_int reg = irq / 4;
    u_int shift = 8 * (irq % 4);
    u_int mask = 0xffU << shift;
    uint32_t regval;

    if (arm_gics[0].gic_revision > 2) {
        if (irq < 32) {
            uint cpu = arch_curr_cpu_num();

            /* On GICv3 IPRIORITY registers are on redistributor */
            regval = gicr_read(0, cpu, GICR_IPRIORITYR(reg));
            LTRACEF("irq %i, cpu %d: old GICR_IPRIORITYR%d = %x\n", irq, cpu, reg,
                    regval);
            regval = (regval & ~mask) | ((uint32_t)priority << shift);
            gicr_write(0, cpu, GICR_IPRIORITYR(reg), regval);
            LTRACEF("irq %i, cpu %d, new GICD_IPRIORITYR%d = %x, req %x\n",
                    irq, cpu, reg, gicd_read(0, GICD_IPRIORITYR(reg)), regval);
            return 0;
        }
    }

    regval = gicd_read(0, GICD_IPRIORITYR(reg));
    LTRACEF("irq %i, old GICD_IPRIORITYR%d = %x\n", irq, reg, regval);
    regval = (regval & ~mask) | ((uint32_t)priority << shift);
    gicd_write(0, GICD_IPRIORITYR(reg), regval);
    LTRACEF("irq %i, new GICD_IPRIORITYR%d = %x, req %x\n",
            irq, reg, gicd_read(0, GICD_IPRIORITYR(reg)), regval);

    return 0;
}

status_t arm_gic_sgi(u_int irq, u_int flags, u_int cpu_mask) {
    if (irq >= 16) {
        return ERR_INVALID_ARGS;
    }

    if (arm_gics[0].gic_revision > 2) {
        return arm_gicv3_sgi(irq, flags, cpu_mask);
    } else {
        return arm_gicv2_sgi(irq, flags, cpu_mask);
    }

    return NO_ERROR;
}

status_t mask_interrupt(unsigned int vector) {
    if (vector >= MAX_INT) {
        return ERR_INVALID_ARGS;
    }

    LTRACEF("mask_interrupt %d\n", vector);

    if (arm_gic_interrupt_change_allowed(vector)) {
        gic_set_enable(vector, false);
    }

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector) {
    if (vector >= MAX_INT) {
        return ERR_INVALID_ARGS;
    }

    LTRACEF("unmask_interrupt %d\n", vector);

    if (arm_gic_interrupt_change_allowed(vector)) {
        gic_set_enable(vector, true);
    }

    return NO_ERROR;
}

// Top level IRQ and FIQ handlers from arm/arm64 layer
// that dispatches to the appropriate driver
enum handler_return platform_irq(struct iframe *frame);
enum handler_return platform_irq(struct iframe *frame) {
    if (arm_gics[0].gic_revision > 2) {
        return arm_gicv3_platform_irq(frame);
    } else {
        return arm_gicv2_platform_irq(frame);
    }
}

void platform_fiq(struct iframe *frame);
void platform_fiq(struct iframe *frame) {
    if (arm_gics[0].gic_revision > 2) {
        arm_gicv3_platform_fiq(frame);
        return;
    } else {
        arm_gicv2_platform_fiq(frame);
        return;
    }
}
