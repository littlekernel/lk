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
#include <reg.h>
#include <platform/iomap.h>
#include <dev/gpio.h>

#define GPIO1_REG(off) (MSM_GPIO1_BASE + 0x800 + (off))
#define GPIO2_REG(off) (MSM_GPIO2_BASE + 0xC00 + (off))

/* output value */
#define GPIO_OUT_0         GPIO1_REG(0x00)  /* gpio  15-0  */
#define GPIO_OUT_1         GPIO2_REG(0x00)  /* gpio  42-16 */
#define GPIO_OUT_2         GPIO1_REG(0x04)  /* gpio  67-43 */
#define GPIO_OUT_3         GPIO1_REG(0x08)  /* gpio  94-68 */
#define GPIO_OUT_4         GPIO1_REG(0x0C)  /* gpio 106-95 */

/* same pin map as above, output enable */
#define GPIO_OE_0          GPIO1_REG(0x10)
#define GPIO_OE_1          GPIO2_REG(0x08)
#define GPIO_OE_2          GPIO1_REG(0x14)
#define GPIO_OE_3          GPIO1_REG(0x18)
#define GPIO_OE_4          GPIO1_REG(0x1C)

/* same pin map as above, input read */
#define GPIO_IN_0          GPIO1_REG(0x34)
#define GPIO_IN_1          GPIO2_REG(0x20)
#define GPIO_IN_2          GPIO1_REG(0x38)
#define GPIO_IN_3          GPIO1_REG(0x3C)
#define GPIO_IN_4          GPIO1_REG(0x40)

/* same pin map as above, 1=edge 0=level interrup */
#define GPIO_INT_EDGE_0    GPIO1_REG(0x60)
#define GPIO_INT_EDGE_1    GPIO2_REG(0x50)
#define GPIO_INT_EDGE_2    GPIO1_REG(0x64)
#define GPIO_INT_EDGE_3    GPIO1_REG(0x68)
#define GPIO_INT_EDGE_4    GPIO1_REG(0x6C)

/* same pin map as above, 1=positive 0=negative */
#define GPIO_INT_POS_0     GPIO1_REG(0x70)
#define GPIO_INT_POS_1     GPIO2_REG(0x58)
#define GPIO_INT_POS_2     GPIO1_REG(0x74)
#define GPIO_INT_POS_3     GPIO1_REG(0x78)
#define GPIO_INT_POS_4     GPIO1_REG(0x7C)

/* same pin map as above, interrupt enable */
#define GPIO_INT_EN_0      GPIO1_REG(0x80)
#define GPIO_INT_EN_1      GPIO2_REG(0x60)
#define GPIO_INT_EN_2      GPIO1_REG(0x84)
#define GPIO_INT_EN_3      GPIO1_REG(0x88)
#define GPIO_INT_EN_4      GPIO1_REG(0x8C)

/* same pin map as above, write 1 to clear interrupt */
#define GPIO_INT_CLEAR_0   GPIO1_REG(0x90)
#define GPIO_INT_CLEAR_1   GPIO2_REG(0x68)
#define GPIO_INT_CLEAR_2   GPIO1_REG(0x94)
#define GPIO_INT_CLEAR_3   GPIO1_REG(0x98)
#define GPIO_INT_CLEAR_4   GPIO1_REG(0x9C)

/* same pin map as above, 1=interrupt pending */
#define GPIO_INT_STATUS_0  GPIO1_REG(0xA0)
#define GPIO_INT_STATUS_1  GPIO2_REG(0x70)
#define GPIO_INT_STATUS_2  GPIO1_REG(0xA4)
#define GPIO_INT_STATUS_3  GPIO1_REG(0xA8)
#define GPIO_INT_STATUS_4  GPIO1_REG(0xAC)

typedef struct gpioregs gpioregs;

