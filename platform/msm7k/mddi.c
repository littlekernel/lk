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
#include <stdlib.h>
#include <string.h>
#include <dev/fbcon.h>
#include <kernel/thread.h>
#include <platform/iomap.h>
#include <platform/mddi.h>

#include "mddi_hw.h"

static mddi_llentry *mlist = NULL;
static mddi_llentry *mlist_remote_write = NULL;

#define MDDI_MAX_REV_PKT_SIZE 0x60
#define MDDI_REV_PKT_BUF_SIZE (MDDI_MAX_REV_PKT_SIZE * 4)
static void *rev_pkt_buf;

/* functions provided by the target specific panel code */
void panel_init(struct mddi_client_caps *client_caps);
void panel_poweron(void);
void panel_backlight(int on);

/* forward decls */
static void mddi_start_update(void);
static int mddi_update_done(void);

static struct fbcon_config fb_cfg = {
	.format		= FB_FORMAT_RGB565,
	.bpp		= 16,
	.update_start	= mddi_start_update,
	.update_done	= mddi_update_done,
};

static void printcaps(struct mddi_client_caps *c)
{
	if ((c->length != 0x4a) || (c->type != 0x42)) {
		dprintf(INFO, "bad caps header\n");
		memset(c, 0, sizeof(*c));
		return;
	}

	dprintf(INFO, "mddi: bm: %d,%d win %d,%d rgb %x\n",
		c->bitmap_width, c->bitmap_height,
		c->display_window_width, c->display_window_height,
		c->rgb_cap);
	dprintf(INFO, "mddi: vend %x prod %x\n",
		c->manufacturer_name, c->product_code);
}

/* TODO: add timeout */
static int mddi_wait_status(unsigned statmask)
{
	while ((readl(MDDI_STAT) & statmask) == 0);
	return 0;
}

/* TODO: add timeout */
static int mddi_wait_interrupt(unsigned intmask)
{
	while ((readl(MDDI_INT) & intmask) == 0);
	return 0;
}

void mddi_remote_write(unsigned val, unsigned reg)
{
	mddi_llentry *ll;
	mddi_register_access *ra;

	ll = mlist_remote_write;
	
	ra = &(ll->u.r);
	ra->length = 14 + 4;
	ra->type = TYPE_REGISTER_ACCESS;
	ra->client_id = 0;
	ra->rw_info = MDDI_WRITE | 1;
	ra->crc = 0;

	ra->reg_addr = reg;
	ra->reg_data = val;

	ll->flags = 1;
	ll->header_count = 14;
	ll->data_count = 4;
	ll->data = &ra->reg_data;
	ll->next = (void *) 0;
	ll->reserved = 0;

	writel((unsigned) ll, MDDI_PRI_PTR);

	mddi_wait_status(MDDI_STAT_PRI_LINK_LIST_DONE);
}

static void mddi_start_update(void)
{
	writel((unsigned) mlist, MDDI_PRI_PTR);
}

static int mddi_update_done(void)
{
	return !!(readl(MDDI_STAT) & MDDI_STAT_PRI_LINK_LIST_DONE);
}

static void mddi_do_cmd(unsigned cmd)
{
	writel(cmd, MDDI_CMD);
	mddi_wait_interrupt(MDDI_INT_NO_REQ_PKTS_PENDING);
}

static void mddi_init_rev_encap(void)
{
	memset(rev_pkt_buf, 0xee, MDDI_REV_PKT_BUF_SIZE);
	writel((unsigned) rev_pkt_buf, MDDI_REV_PTR);
	writel((unsigned) rev_pkt_buf, MDDI_REV_PTR);
	mddi_do_cmd(CMD_FORCE_NEW_REV_PTR);
}

static void mddi_set_auto_hibernate(unsigned on)
{
	writel(CMD_POWER_DOWN, MDDI_CMD);
	mddi_wait_interrupt(MDDI_INT_IN_HIBERNATION);
	mddi_do_cmd(CMD_HIBERNATE | !!on);
}

static void mddi_get_caps(struct mddi_client_caps *caps)
{
	unsigned timeout = 100000;
	unsigned n;

	writel(0xffffffff, MDDI_INT);
	mddi_do_cmd(CMD_LINK_ACTIVE);

	/* sometimes this will fail -- do it three times for luck... */
	mddi_do_cmd(CMD_RTD_MEASURE);
	thread_sleep(1);//mdelay(1);

	mddi_do_cmd(CMD_RTD_MEASURE);
	thread_sleep(1);//mdelay(1);

	mddi_do_cmd(CMD_RTD_MEASURE);
	thread_sleep(1);//mdelay(1);

	mddi_do_cmd(CMD_GET_CLIENT_CAP);

	do {
		n = readl(MDDI_INT);
	} while (!(n & MDDI_INT_REV_DATA_AVAIL) && (--timeout));
	
	if (timeout == 0)
		dprintf(INFO, "timeout\n");

	memcpy(caps, rev_pkt_buf, sizeof(struct mddi_client_caps));
}

