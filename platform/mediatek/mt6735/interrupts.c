/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arch/arm.h>
#include <reg.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <sync_write.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_gpt.h>
#include <platform/mt_irq.h>

#include <debug.h>

#define GIC_ICDISER0    (GIC_DIST_BASE + 0x100)
#define GIC_ICDISER1    (GIC_DIST_BASE + 0x104)
#define GIC_ICDISER2    (GIC_DIST_BASE + 0x108)
#define GIC_ICDISER3    (GIC_DIST_BASE + 0x10C)
#define GIC_ICDISER4    (GIC_DIST_BASE + 0x110)
#define GIC_ICDISER5    (GIC_DIST_BASE + 0x114)
#define GIC_ICDISER6    (GIC_DIST_BASE + 0x118)
#define GIC_ICDISER7    (GIC_DIST_BASE + 0x11C)

#define GIC_ICDICER0    (GIC_DIST_BASE + 0x180)
#define GIC_ICDICER1    (GIC_DIST_BASE + 0x184)
#define GIC_ICDICER2    (GIC_DIST_BASE + 0x188)
#define GIC_ICDICER3    (GIC_DIST_BASE + 0x18C)
#define GIC_ICDICER4    (GIC_DIST_BASE + 0x190)
#define GIC_ICDICER5    (GIC_DIST_BASE + 0x194)
#define GIC_ICDICER6    (GIC_DIST_BASE + 0x198)
#define GIC_ICDICER7    (GIC_DIST_BASE + 0x19C)

#define INT_POL_CTL0  (MCUCFG_BASE + 0x620)

static void mt_gic_cpu_init(void)
{
    DRV_WriteReg32(GIC_CPU_BASE + GIC_CPU_PRIMASK, 0xF0);
    DRV_WriteReg32(GIC_CPU_BASE + GIC_CPU_CTRL, 0x1);
    dsb();
}

static void mt_gic_dist_init(void)
{
    unsigned int i;
#ifndef MTK_FORCE_CLUSTER1
    unsigned int cpumask = 1 << 0;
#else
    unsigned int cpumask = 1 << 4;
#endif

    cpumask |= cpumask << 8;
    cpumask |= cpumask << 16;

    DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CTRL, 0);

    /*
     * Set all global interrupts to be level triggered, active low.
     */
    for (i = 32; i < (MT_NR_SPI + 32); i += 16) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CONFIG + i * 4 / 16, 0);
    }

    /*
     * Set all global interrupts to this CPU only.
     */
    for (i = 32; i < (MT_NR_SPI + 32); i += 4) {
        DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_TARGET + i * 4 / 4, cpumask);
    }

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

dsb();

    DRV_WriteReg32(GIC_DIST_BASE + GIC_DIST_CTRL, 1);
}

void platform_init_interrupts(void)
{
	mt_gic_dist_init();
	mt_gic_cpu_init();
}

void platform_deinit_interrupts(void)
{
        unsigned int irq;

        DRV_WriteReg32(GIC_ICDICER0, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER1, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER2, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER3, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER4, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER5, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER6, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER7, 0xFFFFFFFF);
        dsb();

        while((irq = DRV_Reg32(GIC_CPU_BASE + GIC_CPU_INTACK)) != 1023 ) {
            DRV_WriteReg32(GIC_CPU_BASE + GIC_CPU_EOI, irq);
        }
}

extern void lk_scheduler(void);
//extern void lk_usb_scheduler(void);
extern void lk_nand_irq_handler(unsigned int irq);
//extern void lk_msdc_irq_handler(unsigned int irq);
#ifdef DUMMY_AP
extern void dummy_ap_irq_handler(unsigned int irq);
#endif

enum handler_return platform_irq(struct arm_iframe *frame)
{
	unsigned int irq = DRV_Reg32(GIC_CPU_BASE + GIC_CPU_INTACK);

	if(irq == MT_GPT_IRQ_ID)
		lk_scheduler();
//	else if(irq == MT_USB0_IRQ_ID)
//		lk_usb_scheduler();
#ifndef MTK_EMMC_SUPPORT
//	else if(irq == MT_NFI_IRQ_ID)
//		lk_nand_irq_handler(irq);
#endif
//	else if(irq == MT_MSDC0_IRQ_ID || irq == MT_MSDC1_IRQ_ID)
//		lk_msdc_irq_handler(irq);

#ifdef DUMMY_AP
	dummy_ap_irq_handler(irq);
#endif

	//return INT_NO_RESCHEDULE;
	return INT_RESCHEDULE;
}

void platform_fiq(struct arm_iframe *frame)
{

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
    }else {
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
    }else {
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
 * mt_irq_mask: mask one IRQ
 * @irq: IRQ line of the IRQ to mask
 */
void mt_irq_ack(unsigned int irq)
{
    DRV_WriteReg32(GIC_CPU_BASE + GIC_CPU_EOI, irq);
    dsb();
}

/*
 * mt_irq_mask_all: mask all IRQ lines. (This is ONLY used for the sleep driver)
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 */
int mt_irq_mask_all(struct mtk_irq_mask *mask)
{
    if (mask) {

        mask->mask0 = DRV_Reg32(GIC_ICDISER0);
        mask->mask1 = DRV_Reg32(GIC_ICDISER1);
        mask->mask2 = DRV_Reg32(GIC_ICDISER2);
        mask->mask3 = DRV_Reg32(GIC_ICDISER3);
        mask->mask4 = DRV_Reg32(GIC_ICDISER4);
        mask->mask5 = DRV_Reg32(GIC_ICDISER5);
        mask->mask6 = DRV_Reg32(GIC_ICDISER6);
        mask->mask7 = DRV_Reg32(GIC_ICDISER7);

        DRV_WriteReg32(GIC_ICDICER0, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER1, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER2, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER3, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER4, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER5, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER6, 0xFFFFFFFF);
        DRV_WriteReg32(GIC_ICDICER7, 0xFFFFFFFF);

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
    if (!mask) {
        return -1;
    }
    if (mask->header != IRQ_MASK_HEADER) {
        return -1;
    }
    if (mask->footer != IRQ_MASK_FOOTER) {
        return -1;
    }

    DRV_WriteReg32(GIC_ICDISER0,mask->mask0);
    DRV_WriteReg32(GIC_ICDISER1,mask->mask1);
    DRV_WriteReg32(GIC_ICDISER2,mask->mask2);
    DRV_WriteReg32(GIC_ICDISER3,mask->mask3);
    DRV_WriteReg32(GIC_ICDISER4,mask->mask4);
    DRV_WriteReg32(GIC_ICDISER5,mask->mask5);
    DRV_WriteReg32(GIC_ICDISER6,mask->mask6);
    DRV_WriteReg32(GIC_ICDISER7,mask->mask7);

    dsb();


    return 0;
}
