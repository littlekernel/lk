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
#include <inttypes.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <lk/bits.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdint.h>

#define LOCAL_TRACE 0

#include <dev/interrupt/arm_gic.h>

#include "arm_gic_common.h"
#include "gic_v3.h"

#define WAKER_QSC_BIT (0x1u << 31)
#define WAKER_CA_BIT  (0x1u << 2)
#define WAKER_PS_BIT  (0x1u << 1)
#define WAKER_SL_BIT  (0x1u << 0)

/* GICv3/v4 */

#define GICV3_IRQ_GROUP_GRP0S  0
#define GICV3_IRQ_GROUP_GRP1NS 1
#define GICV3_IRQ_GROUP_GRP1S  2

#ifndef ARM_GIC_SELECTED_IRQ_GROUP
#define ARM_GIC_SELECTED_IRQ_GROUP GRP1NS
#endif

#define COMBINE2(a, b)  a##b
#define XCOMBINE2(a, b) COMBINE2(a, b)
#define GICV3_IRQ_GROUP XCOMBINE2(GICV3_IRQ_GROUP_, ARM_GIC_SELECTED_IRQ_GROUP)

/*
 * In ARMv8 for GICv3/v4, ARM suggest to use system register
 * to access GICC instead of memory map.
 */
#ifdef ARCH_ARM64

#define GICCREG_READ(gic, reg)       ARM64_READ_SYSREG(reg)
#define GICCREG_WRITE(gic, reg, val) ARM64_WRITE_SYSREG(reg, (uint64_t)val)

#else /* ARCH_ARM64 */

/* For 32bit mode, use different way to access registers */
#define GICCREG_READ(gic, reg)       COMBINE2(arm_read_, reg)()
#define GICCREG_WRITE(gic, reg, val) COMBINE2(arm_write_, reg)(val)

GEN_CP15_REG_FUNCS(icc_ctlr_el1, 0, c12, c12, 4);
GEN_CP15_REG_FUNCS(icc_pmr_el1, 0, c4, c6, 0);
GEN_CP15_REG_FUNCS(icc_bpr0_el1, 0, c12, c8, 3);
GEN_CP15_REG_FUNCS(icc_iar0_el1, 0, c12, c8, 0);
GEN_CP15_REG_FUNCS(icc_eoir0_el1, 0, c12, c8, 1);
GEN_CP15_REG_FUNCS(icc_rpr_el1, 0, c12, c11, 3);
GEN_CP15_REG_FUNCS(icc_hppir0_el1, 0, c12, c8, 2);
GEN_CP15_REG_FUNCS(icc_bpr1_el1, 0, c12, c12, 3);
GEN_CP15_REG_FUNCS(icc_iar1_el1, 0, c12, c12, 0);
GEN_CP15_REG_FUNCS(icc_eoir1_el1, 0, c12, c12, 1);
GEN_CP15_REG_FUNCS(icc_hppir1_el1, 0, c12, c12, 2);
GEN_CP15_REG_FUNCS(icc_dir_el1, 0, c12, c11, 1);
GEN_CP15_REG_FUNCS(icc_sre_el1, 0, c12, c12, 5);
GEN_CP15_REG_FUNCS(icc_igrpen0_el1, 0, c12, c12, 6);
GEN_CP15_REG_FUNCS(icc_igrpen1_el1, 0, c12, c12, 7);
GEN_CP15_REG_FUNCS(icc_ap0r0_el1, 0, c12, c8, 4);
GEN_CP15_REG_FUNCS(icc_ap0r1_el1, 0, c12, c8, 5);
GEN_CP15_REG_FUNCS(icc_ap0r2_el1, 0, c12, c8, 6);
GEN_CP15_REG_FUNCS(icc_ap0r3_el1, 0, c12, c8, 7);
GEN_CP15_REG_FUNCS(icc_ap1r0_el1, 0, c12, c9, 0);
GEN_CP15_REG_FUNCS(icc_ap1r1_el1, 0, c12, c9, 1);
GEN_CP15_REG_FUNCS(icc_ap1r2_el1, 0, c12, c9, 2);
GEN_CP15_REG_FUNCS(icc_ap1r3_el1, 0, c12, c9, 3);
GEN_CP15_REG64_FUNCS(icc_sgi1r_el1, 0, c12);
GEN_CP15_REG64_FUNCS(icc_asgi1r_el1, 1, c12);
GEN_CP15_REG64_FUNCS(icc_sgi0r_el1, 2, c12);

