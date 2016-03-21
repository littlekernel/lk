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
#include <reg.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_irq.h>
#include <mt_gic.h>
#include <sync_write.h>
#include <debug.h>

#define GICD_CTLR_RWP       (1 << 31)
#define GICD_CTLR_ARE       (1 << 4)
#define GICD_CTLR_ENGRP1S   (1 << 2)
#define GICD_CTLR_ENGRP1NS  (1 << 1)
#define GICR_WAKER_ProcessorSleep   (1 << 1)
#define GICR_WAKER_ChildrenAsleep   (1 << 2)

extern uint32_t mt_interrupt_needed_for_secure(void);
extern uint64_t mt_irq_get_affinity(void);

static void mt_gic_icc_primask_write(uint32_t reg)
{
    __asm__ volatile("MCR p15, 0, %0, c4, c6, 0" :: "r" (reg));
}

static uint32_t mt_gic_icc_primask_read(void)
{
    uint32_t reg;

    __asm__ volatile("MRC p15, 0, %0, c4, c6, 0" : "=r" (reg));

    return reg;
}

static void mt_gic_icc_igrpen1_write(uint32_t reg)
{
    __asm__ volatile("MCR p15, 0, %0, c12, c12, 7" :: "r" (reg));
}

static uint32_t mt_gic_icc_igrpen1_read(void)
{
    uint32_t reg;

    __asm__ volatile("MRC p15, 0, %0, c12, c12, 7" : "=r" (reg));

    return reg;
}

static uint32_t mt_gic_icc_iar1_read(void)
{
    uint32_t reg;

    __asm__ volatile("MRC p15, 0, %0, c12, c12, 0" : "=r" (reg));

    return reg;
}

static void mt_gic_icc_msre_write(void)
{
    uint32_t reg;

#define MON_MODE    "#22"
#define SVC_MODE    "#19"

    /*
     * switch to monitor mode and mark ICC_MSRE.
     */
    __asm__ volatile("CPS " MON_MODE "\n"
                     "MRC p15, 6, %0, c12, c12, 5\n"
                     "ORR %0, %0, #9\n"
                     "MCR p15, 6, %0, c12, c12, 5\n"
                     "CPS " SVC_MODE "\n" : "=r" (reg));

    dsb();
}

static void mt_gic_icc_sre_write(uint32_t reg)
{
    __asm__ volatile("MCR p15, 0, %0, c12, c12, 5" :: "r" (reg));
    dsb();
}

static uint32_t mt_gic_icc_sre_read(void)
{
    uint32_t reg;

    __asm__ volatile("MRC p15, 0, %0, c12, c12, 5" : "=r" (reg));

    return reg;
}

static void mt_gic_icc_eoir1_write(uint32_t reg)
{
    __asm__ volatile("MCR p15, 0, %0, c12, c12, 1" :: "r" (reg));
}

uint32_t mt_mpidr_read(void)
{
    uint32_t reg;

    __asm__ volatile("MRC p15, 0, %0, c0, c0, 5" : "=r" (reg));

    return reg;
}

static void mt_gic_cpu_init(void)
{
    mt_gic_icc_sre_write(0x01);
    mt_gic_icc_primask_write(0xF0);
    mt_gic_icc_igrpen1_write(0x01);
    dsb();
}

static void mt_gic_redist_init(void)
{
    unsigned int value;

    /* Wake up this CPU redistributor */
    value = DRV_Reg32(GIC_REDIS_BASE + GIC_REDIS_WAKER);
    value &= ~GICR_WAKER_ProcessorSleep;
    DRV_WriteReg32(GIC_REDIS_BASE + GIC_REDIS_WAKER, value);

    while (DRV_Reg32(GIC_REDIS_BASE + GIC_REDIS_WAKER) & GICR_WAKER_ChildrenAsleep);
}

static void mt_git_dist_rwp(void)
{
    /*
     * check GICD_CTLR.RWP for done check
     */
    while (DRV_Reg32(GIC_DIST_BASE + GIC_DIST_CTRL) & GICD_CTLR_RWP) {

    }
}

