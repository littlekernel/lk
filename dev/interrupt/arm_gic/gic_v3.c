/*
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
#include <lk/bits.h>
#include <lk/trace.h>
#include <stdint.h>
#include <inttypes.h>

#define LOCAL_TRACE 0

#include <dev/interrupt/arm_gic.h>

#include "arm_gic_common.h"
#include "gic_v3.h"

#define WAKER_QSC_BIT (0x1u << 31)
#define WAKER_CA_BIT (0x1u << 2)
#define WAKER_PS_BIT (0x1u << 1)
#define WAKER_SL_BIT (0x1u << 0)

static void gicv3_gicr_exit_sleep(uint32_t cpu) {
    uint32_t val = GICRREG_READ(0, cpu, GICR_WAKER);

    if (val & WAKER_QSC_BIT) {
        /* clear sleep bit */
        GICRREG_WRITE(0, cpu, GICR_WAKER, val & ~WAKER_SL_BIT);
        while (GICRREG_READ(0, cpu, GICR_WAKER) & WAKER_QSC_BIT) {
        }
    }
}

static void gicv3_gicr_mark_awake(uint32_t cpu) {
    uint32_t val = GICRREG_READ(0, cpu, GICR_WAKER);

    if (val & WAKER_CA_BIT) {
        /* mark CPU as awake */
        GICRREG_WRITE(0, cpu, GICR_WAKER, val & ~WAKER_PS_BIT);
        while (GICRREG_READ(0, cpu, GICR_WAKER) & WAKER_CA_BIT) {
        }
    }
}

#if GIC600
/*
 * GIC-600 implements an additional GICR power control register
 */
#define GICR_PWRR (GICR_OFFSET + 0x0024)

#define PWRR_ON (0x0u << 0)
#define PWRR_OFF (0x1u << 0)
#define PWRR_RDGPD (0x1u << 2)
#define PWRR_RDGPO (0x1u << 3)
#define PWRR_RDGP_MASK (PWRR_RDGPD | PWRR_RDGPO)

static void gicv3_gicr_power_on(uint32_t cpu) {
    /* Initiate power up */
    GICRREG_WRITE(0, cpu, GICR_PWRR, PWRR_ON);

    /* wait until it is complete (both bits are clear) */
    while (GICRREG_READ(0, cpu, GICR_PWRR) & PWRR_RDGP_MASK) {
    }
}

static void gicv3_gicr_off(uint32_t cpu) {
    /* initiate power down */
    GICRREG_WRITE(0, cpu, GICR_PWRR, PWRR_OFF);

    /* wait until it is complete (both bits are set) */
    while ((GICRREG_READ(0, cpu, GICR_PWRR) & PWRR_RDGP_MASK) !=
           PWRR_RDGP_MASK) {
    }
}
#else /* GIC600 */

static void gicv3_gicr_power_on(uint32_t cpu) {}
static void gicv3_gicr_power_off(uint32_t cpu) {}

#endif /* GIC600 */

static void arm_gicv3_wait_for_gicr_write_complete(uint cpu) {
    /* wait until write complete */
    while (GICRREG_READ(0, cpu, GICR_CTRL) & (1<<31)) { // GICR_CTLR.RWP
    }
}
static void gicv3_gicr_init(void) {
    uint32_t cpu = arch_curr_cpu_num();

    gicv3_gicr_exit_sleep(cpu);
    gicv3_gicr_power_on(cpu);
    gicv3_gicr_mark_awake(cpu);

#if !WITH_LIB_SM
    // redistributer config: configure sgi/ppi as non-secure group 1.
    GICRREG_WRITE(0, cpu, GICR_IGROUPR0, ~0);
    GICRREG_WRITE(0, cpu, GICR_IGRPMODR0, 0);
    arm_gicv3_wait_for_gicr_write_complete(cpu);
#endif

    // redistributer config: clear and mask sgi/ppi.
    GICRREG_WRITE(0, cpu, GICR_ICENABLER0, ~0);
    GICRREG_WRITE(0, cpu, GICR_ICPENDR0, ~0);
    arm_gicv3_wait_for_gicr_write_complete(cpu);
}

void arm_gicv3_wait_for_write_complete(void) {
    /* wait until write complete */
    while (GICDREG_READ(0, GICD_CTLR) & GICD_CTLR_RWP) {
    }
}

static void gicv3_gicd_ctrl_write(uint32_t val) {
    /* write CTRL register */
    GICDREG_WRITE(0, GICD_CTLR, val);

    /* wait until write complete */
    arm_gicv3_wait_for_write_complete();
}