#endif /* ARCH_ARM64 */

#if GICV3_IRQ_GROUP == GICV3_IRQ_GROUP_GRP0S
#define GICC_PRIMARY_HPPIR icc_hppir0_el1
#define GICC_PRIMARY_IAR   icc_iar0_el1
#define GICC_PRIMARY_EOIR  icc_eoir0_el1
#define GICC_PRIMARY_SGIR  icc_sgi0r_el1
#else
#define GICC_PRIMARY_HPPIR icc_hppir1_el1
#define GICC_PRIMARY_IAR   icc_iar1_el1
#define GICC_PRIMARY_EOIR  icc_eoir1_el1
#define GICC_PRIMARY_SGIR  icc_sgi1r_el1
#endif

static void gicv3_gicr_exit_sleep(uint32_t cpu) {
    uint32_t val = gicr_read(0, cpu, GICR_WAKER);

    if (val & WAKER_QSC_BIT) {
        /* clear sleep bit */
        gicr_write(0, cpu, GICR_WAKER, val & ~WAKER_SL_BIT);
        while (gicr_read(0, cpu, GICR_WAKER) & WAKER_QSC_BIT) {
        }
    }
}

static void gicv3_gicr_mark_awake(uint32_t cpu) {
    uint32_t val = gicr_read(0, cpu, GICR_WAKER);

    if (val & WAKER_CA_BIT) {
        /* mark CPU as awake */
        gicr_write(0, cpu, GICR_WAKER, val & ~WAKER_PS_BIT);
        while (gicr_read(0, cpu, GICR_WAKER) & WAKER_CA_BIT) {
        }
    }
}

#if GIC600
/*
 * GIC-600 implements an additional GICR power control register
 */
#define GICR_PWRR (GICR_OFFSET + 0x0024)

#define PWRR_ON        (0x0u << 0)
#define PWRR_OFF       (0x1u << 0)
#define PWRR_RDGPD     (0x1u << 2)
#define PWRR_RDGPO     (0x1u << 3)
#define PWRR_RDGP_MASK (PWRR_RDGPD | PWRR_RDGPO)

static void gicv3_gicr_power_on(uint32_t cpu) {
    /* Initiate power up */
    gicr_write(0, cpu, GICR_PWRR, PWRR_ON);

    /* wait until it is complete (both bits are clear) */
    while (gicr_read(0, cpu, GICR_PWRR) & PWRR_RDGP_MASK) {
    }
}

static void gicv3_gicr_off(uint32_t cpu) {
    /* initiate power down */
    gicr_write(0, cpu, GICR_PWRR, PWRR_OFF);

    /* wait until it is complete (both bits are set) */
    while ((gicr_read(0, cpu, GICR_PWRR) & PWRR_RDGP_MASK) !=
           PWRR_RDGP_MASK) {
    }
}
#else /* GIC600 */

static void gicv3_gicr_power_on(uint32_t cpu) {}
static void gicv3_gicr_power_off(uint32_t cpu) {}

#endif /* GIC600 */