static void mt_gic_dist_init(void)
{
    unsigned int i;
    uint64_t affinity;

    affinity = mt_irq_get_affinity();

    DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CTRL, GICD_CTLR_ARE);

    mt_git_dist_rwp();

    /*
     * Set all global interrupts to be level triggered, active low.
     */
    for (i = 32; i < (MT_NR_SPI + 32); i += 16) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CONFIG + i * 4 / 16, 0);
    }

    /*
     * Set all global interrupts to this CPU only.
     */
    for (i = 0; i < MT_NR_SPI; i++) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ROUTE + i * 8, (affinity & 0xFFFFFFFF));
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ROUTE + i * 8 + 4, (affinity >> 32));
    }

    /*
     * Set all interrupts to G1S.  Leave the PPI and SGIs alone
     * as they are set by redistributor registers.
     */
    for (i = 0; i < NR_IRQ_LINE; i += 32)
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_IGRPMODR + i / 8, 0xFFFFFFFF);

    /*
     * Set priority on all interrupts.
     */
    for (i = 0; i < NR_IRQ_LINE; i += 4) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_PRI + i * 4 / 4, 0xA0A0A0A0);
    }

    /*
     * Disable all interrupts.
     */
    for (i = 0; i < NR_IRQ_LINE; i += 32) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + i * 4 / 32, 0xFFFFFFFF);
    }

    /*
     * Clear all active status
     */
    for (i = 0; i < NR_IRQ_LINE; i += 32) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ACTIVE_CLEAR + i * 4 / 32, 0xFFFFFFFF);
    }

    /*
     * Clear all pending status
     */
    for (i = 0; i < NR_IRQ_LINE; i += 32) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_PENDING_CLEAR + i * 4 / 32, 0xFFFFFFFF);
    }


    dsb();
    mt_git_dist_rwp();
    DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CTRL, GICD_CTLR_ARE | GICD_CTLR_ENGRP1S | GICD_CTLR_ENGRP1NS);
    mt_git_dist_rwp();
}

void platform_init_interrupts(void)
{
    uint32_t sec;

    sec = mt_interrupt_needed_for_secure();

    if (sec)
        mt_gic_icc_msre_write();

    mt_gic_dist_init();

    if (sec)
        mt_gic_redist_init();

    mt_gic_cpu_init();
}

void platform_deinit_interrupts(void)
{
    unsigned int irq;

    for (irq = 0; irq < NR_IRQ_LINE; irq += 32) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + irq * 4 / 32, 0xFFFFFFFF);
    }

    dsb();

    while ((irq = mt_gic_icc_iar1_read()) != 1023 ) {
        mt_gic_icc_eoir1_write(irq);
    }
}

uint32_t mt_irq_get(void)
{
    return mt_gic_icc_iar1_read();
}

void mt_irq_set_polarity(unsigned int irq, unsigned int polarity)
{
    unsigned int offset;
    unsigned int reg_index;
    unsigned int value;

    // peripheral device's IRQ line is using GIC's SPI, and line ID >= GIC_PRIVATE_SIGNALS
    if (irq < GIC_PRIVATE_SIGNALS) {
        return;
    }

    offset = (irq - GIC_PRIVATE_SIGNALS) & 0x1F;
    reg_index = (irq - GIC_PRIVATE_SIGNALS) >> 5;
    if (polarity == 0) {
        value = DRV_Reg32(INT_POL_CTL0 + (reg_index * 4));
        value |= (1 << offset); // always invert the incoming IRQ's polarity
        DRV_WriteReg32((INT_POL_CTL0 + (reg_index * 4)), value);
    } else {
        value = DRV_Reg32(INT_POL_CTL0 + (reg_index * 4));
        value &= ~(0x1 << offset);
        DRV_WriteReg32(INT_POL_CTL0 + (reg_index * 4), value);
    }
}

void mt_irq_set_sens(unsigned int irq, unsigned int sens)
{
    unsigned int config;

    if (sens == MT65xx_EDGE_SENSITIVE) {
        config = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
        config |= (0x2 << (irq % 16) * 2);
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4, config);
    } else {
        config = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
        config &= ~(0x2 << (irq % 16) * 2);
        DRV_WriteReg32( GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4, config);
    }
    dsb();
}

/*
 * mt_irq_mask: mask one IRQ
 * @irq: IRQ line of the IRQ to mask
 */
void mt_irq_mask(unsigned int irq)
{
    unsigned int mask = 1 << (irq % 32);

    DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + irq / 32 * 4, mask);
    dsb();
}

/*
 * mt_irq_unmask: unmask one IRQ
 * @irq: IRQ line of the IRQ to unmask
 */
