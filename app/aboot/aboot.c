/*
 * Copyright (c) 2009, Google Inc.
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

#include <app.h>
#include <debug.h>
#include <arch/arm.h>
#include <dev/udc.h>
#include <string.h>
#include <kernel/thread.h>
#include <arch/ops.h>

#include "bootimg.h"
#include "fastboot.h"

#define TAGS_ADDR	0x10000100
#define KERNEL_ADDR	0x10800000
#define RAMDISK_ADDR	0x11000000

static struct udc_device surf_udc_device = {
	.vendor_id	= 0x18d1,
	.product_id	= 0x0001,
	.version_id	= 0x0100,
	.manufacturer	= "Google",
	.product	= "Android",
};

void platform_uninit_timer(void);


void boot_linux(void *kernel, unsigned *tags, 
		const char *cmdline, unsigned machtype,
		void *ramdisk, unsigned ramdisk_size)
{
	unsigned *ptr = tags;
	void (*entry)(unsigned,unsigned,unsigned*) = kernel;

	/* CORE */
	*ptr++ = 2;
	*ptr++ = 0x54410001;

	if (ramdisk_size) {
		*ptr++ = 4;
		*ptr++ = 0x54420005;
		*ptr++ = RAMDISK_ADDR;
		*ptr++ = ramdisk_size;
	}

	if (cmdline && cmdline[0]) {
		unsigned n;
		/* include terminating 0 and round up to a word multiple */
		n = (strlen(cmdline) + 4) & (~3);
		*ptr++ = (n / 4) + 2;
		*ptr++ = 0x54410009;
		memcpy(ptr, cmdline, n);
		ptr += (n / 4);
	}

	/* END */
	*ptr++ = 0;
	*ptr++ = 0;

	dprintf(INFO, "booting linux @ %p, ramdisk @ %p (%d)\n",
		kernel, ramdisk, ramdisk_size);
	if (cmdline)
		dprintf(INFO, "cmdline: %s\n", cmdline);

	enter_critical_section();
	platform_uninit_timer();
	arch_disable_cache(UCACHE);
	arch_enable_cache(ICACHE);
	arch_disable_mmu();

	entry(0, machtype, tags);

}

#define PAGE_MASK 2047

#define ROUND_TO_PAGE(x) (((x) + PAGE_MASK) & (~PAGE_MASK))


void cmd_boot(const char *arg, void *data, unsigned sz)
{
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	static struct boot_img_hdr hdr;
	char *ptr = ((char*) data);

	if (sz < sizeof(hdr)) {
		fastboot_fail("invalid bootimage header");
		return;
	}

	memcpy(&hdr, data, sizeof(hdr));

	/* ensure commandline is terminated */
	hdr.cmdline[BOOT_ARGS_SIZE-1] = 0;

	kernel_actual = ROUND_TO_PAGE(hdr.kernel_size);
	ramdisk_actual = ROUND_TO_PAGE(hdr.ramdisk_size);

	if (2048 + kernel_actual + ramdisk_actual < sz) {
		fastboot_fail("incomplete bootimage");
		return;
	}

	memmove((void*) KERNEL_ADDR, ptr + 2048, hdr.kernel_size);
	memmove((void*) RAMDISK_ADDR, ptr + 2048 + kernel_actual, hdr.ramdisk_size);

	fastboot_okay("");
	udc_stop();


	boot_linux((void*) KERNEL_ADDR, (void*) TAGS_ADDR,
		   (const char*) hdr.cmdline, 1008000,
		   (void*) RAMDISK_ADDR, hdr.ramdisk_size);
}

void aboot_init(const struct app_descriptor *app)
{
	udc_init(&surf_udc_device);

	fastboot_register("boot", cmd_boot);
	fastboot_publish("product", "swordfish");
	fastboot_publish("kernel", "lk");

	fastboot_init((void*) 0x10100000, 100 * 1024 * 1024);
	udc_start();
}

APP_START(aboot)
	.init = aboot_init,
APP_END

