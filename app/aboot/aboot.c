/*
 * Copyright (c) 2009, Google Inc.
 * Copyright (c) 2014, Xiaomi Inc.
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
#include <dev/udc.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/vm.h>

#include <dev/flash.h>
#include <lib/ptable.h>
#include <dev/keys.h>

#include "bootimg.h"
#include "fastboot.h"

#define DEFAULT_CMDLINE "mem=50M console=null";

static struct udc_device surf_udc_device = {
	.vendor_id  = 0x18d1,
	.product_id = 0x0001,
	.version_id = 0x0100,
	.manufacturer   = "Google",
	.product    = "Android",
};

struct atag_ptbl_entry {
	char name[16];
	unsigned offset;
	unsigned size;
	unsigned flags;
};

static void ptentry_to_tag(unsigned **ptr, struct ptable_entry *ptn)
{
	struct atag_ptbl_entry atag_ptn;

	memcpy(atag_ptn.name, ptn->name, 16);
	atag_ptn.name[15] = '\0';
	atag_ptn.offset = ptn->offset;
	atag_ptn.size = ptn->length;
	atag_ptn.flags = ptn->flags;
	memcpy(*ptr, &atag_ptn, sizeof(struct atag_ptbl_entry));
	*ptr += sizeof(struct atag_ptbl_entry) / sizeof(unsigned);
}

void boot_linux(void *kernel, unsigned *tags,
                const char *cmdline, unsigned machtype,
                void *ramdisk, unsigned ramdisk_size)
{
	unsigned *ptr = tags;
	int count;

	/* CORE */
	*ptr++ = 2;
	*ptr++ = 0x54410001;

	if (ramdisk_size) {
		*ptr++ = 4;
		*ptr++ = 0x54420005;
		*ptr++ = vaddr_to_paddr(ramdisk);
		*ptr++ = ramdisk_size;
	}

	count = ptable_get_count();
	if (count != 0) {
		int i;
		*ptr++ = 2 + (count * (sizeof(struct atag_ptbl_entry) /
		                               sizeof(unsigned)));
		*ptr++ = 0x4d534d70;
		for (i = 0; i < count; ++i) {
			struct ptable_entry entry;
			bzero(&entry, sizeof(entry));
			ptable_get(i, &entry);
			ptentry_to_tag(&ptr, &entry);
		}
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

	chain_load3(kernel, 0, machtype, vaddr_to_paddr(tags));
}

#define PAGE_MASK 2047

#define ROUND_TO_PAGE(x) (((x) + PAGE_MASK) & (~PAGE_MASK))

static unsigned char buf[2048];

int boot_linux_from_flash(void)
{
	struct boot_img_hdr *hdr = (void*) buf;
	unsigned n;
	struct ptable_entry entry;
	struct ptable_entry *ptn = &entry;
	unsigned offset = 0;
	const char *cmdline;
	void *kernel;
	void *ramdisk;

	if (ptable_find("boot", ptn) != 0) {
		dprintf(CRITICAL, "ERROR: No boot partition found\n");
		return -1;
	}

	if (flash_read(ptn, offset, buf, 2048)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
		return -1;
	}
	offset += 2048;

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(CRITICAL, "ERROR: Invaled boot image heador\n");
		return -1;
	}

	n = ROUND_TO_PAGE(hdr->kernel_size);
	kernel = paddr_to_kvaddr(hdr->kernel_addr);
	if (flash_read(ptn, offset, kernel, n)) {
		dprintf(CRITICAL, "ERROR: Cannot read kernel image\n");
		return -1;
	}
	offset += n;

	n = ROUND_TO_PAGE(hdr->ramdisk_size);
	ramdisk = paddr_to_kvaddr(hdr->ramdisk_addr);
	if (flash_read(ptn, offset, ramdisk, n)) {
		dprintf(CRITICAL, "ERROR: Cannot read ramdisk image\n");
		return -1;
	}
	offset += n;

	dprintf(INFO, "\nkernel  @ %x (%d bytes)\n", hdr->kernel_addr,
	        hdr->kernel_size);
	dprintf(INFO, "ramdisk @ %x (%d bytes)\n", hdr->ramdisk_addr,
	        hdr->ramdisk_size);

	if (hdr->cmdline[0]) {
		cmdline = (char*) hdr->cmdline;
	} else {
		cmdline = DEFAULT_CMDLINE;
	}
	dprintf(INFO, "cmdline = '%s'\n", cmdline);

	/* TODO: create/pass atags to kernel */

	dprintf(INFO, "\nBooting Linux\n");
	boot_linux(kernel, paddr_to_kvaddr(hdr->tags_addr),
	           (const char *)cmdline, LINUX_MACHTYPE,
	           ramdisk, hdr->ramdisk_size);

	return 0;
}

