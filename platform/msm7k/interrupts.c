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

#include <debug.h>
#include <arch/arm.h>
#include <reg.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>

#include <platform/irqs.h>
#include <platform/iomap.h>

#define VIC_REG(off) (MSM_VIC_BASE + (off))

#define VIC_INT_SELECT0     VIC_REG(0x0000)  /* 1: FIQ, 0: IRQ */
#define VIC_INT_SELECT1     VIC_REG(0x0004)  /* 1: FIQ, 0: IRQ */
#define VIC_INT_EN0         VIC_REG(0x0010)
#define VIC_INT_EN1         VIC_REG(0x0014)
#define VIC_INT_ENCLEAR0    VIC_REG(0x0020)
#define VIC_INT_ENCLEAR1    VIC_REG(0x0024)
#define VIC_INT_ENSET0      VIC_REG(0x0030)
#define VIC_INT_ENSET1      VIC_REG(0x0034)
#define VIC_INT_TYPE0       VIC_REG(0x0040)  /* 1: EDGE, 0: LEVEL  */
#define VIC_INT_TYPE1       VIC_REG(0x0044)  /* 1: EDGE, 0: LEVEL  */
#define VIC_INT_POLARITY0   VIC_REG(0x0050)  /* 1: NEG, 0: POS */
#define VIC_INT_POLARITY1   VIC_REG(0x0054)  /* 1: NEG, 0: POS */
#define VIC_NO_PEND_VAL     VIC_REG(0x0060)
#define VIC_INT_MASTEREN    VIC_REG(0x0064)  /* 1: IRQ, 2: FIQ     */
#define VIC_PROTECTION      VIC_REG(0x006C)  /* 1: ENABLE          */
#define VIC_CONFIG          VIC_REG(0x0068)  /* 1: USE ARM1136 VIC */
#define VIC_IRQ_STATUS0     VIC_REG(0x0080)
#define VIC_IRQ_STATUS1     VIC_REG(0x0084)
#define VIC_FIQ_STATUS0     VIC_REG(0x0090)
#define VIC_FIQ_STATUS1     VIC_REG(0x0094)
#define VIC_RAW_STATUS0     VIC_REG(0x00A0)
#define VIC_RAW_STATUS1     VIC_REG(0x00A4)
#define VIC_INT_CLEAR0      VIC_REG(0x00B0)
#define VIC_INT_CLEAR1      VIC_REG(0x00B4)
#define VIC_SOFTINT0        VIC_REG(0x00C0)
#define VIC_SOFTINT1        VIC_REG(0x00C4)
#define VIC_IRQ_VEC_RD      VIC_REG(0x00D0)  /* pending int # */
#define VIC_IRQ_VEC_PEND_RD VIC_REG(0x00D4)  /* pending vector addr */
#define VIC_IRQ_VEC_WR      VIC_REG(0x00D8)
#define VIC_IRQ_IN_SERVICE  VIC_REG(0x00E0)
#define VIC_IRQ_IN_STACK    VIC_REG(0x00E4)
#define VIC_TEST_BUS_SEL    VIC_REG(0x00E8)

struct ihandler {
	int_handler func;
	void *arg;
};

static struct ihandler handler[NR_IRQS];

void platform_init_interrupts(void)
{
	writel(0xffffffff, VIC_INT_CLEAR0);
	writel(0xffffffff, VIC_INT_CLEAR1);
	writel(0, VIC_INT_SELECT0);
	writel(0, VIC_INT_SELECT1);
	writel(0xffffffff, VIC_INT_TYPE0);
	writel(0xffffffff, VIC_INT_TYPE1);
	writel(0, VIC_CONFIG);
	writel(1, VIC_INT_MASTEREN);
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
	unsigned num;
	enum handler_return ret;
	num = readl(VIC_IRQ_VEC_RD);
	num = readl(VIC_IRQ_VEC_PEND_RD);
	if (num > NR_IRQS)
		return 0;
	writel(1 << (num & 31), (num > 31) ? VIC_INT_CLEAR1 : VIC_INT_CLEAR0);
	ret = handler[num].func(handler[num].arg);
	writel(0, VIC_IRQ_VEC_WR);
	return ret;
}

void platform_fiq(struct arm_iframe *frame)
{
	PANIC_UNIMPLEMENTED;
}

status_t mask_interrupt(unsigned int vector)
{
	unsigned reg = (vector > 31) ? VIC_INT_ENCLEAR1 : VIC_INT_ENCLEAR0;
	unsigned bit = 1 << (vector & 31);
	writel(bit, reg);
	return 0;
}

status_t unmask_interrupt(unsigned int vector)
{
	unsigned reg = (vector > 31) ? VIC_INT_ENSET1 : VIC_INT_ENSET0;
	unsigned bit = 1 << (vector & 31);
	writel(bit, reg);
	return 0;
}

void register_int_handler(unsigned int vector, int_handler func, void *arg)
{
	if (vector >= NR_IRQS)
		return;

	enter_critical_section();
	handler[vector].func = func;
	handler[vector].arg = arg;
	exit_critical_section();
}