void mt_irq_unmask(unsigned int irq)
{
    unsigned int mask = 1 << (irq % 32);

    DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4, mask);
    dsb();
}

/*
 * mt_irq_ack: ack IRQ
 * @irq: IRQ line of the IRQ to mask
 */
void mt_irq_ack(unsigned int irq)
{
    mt_gic_icc_eoir1_write(irq);
    dsb();
}

/*
 * mt_irq_mask_all: mask all IRQ lines. (This is ONLY used for the sleep driver)
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_irq_mask_all(struct mtk_irq_mask *mask)
{
    unsigned int i;

    if (mask) {
        for (i = 0; i < IRQ_REGS; i++) {
            mask->mask[i] = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + i * 4);
            DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR + i * 4, 0xFFFFFFFF);
        }

        dsb();

        mask->header = IRQ_MASK_HEADER;
        mask->footer = IRQ_MASK_FOOTER;

        return 0;
    } else {
        return -1;
    }
}

/*
 * mt_irq_mask_restore: restore all IRQ lines' masks. (This is ONLY used for the sleep driver)
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_irq_mask_restore(struct mtk_irq_mask *mask)
{
    unsigned int i;

    if (!mask) {
        return -1;
    }
    if (mask->header != IRQ_MASK_HEADER) {
        return -1;
    }
    if (mask->footer != IRQ_MASK_FOOTER) {
        return -1;
    }

    for (i = 0; i < IRQ_REGS; i++) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + i * 4, mask->mask[i]);
    }

    dsb();


    return 0;
}

void mt_irq_register_dump(void)
{
    int i;
    uint32_t reg, reg2;

    dprintf(CRITICAL, "%s(): do irq register dump\n", __func__);

    reg = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_CTRL);
    dprintf(CRITICAL, "GICD_CTLR: 0x%08x\n", reg);

    for (i = 0; i < MT_NR_SPI; i++) {
        reg = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_ROUTE + i * 8);
        reg2 = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_ROUTE + i * 8 + 4);
        dprintf(CRITICAL, "GICD_IROUTER[%d]: 0x%08x, 0x%08x\n", i, reg, reg2);
    }

    for (i = 0; i < NR_IRQ_LINE; i += 32) {
        reg = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_IGRPMODR + i / 8);
        dprintf(CRITICAL, "GICD_IGRPMODR[%d]: 0x%08x\n", i >> 5, reg);
    }

    for (i = 0; i < NR_IRQ_LINE; i += 4) {
        reg = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_PRI + i * 4 / 4);
        dprintf(CRITICAL, "GICD_IPRIORITYR[%d]: 0x%08x\n", i >> 2, reg);
    }

    for (i = 32; i < (MT_NR_SPI + 32); i += 16) {
        reg = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_CONFIG + i * 4 / 16);
        dprintf(CRITICAL, "DIST_ICFGR[%d]: 0x%08x\n", (i >> 4) - 2, reg);
    }

    for (i = 0; i < IRQ_REGS; i++) {
        reg = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + i * 4);
        dprintf(CRITICAL, "GICD_ISENABLER[%d]: 0x%08x\n", i, reg);
    }

    for (i = 0; i < IRQ_REGS; i++) {
        reg = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_PENDING_SET + i * 4);
        dprintf(CRITICAL, "GICD_ISPENDR[%d]: 0x%08x\n", i, reg);
    }

    for (i = 0; i < IRQ_REGS; i++) {
        reg = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_ACTIVE_SET + i * 4);
        dprintf(CRITICAL, "GICD_ISACTIVER[%d]: 0x%08x\n", i, reg);
    }

    reg = mt_gic_icc_sre_read();
    dprintf(CRITICAL, "ICC_SRE: 0x%08x\n", reg);

    reg = mt_gic_icc_primask_read();
    dprintf(CRITICAL, "ICC_PMR: 0x%08x\n", reg);

    reg = mt_gic_icc_igrpen1_read();
    dprintf(CRITICAL, "ICC_IGRPEN1: 0x%08x\n", reg);

    reg = mt_gic_icc_iar1_read();
    dprintf(CRITICAL, "ICC_IAR1: 0x%08x\n", reg);

    reg = mt_mpidr_read();
    dprintf(CRITICAL, "MPIDR: 0x%08x\n", reg);
}