struct gpioregs
{
	unsigned out;
	unsigned in;
	unsigned int_status;
	unsigned int_clear;
	unsigned int_en;
	unsigned int_edge;
	unsigned int_pos;
	unsigned oe;
};

static gpioregs GPIO_REGS[] = {
	{
		.out =         GPIO_OUT_0,
		.in =          GPIO_IN_0,
		.int_status =  GPIO_INT_STATUS_0,
		.int_clear =   GPIO_INT_CLEAR_0,
		.int_en =      GPIO_INT_EN_0,
		.int_edge =    GPIO_INT_EDGE_0,
		.int_pos =     GPIO_INT_POS_0,
		.oe =          GPIO_OE_0,
	},
	{
		.out =         GPIO_OUT_1,
		.in =          GPIO_IN_1,
		.int_status =  GPIO_INT_STATUS_1,
		.int_clear =   GPIO_INT_CLEAR_1,
		.int_en =      GPIO_INT_EN_1,
		.int_edge =    GPIO_INT_EDGE_1,
		.int_pos =     GPIO_INT_POS_1,
		.oe =          GPIO_OE_1,
	},
	{
		.out =         GPIO_OUT_2,
		.in =          GPIO_IN_2,
		.int_status =  GPIO_INT_STATUS_2,
		.int_clear =   GPIO_INT_CLEAR_2,
		.int_en =      GPIO_INT_EN_2,
		.int_edge =    GPIO_INT_EDGE_2,
		.int_pos =     GPIO_INT_POS_2,
		.oe =          GPIO_OE_2,
	},
	{
		.out =         GPIO_OUT_3,
		.in =          GPIO_IN_3,
		.int_status =  GPIO_INT_STATUS_3,
		.int_clear =   GPIO_INT_CLEAR_3,
		.int_en =      GPIO_INT_EN_3,
		.int_edge =    GPIO_INT_EDGE_3,
		.int_pos =     GPIO_INT_POS_3,
		.oe =          GPIO_OE_3,
	},
	{
		.out =         GPIO_OUT_4,
		.in =          GPIO_IN_4,
		.int_status =  GPIO_INT_STATUS_4,
		.int_clear =   GPIO_INT_CLEAR_4,
		.int_en =      GPIO_INT_EN_4,
		.int_edge =    GPIO_INT_EDGE_4,
		.int_pos =     GPIO_INT_POS_4,
		.oe =          GPIO_OE_4,
	},
};

static gpioregs *find_gpio(unsigned n, unsigned *bit)
{
	if(n > 106)
		return 0;
	if(n > 94) {
		*bit = 1 << (n - 95);
		return GPIO_REGS + 4;
	}
	if(n > 67) {
		*bit = 1 << (n - 68);
		return GPIO_REGS + 3;
	}
	if(n > 42) {
		*bit = 1 << (n - 43);
		return GPIO_REGS + 2;
	}
	if(n > 15) {
		*bit = 1 << (n - 16);
		return GPIO_REGS + 1;
	}
	*bit = 1 << n;
	return GPIO_REGS + 0;
}

int gpio_config(unsigned n, unsigned flags)
{
	gpioregs *r;
	unsigned b;
	unsigned v;
    
	if ((r = find_gpio(n, &b)) == 0)
		return -1;

	v = readl(r->oe);
	if (flags & GPIO_OUTPUT) {
		writel(v | b, r->oe);
	} else {
		writel(v & (~b), r->oe);
	}
	return 0;
}

void gpio_set(unsigned n, unsigned on)
{
	gpioregs *r;
	unsigned b;
	unsigned v;
    
	if((r = find_gpio(n, &b)) == 0)
		return;

	v = readl(r->out);
	if(on) {
		writel(v | b, r->out);
	} else {
		writel(v & (~b), r->out);
	}
}

int gpio_get(unsigned n)
{
	gpioregs *r;
	unsigned b;
    
	if((r = find_gpio(n, &b)) == 0) return 0;

	return (readl(r->in) & b) ? 1 : 0;
}