void cmd_boot(const char *arg, void *data, unsigned sz)
{
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	static struct boot_img_hdr hdr;
	char *ptr = ((char*) data);
	void *kernel;
	void *ramdisk;

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

	kernel = paddr_to_kvaddr(hdr.kernel_addr);
	memmove(kernel, ptr + 2048, hdr.kernel_size);

	ramdisk = paddr_to_kvaddr(hdr.ramdisk_addr);
	memmove(ramdisk, ptr + 2048 + kernel_actual, hdr.ramdisk_size);

	fastboot_okay("");
	udc_stop();


	boot_linux(kernel, paddr_to_kvaddr(hdr.tags_addr),
	           (const char*) hdr.cmdline, LINUX_MACHTYPE,
	           ramdisk, hdr.ramdisk_size);
}

void cmd_erase(const char *arg, void *data, unsigned sz)
{
	struct ptable_entry entry;
	struct ptable_entry *ptn = &entry;

	if (ptable_find(arg, ptn) != 0) {
		fastboot_fail("unknown partition name");
		return;
	}

	if (flash_erase(ptn)) {
		fastboot_fail("failed to erase partition");
		return;
	}
	fastboot_okay("");
}

void cmd_flash(const char *arg, void *data, unsigned sz)
{
	struct ptable_entry entry;
	struct ptable_entry *ptn = &entry;
	unsigned extra = 0;

	if (ptable_find(arg, ptn) != 0) {
		fastboot_fail("unknown partition name");
		return;
	}

	if (!strcmp((char *)ptn->name, "boot") || !strcmp((char *)ptn->name, "recovery")) {
		if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
			fastboot_fail("image is not a boot image");
			return;
		}
	}

	if (!strcmp((char *)ptn->name, "system") || !strcmp((char *)ptn->name, "userdata"))
		extra = 64;
	else
		sz = ROUND_TO_PAGE(sz);

	dprintf(INFO, "writing %d bytes to '%s'\n", sz, ptn->name);
	if (flash_write(ptn, extra, data, sz)) {
		fastboot_fail("flash write failure");
		return;
	}
	dprintf(INFO, "partition '%s' updated\n", ptn->name);
	fastboot_okay("");
}

void cmd_continue(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	udc_stop();

	boot_linux_from_flash();
}

void aboot_init(const struct app_descriptor *app)
{
	if (keys_get_state(KEY_BACK) != 0)
		goto fastboot;

	boot_linux_from_flash();
	dprintf(CRITICAL, "ERROR: Could not do normal boot. Reverting "
	        "to fastboot mode.\n");

fastboot:
	udc_init(&surf_udc_device);

	fastboot_register("boot", cmd_boot);
	fastboot_register("erase:", cmd_erase);
	fastboot_register("flash:", cmd_flash);
	fastboot_register("continue", cmd_continue);
	fastboot_publish("product", "swordfish");
	fastboot_publish("kernel", "lk");

	fastboot_init((void*) 0x10100000, 100 * 1024 * 1024);
	udc_start();
}

APP_START(aboot)
	.init = aboot_init,
APP_END