static unsigned mddi_init_regs(void)
{
	mddi_set_auto_hibernate(0);
	mddi_do_cmd(CMD_RESET);

	mddi_do_cmd(CMD_PERIODIC_REV_ENC);

	writel(0x0001, MDDI_VERSION);
	writel(0x3C00, MDDI_BPS);
	writel(0x0003, MDDI_SPM);

	writel(0x0005, MDDI_TA1_LEN);
	writel(0x000C, MDDI_TA2_LEN);
	writel(0x0096, MDDI_DRIVE_HI);
	writel(0x0050, MDDI_DRIVE_LO);
	writel(0x003C, MDDI_DISP_WAKE);
	writel(0x0002, MDDI_REV_RATE_DIV);

	writel(MDDI_REV_PKT_BUF_SIZE, MDDI_REV_SIZE);
//	writel(MDDI_REV_PKT_BUF_SIZE, MDDI_REV_ENCAP_SZ);
	writel(MDDI_MAX_REV_PKT_SIZE, MDDI_REV_ENCAP_SZ);

	mddi_do_cmd(CMD_PERIODIC_REV_ENC);

	/* needs to settle for 5uS */
	if (readl(MDDI_PAD_CTL) == 0) {
		writel(0x08000, MDDI_PAD_CTL);
		thread_sleep(1);//udelay(5);
	}

	writel(0xA850F, MDDI_PAD_CTL);
	writel(0x60006, MDDI_DRIVER_START_CNT);

	/* disable hibernate */
	mddi_set_auto_hibernate(0);
	mddi_do_cmd(CMD_IGNORE);

	mddi_init_rev_encap();
	return readl(MDDI_CORE_VER) & 0xffff;
}

struct fbcon_config *mddi_init(void)
{
	unsigned n;
	struct mddi_client_caps client_caps;

	dprintf(INFO, "mddi_init()\n");

	rev_pkt_buf = memalign(32, MDDI_REV_PKT_BUF_SIZE);
	mlist_remote_write = memalign(32, sizeof(struct mddi_llentry));

	n = mddi_init_regs();
	dprintf(INFO, "mddi version: 0x%08x\n", n);

	mddi_get_caps(&client_caps);
	ASSERT(client_caps.length == 0x4a && client_caps.type == 0x42);

	fb_cfg.width = client_caps.bitmap_width;
	fb_cfg.stride = fb_cfg.width;
	fb_cfg.height = client_caps.bitmap_height;

	printcaps(&client_caps);

	panel_init(&client_caps);

	panel_backlight(0);
	panel_poweron();

	/* v > 8?  v > 8 && < 0x19 ? */
	writel(2, MDDI_TEST);

	dprintf(INFO, "panel is %d x %d\n", fb_cfg.width, fb_cfg.height);

	fb_cfg.base =
		memalign(4096, fb_cfg.width * fb_cfg.height * (fb_cfg.bpp / 8));

	mlist = memalign(32, sizeof(mddi_llentry) * (fb_cfg.height / 8));
	dprintf(INFO, "FB @ %p  mlist @ %x\n", fb_cfg.base, (unsigned) mlist);
	
	for(n = 0; n < (fb_cfg.height / 8); n++) {
		unsigned y = n * 8;
		unsigned pixels = fb_cfg.width * 8;
		mddi_video_stream *vs = &(mlist[n].u.v);

		vs->length = sizeof(mddi_video_stream) - 2 + (pixels * 2);
		vs->type = TYPE_VIDEO_STREAM;
		vs->client_id = 0;
		vs->format = 0x5565; // FORMAT_16BPP;
		vs->pixattr = PIXATTR_BOTH_EYES | PIXATTR_TO_ALL;
		
		vs->left = 0;
		vs->right = fb_cfg.width - 1;
		vs->top = y;
		vs->bottom = y + 7;
		
		vs->start_x = 0;
		vs->start_y = y;
		
		vs->pixels = pixels;
		vs->crc = 0;
		vs->reserved = 0;
		
		mlist[n].header_count = sizeof(mddi_video_stream) - 2;
		mlist[n].data_count = pixels * 2;
		mlist[n].reserved = 0;
		mlist[n].data = fb_cfg.base + (y * fb_cfg.width * 2);
		mlist[n].next = &mlist[n + 1];
		mlist[n].flags = 0;
	}

	mlist[n-1].flags = 1;
	mlist[n-1].next = 0;

	mddi_set_auto_hibernate(1);
	mddi_do_cmd(CMD_LINK_ACTIVE);

	panel_backlight(1);

	return &fb_cfg;
}