static void arm_gicv3_wait_for_gicr_write_complete(uint cpu) {
    /* wait until write complete */
    while (gicr_read(0, cpu, GICR_CTRL) & (1 << 31)) { // GICR_CTLR.RWP
    }
}
static void gicv3_gicr_init(void) {
    uint32_t cpu = arch_curr_cpu_num();

    gicv3_gicr_exit_sleep(cpu);
    gicv3_gicr_power_on(cpu);
    gicv3_gicr_mark_awake(cpu);

    // redistributer config: configure sgi/ppi as non-secure group 1.
    gicr_write(0, cpu, GICR_IGROUPR0, ~0);
    gicr_write(0, cpu, GICR_IGRPMODR0, 0);
    arm_gicv3_wait_for_gicr_write_complete(cpu);

    // redistributer config: clear and mask sgi/ppi.
    gicr_write(0, cpu, GICR_ICENABLER0, ~0);
    gicr_write(0, cpu, GICR_ICPENDR0, ~0);
    arm_gicv3_wait_for_gicr_write_complete(cpu);
}

void arm_gicv3_wait_for_write_complete(void) {
    /* wait until write complete */
    while (gicd_read(0, GICD_CTLR) & GICD_CTLR_RWP) {
    }
}

static void gicv3_gicd_ctrl_write(uint32_t val) {
    /* write CTRL register */
    gicd_write(0, GICD_CTLR, val);

    /* wait until write complete */
    arm_gicv3_wait_for_write_complete();
}