static void gicv3_gicd_setup_irq_group(uint32_t vector, uint32_t grp) {
    uint32_t val;
    uint32_t mask;

    ASSERT((vector >= 32) && (vector < MAX_INT));

    mask = (0x1u << (vector % 32));

    val = GICDREG_READ(0, GICD_IGROUPR(vector / 32));
    if (grp & 0x1u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    GICDREG_WRITE(0, GICD_IGROUPR(vector / 32), val);

    val = GICDREG_READ(0, GICD_IGRPMODR(vector / 32));
    if (grp & 0x2u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    GICDREG_WRITE(0, GICD_IGRPMODR(vector / 32), val);
}

static void gicv3_gicd_setup_default_group(uint32_t grp) {
    uint32_t i;

    /* Assign all interrupts to selected group */
    for (i = 32; i < MAX_INT; i += 32) {
        GICDREG_WRITE(0, GICD_IGROUPR(i / 32), (grp & 0x1u) ? ~0U : 0);
        GICDREG_WRITE(0, GICD_IGRPMODR(i / 32), (grp & 0x2u) ? ~0U : 0);
    }
}

static bool gicv3_gicd_security_disabled(void) {
    return GICDREG_READ(0, GICD_CTLR) & GICD_CTLR_DS;
}

static void gicv3_gicr_setup_irq_group(uint32_t vector, uint32_t grp) {
    uint32_t val;
    uint32_t mask;
    uint32_t cpu = arch_curr_cpu_num();

    ASSERT(vector < 32);

    mask = (0x1u << vector);

    val = GICRREG_READ(0, cpu, GICR_IGROUPR0);
    if (grp & 0x1u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    GICRREG_WRITE(0, cpu, GICR_IGROUPR0, val);

    val = GICRREG_READ(0, cpu, GICR_IGRPMODR0);
    if (grp & 0x2u) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    GICRREG_WRITE(0, cpu, GICR_IGRPMODR0, val);
}

static void gicv3_gicr_setup_default_group(uint32_t grp) {
    uint32_t cpu = arch_curr_cpu_num();

    GICRREG_WRITE(0, cpu, GICR_IGROUPR0, (grp & 0x1u) ? ~0U : 0);
    GICRREG_WRITE(0, cpu, GICR_IGRPMODR0, (grp & 0x2u) ? ~0U : 0);
}

void arm_gicv3_init(void) {
    bool disabled_security = gicv3_gicd_security_disabled();

    uint32_t pidr2 = GICDREG_READ(0, GICD_PIDR2);
    uint32_t rev = BITS_SHIFT(pidr2, 7, 4);
    if (rev != 3 && rev != 4) {
        panic("GIC not v3 or v4, pidr %#x rev %#x\n", pidr2, rev);
    }

    uint32_t typer = GICDREG_READ(0, GICD_TYPER);
    uint32_t gic_max_int = (BITS(typer, 4, 0) + 1) * 32;

    printf("GICv3 detected: rev %u, max interrupts %u, TYPER %#x\n", rev, gic_max_int, typer);
    if (gic_max_int > MAX_INT) {
        gic_max_int = MAX_INT;
    }
    if (disabled_security) {
        printf("GICv3 security disabled\n");
    }

#if !WITH_LIB_SM
    /* non-TZ */
    /* Disable all groups before making changes */
    gicv3_gicd_ctrl_write(GICDREG_READ(0, GICD_CTLR) & ~0x7U);

    for (uint32_t i = 0; i < gic_max_int; i += 32) {
        GICDREG_WRITE(0, GICD_ICENABLER(i / 32), ~0U);
        GICDREG_WRITE(0, GICD_ICPENDR(i / 32), ~0U);
        GICDREG_WRITE(0, GICD_IGROUPR(i / 32), ~0U);
        GICDREG_WRITE(0, GICD_IGRPMODR(i / 32), ~0U);
    }
    arm_gicv3_wait_for_write_complete();;
#endif

    /* Enable distributor with ARE, group 1 enable */
    if (disabled_security == false) {
        gicv3_gicd_ctrl_write(GICDREG_READ(0, GICD_CTLR) |
            (GICD_CTLR_ENABLE_G0 | GICD_CTLR_ENABLE_G1NS | GICD_CTLR_ARE_S));
    } else {
        // TODO: is there a reasonable other solution here?
    }

    /* Enable selected group */
    uint32_t grp_mask = (0x1u << GICV3_IRQ_GROUP);
    gicv3_gicd_ctrl_write(GICDREG_READ(0, GICD_CTLR) | grp_mask);
    arm_gicv3_wait_for_write_complete();

#if !WITH_LIB_SM
    /* Direct SPI interrupts to core 0 */
    for (uint32_t i = 32; i < gic_max_int; i++) {
        GICDREG_WRITE64(0, GICD_IROUTER(i), 0);
    }
#endif
}

void arm_gicv3_init_percpu(void) {
#if WITH_LIB_SM
    /* TZ */
    /* Initialized by ATF */
#if ARM_GIC_USE_DOORBELL_NS_IRQ
    gicv3_gicr_setup_irq_group(ARM_GIC_DOORBELL_IRQ, GICV3_IRQ_GROUP_GRP1NS);
#endif
#else
    /* non-TZ */

    /* Init registributor interface */
    gicv3_gicr_init();

    /* Enable CPU interface access */
    // TODO: do we need to set bit 1 and 2? (IRQ/FIQ bypass)
    GICCREG_WRITE(0, icc_sre_el1, (GICCREG_READ(0, icc_sre_el1) | 0x7));
#endif

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
            enabled_spi_mask[i / 32] = GICDREG_READ(0, GICD_ISENABLER(i / 32));
        }
    }
    enabled_ppi_mask[cpu] = GICRREG_READ(0, cpu, GICR_ISENABLER0);
}

void arm_gicv3_resume_cpu_locked(unsigned int cpu, bool gicd) {
    uint32_t i;
    ASSERT(cpu < SMP_MAX_CPUS);

    GICRREG_WRITE(0, cpu, GICR_ISENABLER0, enabled_ppi_mask[cpu]);

    if (gicd) {
        /* also resume gicd */
        for (i = 32; i < MAX_INT; i += 32) {
          GICDREG_WRITE(0, GICD_ISENABLER(i / 32), enabled_spi_mask[i / 32]);
        }
    }
}

#define SGIR_AFF1_SHIFT (16)
#define SGIR_AFF2_SHIFT (32)
#define SGIR_AFF3_SHIFT (48)
#define SGIR_IRQ_SHIFT (24)
#define SGIR_RS_SHIFT (44)
#define SGIR_TARGET_LIST_SHIFT (0)
#define SGIR_ASSEMBLE(val, shift) ((uint64_t)val << shift)

uint64_t arm_gicv3_sgir_val(u_int irq, size_t cpu_num) {
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
