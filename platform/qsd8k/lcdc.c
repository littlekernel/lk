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
#include <stdlib.h>
#include <reg.h>
#include <platform/iomap.h>

#define MSM_MDP_BASE1 0xAA200000

unsigned fb_width  = 0;
unsigned fb_height = 0;

static unsigned short *FB;


#define LCDC_PIXCLK_IN_PS 26
#define LCDC_FB_PHYS      0x16600000
#define LCDC_FB_BPP       16

#if 1
/* SURF */
#define LCDC_FB_WIDTH     800
#define LCDC_FB_HEIGHT    480

#define LCDC_HSYNC_PULSE_WIDTH_DCLK 60
#define LCDC_HSYNC_BACK_PORCH_DCLK  81
#define LCDC_HSYNC_FRONT_PORCH_DCLK 81
#define LCDC_HSYNC_SKEW_DCLK        0

#define LCDC_VSYNC_PULSE_WIDTH_LINES 2
#define LCDC_VSYNC_BACK_PORCH_LINES  20
#define LCDC_VSYNC_FRONT_PORCH_LINES 27

#else
/* FFA */
#define LCDC_FB_WIDTH     480
#define LCDC_FB_HEIGHT    640

#define LCDC_HSYNC_PULSE_WIDTH_DCLK 60
#define LCDC_HSYNC_BACK_PORCH_DCLK  144
#define LCDC_HSYNC_FRONT_PORCH_DCLK 33
#define LCDC_HSYNC_SKEW_DCLK        0

#define LCDC_VSYNC_PULSE_WIDTH_LINES 2
#define LCDC_VSYNC_BACK_PORCH_LINES  2
#define LCDC_VSYNC_FRONT_PORCH_LINES 2

#endif

#define BIT(x)  (1<<(x))
#define DMA_DSTC0G_8BITS (BIT(1)|BIT(0))
#define DMA_DSTC1B_8BITS (BIT(3)|BIT(2))
#define DMA_DSTC2R_8BITS (BIT(5)|BIT(4))
#define CLR_G 0x0
#define CLR_B 0x1
#define CLR_R 0x2
#define MDP_GET_PACK_PATTERN(a,x,y,z,bit) (((a)<<(bit*3))|((x)<<(bit*2))|((y)<<bit)|(z))
#define DMA_PACK_ALIGN_LSB 0
#define DMA_PACK_PATTERN_RGB					\
        (MDP_GET_PACK_PATTERN(0,CLR_R,CLR_G,CLR_B,2)<<8)
#define DMA_DITHER_EN                       BIT(24)
#define DMA_OUT_SEL_LCDC                    BIT(20)
#define DMA_IBUF_FORMAT_RGB565              BIT(25)

void lcdc_clock_init(unsigned rate);

void lcdc_init(void)
{
	unsigned n;

	dprintf(INFO, "lcdc_init()\n");

	fb_width  = LCDC_FB_WIDTH;
	fb_height = LCDC_FB_HEIGHT;
	dprintf(INFO, "panel is %d x %d\n", fb_width, fb_height);

	FB = LCDC_FB_PHYS; //alloc(2 * fb_width * fb_height);

	FB = memalign(4096, 2 * fb_width * fb_height);
	dprintf(INFO, "FB %p\n", FB);

	lcdc_clock_init(1000000000 / LCDC_PIXCLK_IN_PS);

	writel(FB, MSM_MDP_BASE1 + 0x90008);
	writel((LCDC_FB_HEIGHT << 16) | LCDC_FB_WIDTH, MSM_MDP_BASE1 + 0x90004);
	writel(LCDC_FB_WIDTH * LCDC_FB_BPP / 8, MSM_MDP_BASE1 + 0x9000c);
	writel(0, MSM_MDP_BASE1 + 0x90010);

	writel(DMA_PACK_ALIGN_LSB|DMA_PACK_PATTERN_RGB|DMA_DITHER_EN|DMA_OUT_SEL_LCDC|
	       DMA_IBUF_FORMAT_RGB565|DMA_DSTC0G_8BITS|DMA_DSTC1B_8BITS|DMA_DSTC2R_8BITS,
	       MSM_MDP_BASE1 + 0x90000);

	int hsync_period  = LCDC_HSYNC_PULSE_WIDTH_DCLK + LCDC_HSYNC_BACK_PORCH_DCLK + LCDC_FB_WIDTH + LCDC_HSYNC_FRONT_PORCH_DCLK;
	int vsync_period  = (LCDC_VSYNC_PULSE_WIDTH_LINES + LCDC_VSYNC_BACK_PORCH_LINES + LCDC_FB_HEIGHT + LCDC_VSYNC_FRONT_PORCH_LINES) * hsync_period;
	int hsync_ctrl    = (hsync_period << 16) | LCDC_HSYNC_PULSE_WIDTH_DCLK;
	int hsync_start_x = LCDC_HSYNC_PULSE_WIDTH_DCLK + LCDC_HSYNC_BACK_PORCH_DCLK;
	int hsync_end_x   = hsync_period - LCDC_HSYNC_FRONT_PORCH_DCLK - 1;
	int display_hctl  = (hsync_end_x << 16) | hsync_start_x;
	int display_vstart= (LCDC_VSYNC_PULSE_WIDTH_LINES + LCDC_VSYNC_BACK_PORCH_LINES) * hsync_period + LCDC_HSYNC_SKEW_DCLK;
	int display_vend  = vsync_period - (LCDC_VSYNC_FRONT_PORCH_LINES * hsync_period) + LCDC_HSYNC_SKEW_DCLK - 1;

	writel((hsync_period << 16) | LCDC_HSYNC_PULSE_WIDTH_DCLK, MSM_MDP_BASE1 + 0xe0004);
	writel(vsync_period, MSM_MDP_BASE1 + 0xe0008);
	writel(LCDC_VSYNC_PULSE_WIDTH_LINES * hsync_period, MSM_MDP_BASE1 + 0xe000c);
	writel(display_hctl, MSM_MDP_BASE1 + 0xe0010);
	writel(display_vstart, MSM_MDP_BASE1 + 0xe0014);
	writel(display_vend, MSM_MDP_BASE1 + 0xe0018);
	writel(0, MSM_MDP_BASE1 + 0xe0028);
	writel(0xff, MSM_MDP_BASE1 + 0xe002c);
	writel(LCDC_HSYNC_SKEW_DCLK, MSM_MDP_BASE1 + 0xe0030);
	writel(0, MSM_MDP_BASE1 + 0xe0038);
	writel(0, MSM_MDP_BASE1 + 0xe001c);
	writel(0, MSM_MDP_BASE1 + 0xe0020);
	writel(0, MSM_MDP_BASE1 + 0xe0024);

	writel(1, MSM_MDP_BASE1 + 0xe0000);

	for(n = 0; n < (fb_width * fb_height); n++) FB[n] = 0x01f0;
}

void *mddi_framebuffer(void)
{
	return FB;
}