static void gicv3_gicd_setup_irq_group(uint32_t vector, uint32_t grp) {
    uint32_t val;
    uint32_t mask;

    ASSERT((vector >= 32) && (vector < MAX_INT));

    mask = (0x1u << (vector % 32));

    val = gicd_read(0, GICD_IGROUPR(vector / 32));
    if (grp & 0x1u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    gicd_write(0, GICD_IGROUPR(vector / 32), val);

    val = gicd_read(0, GICD_IGRPMODR(vector / 32));
    if (grp & 0x2u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    gicd_write(0, GICD_IGRPMODR(vector / 32), val);
}

static void gicv3_gicd_setup_default_group(uint32_t grp) {
    uint32_t i;

    /* Assign all interrupts to selected group */
    for (i = 32; i < MAX_INT; i += 32) {
        gicd_write(0, GICD_IGROUPR(i / 32), (grp & 0x1u) ? ~0U : 0);
        gicd_write(0, GICD_IGRPMODR(i / 32), (grp & 0x2u) ? ~0U : 0);
    }
}

static bool gicv3_gicd_security_disabled(void) {
    return gicd_read(0, GICD_CTLR) & GICD_CTLR_DS;
}

static void gicv3_gicr_setup_irq_group(uint32_t vector, uint32_t grp) {
    uint32_t val;
    uint32_t mask;
    uint32_t cpu = arch_curr_cpu_num();

    ASSERT(vector < 32);

    mask = (0x1u << vector);

    val = gicr_read(0, cpu, GICR_IGROUPR0);
    if (grp & 0x1u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    gicr_write(0, cpu, GICR_IGROUPR0, val);

    val = gicr_read(0, cpu, GICR_IGRPMODR0);
    if (grp & 0x2u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    gicr_write(0, cpu, GICR_IGRPMODR0, val);
}

static void gicv3_gicr_setup_default_group(uint32_t grp) {
    uint32_t cpu = arch_curr_cpu_num();

    gicr_write(0, cpu, GICR_IGROUPR0, (grp & 0x1u) ? ~0U : 0);
    gicr_write(0, cpu, GICR_IGRPMODR0, (grp & 0x2u) ? ~0U : 0);
}

void arm_gicv3_init(void) {
    bool disabled_security = gicv3_gicd_security_disabled();

    uint32_t pidr2 = gicd_read(0, GICD_PIDR2);
    uint32_t rev = BITS_SHIFT(pidr2, 7, 4);
    if (rev != 3 && rev != 4) {
        panic("GIC not v3 or v4, pidr %#x rev %#x\n", pidr2, rev);
    }

    uint32_t typer = gicd_read(0, GICD_TYPER);
    uint32_t gic_max_int = (BITS(typer, 4, 0) + 1) * 32;

    printf("GICv3 detected: rev %u, max interrupts %u, TYPER %#x\n", rev, gic_max_int, typer);
    if (gic_max_int > MAX_INT) {
        gic_max_int = MAX_INT;
    }
    if (disabled_security) {
        printf("GICv3 security disabled\n");
    }

    // save the revision
    arm_gics[0].gic_revision = (int)rev;

    // calculate the cpu stride
    // GICv3 has 2 64k regions pper redistributor:
    //  RD_base, SGI_base
    // GICv4 has 4 64k regions per redistributor:
    //  RD_base, SGI_base, VLPI_base, Reserved
    if (rev == 3) {
        arm_gics[0].gicr_cpu_stride = 0x20000;
    } else {
        arm_gics[0].gicr_cpu_stride = 0x40000;
    }

    /* Disable all groups before making changes */
    gicv3_gicd_ctrl_write(gicd_read(0, GICD_CTLR) & ~0x7U);

    for (uint32_t i = 0; i < gic_max_int; i += 32) {
        gicd_write(0, GICD_ICENABLER(i / 32), ~0U);
        gicd_write(0, GICD_ICPENDR(i / 32), ~0U);
        gicd_write(0, GICD_IGROUPR(i / 32), ~0U);
        gicd_write(0, GICD_IGRPMODR(i / 32), ~0U);
    }
    arm_gicv3_wait_for_write_complete();

    /* Enable distributor with ARE, group 1 enable */
    if (disabled_security == false) {
        gicv3_gicd_ctrl_write(gicd_read(0, GICD_CTLR) |
                              (GICD_CTLR_ENABLE_G0 | GICD_CTLR_ENABLE_G1NS | GICD_CTLR_ARE_S));
    } else {
        // TODO: is there a reasonable other solution here?
    }

    /* Enable selected group */
    uint32_t grp_mask = (0x1u << GICV3_IRQ_GROUP);
    gicv3_gicd_ctrl_write(gicd_read(0, GICD_CTLR) | grp_mask);
    arm_gicv3_wait_for_write_complete();

    /* Direct SPI interrupts to core 0 */
    for (uint32_t i = 32; i < gic_max_int; i++) {
        gicd_write64(0, GICD_IROUTER(i), 0);
    }
}

void arm_gicv3_init_percpu(void) {
    /* Init registributor interface */
    gicv3_gicr_init();

    /* Enable CPU interface access */
    // TODO: do we need to set bit 1 and 2? (IRQ/FIQ bypass)
    GICCREG_WRITE(0, icc_sre_el1, (GICCREG_READ(0, icc_sre_el1) | 0x7));

    /* Set priority mask to maximum to allow all priorities */
    GICCREG_WRITE(0, icc_pmr_el1, 0xFF);

    /* enable selected percpu group */
    if (GICV3_IRQ_GROUP == 0) {
        GICCREG_WRITE(0, icc_igrpen0_el1, 1);
    } else {
        GICCREG_WRITE(0, icc_igrpen1_el1, 1);
    }
}

void arm_gicv3_configure_irq_locked(unsigned int cpu, unsigned int vector) {
    uint32_t grp = GICV3_IRQ_GROUP;

    ASSERT(vector < MAX_INT);

    if (vector < 32) {
        /* PPIs */
        gicv3_gicr_setup_irq_group(vector, grp);
    } else {
        /* SPIs */
        gicv3_gicd_setup_irq_group(vector, grp);
    }
}

static uint32_t enabled_spi_mask[DIV_ROUND_UP(MAX_INT, 32)];
static uint32_t enabled_ppi_mask[SMP_MAX_CPUS];

void arm_gicv3_suspend_cpu(unsigned int cpu) {
    uint32_t i;
    ASSERT(cpu < SMP_MAX_CPUS);

    if (cpu == 0) {
        /* also save gicd */
        for (i = 32; i < MAX_INT; i += 32) {
            enabled_spi_mask[i / 32] = gicd_read(0, GICD_ISENABLER(i / 32));
        }
    }
    enabled_ppi_mask[cpu] = gicr_read(0, cpu, GICR_ISENABLER0);
}

void arm_gicv3_resume_cpu_locked(unsigned int cpu, bool gicd) {
    uint32_t i;
    ASSERT(cpu < SMP_MAX_CPUS);

    gicr_write(0, cpu, GICR_ISENABLER0, enabled_ppi_mask[cpu]);

    if (gicd) {
        /* also resume gicd */
        for (i = 32; i < MAX_INT; i += 32) {
            gicd_write(0, GICD_ISENABLER(i / 32), enabled_spi_mask[i / 32]);
        }
    }
}

#define SGIR_AFF1_SHIFT           (16)
#define SGIR_AFF2_SHIFT           (32)
#define SGIR_AFF3_SHIFT           (48)
#define SGIR_IRQ_SHIFT            (24)
#define SGIR_RS_SHIFT             (44)
#define SGIR_TARGET_LIST_SHIFT    (0)
#define SGIR_ASSEMBLE(val, shift) (((uint64_t)(val)) << (shift))

static uint64_t arm_gicv3_sgir_val(u_int irq, size_t cpu_num) {
    DEBUG_ASSERT(irq < 16);
    struct {
        uint8_t aff0;
        uint8_t aff1;
        uint8_t aff2;
        uint8_t aff3;
    } affs = {0};

#if __aarch64__
    uint64_t mpidr = arm64_cpu_num_to_mpidr(cpu_num);
    affs.aff0 = mpidr & 0xff;
    affs.aff1 = (mpidr >> 8) & 0xff;
    affs.aff2 = (mpidr >> 16) & 0xff;
    affs.aff3 = (mpidr >> 32) & 0xff;
#else
    // TODO: fix for arm32
    affs.aff0 = cpu_num;
#endif

    // TODO: configure this based on ICC_CTLR_EL1.RSS
    uint8_t range_selector = affs.aff0 >> 4;
    uint16_t target_list = 1U << (affs.aff0 & 0xf);
    uint64_t sgir = SGIR_ASSEMBLE(irq, SGIR_IRQ_SHIFT) |
                    SGIR_ASSEMBLE(affs.aff3, SGIR_AFF3_SHIFT) |
                    SGIR_ASSEMBLE(affs.aff2, SGIR_AFF2_SHIFT) |
                    SGIR_ASSEMBLE(affs.aff1, SGIR_AFF1_SHIFT) |
                    SGIR_ASSEMBLE(range_selector, SGIR_RS_SHIFT) |
                    SGIR_ASSEMBLE(target_list, SGIR_TARGET_LIST_SHIFT);

    LTRACEF_LEVEL(2, "irq %u cpu %zu affs %02x:%02x:%02x:%02x rs %u tl 0x%x sgir 0x%016" PRIx64 "\n",
                  irq, cpu_num, affs.aff3, affs.aff2, affs.aff1, affs.aff0,
                  range_selector, target_list,
                  sgir);

    return sgir;
}

status_t arm_gicv3_sgi(u_int irq, u_int flags, u_int cpu_mask) {
    for (size_t cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (!((cpu_mask >> cpu) & 1)) {
            continue;
        }

        uint64_t val = arm_gicv3_sgir_val(irq, cpu);

        GICCREG_WRITE(0, GICC_PRIMARY_SGIR, val);
    }
    return NO_ERROR;
}

enum handler_return arm_gicv3_platform_irq(struct iframe *frame) {
    // get the current vector
    uint32_t iar = GICCREG_READ(0, GICC_PRIMARY_IAR);

    // TODO: deal with extended interrupt IDs
    unsigned int vector = iar & 0x3ff;

    if (vector >= 1020) {
        // spurious or other unhandlable interrupt
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

void arm_gicv3_platform_fiq(struct iframe *frame) {
    PANIC_UNIMPLEMENTED;
}
