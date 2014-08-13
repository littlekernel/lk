/*
 * Copyright (c) 2014 Brian Swetland
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


#include <platform.h>
#include <stdio.h>
#include <debug.h>
#include <string.h>
#include <endian.h>
#include <malloc.h>
#include <arch.h>

#include <kernel/thread.h>

#include <lib/ptable.h>
#include <lib/bio.h>
#include <lib/sysparam.h>

#include <app/lkboot.h>

#if PLATFORM_ZYNQ
#include <platform/fpga.h>
#endif

#define bootdevice "spi0"

extern void *lkb_iobuffer;
extern paddr_t lkb_iobuffer_phys;
extern size_t lkb_iobuffer_size;

struct lkb_command {
	struct lkb_command *next;
	const char *name;
	lkb_handler_t handler;
	void *cookie;
};

struct lkb_command *lkb_cmd_list = NULL;

void lkb_register(const char *name, lkb_handler_t handler, void *cookie) {
	struct lkb_command *cmd = malloc(sizeof(struct lkb_command));
	if (cmd != NULL) {
		cmd->next = lkb_cmd_list;
		cmd->name = name;
		cmd->handler = handler;
		cmd->cookie = cookie;
		lkb_cmd_list = cmd;
	}
}

static int do_reboot(void *arg) {
	thread_sleep(250);
	platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RESET);
	return 0;
}

static int do_ramboot(void *arg) {
	thread_sleep(250);
	arch_chain_load(lkb_iobuffer);
	return 0;
}

// return NULL for success, error string for failure
const char *lkb_handle_command(lkb_t *lkb, const char *cmd, const char *arg, unsigned len) {
	struct lkb_command *lcmd;
	for (lcmd = lkb_cmd_list; lcmd; lcmd = lcmd->next) {
		if (!strcmp(lcmd->name, cmd)) {
			return lcmd->handler(lkb, arg, len, lcmd->cookie);
		}
	}

	if (len > lkb_iobuffer_size) {
		return "buffer too small";
	}
	if (!strcmp(cmd, "flash") || !strcmp(cmd, "erase")) {
		struct ptable_entry entry;
		bdev_t *bdev;
		if (ptable_find(arg, &entry) < 0) {
			return "no such partition";
		}
		if (len > entry.length) {
			return "partition too small";
		}
		if (lkb_read(lkb, lkb_iobuffer, len)) {
			return "io error";
		}
		if (!(bdev = bio_open(bootdevice))) {
			return "bio_open failed";
		}
		if (bio_erase(bdev, entry.offset, entry.length) != (ssize_t)entry.length) {
			bio_close(bdev);
			return "bio_erase failed";
		}
		if (!strcmp(cmd, "flash")) {
			if (bio_write(bdev, lkb_iobuffer, entry.offset, len) != (ssize_t)len) {
				bio_close(bdev);
				return "bio_write failed";
			}
		}
		bio_close(bdev);
		return NULL;
	} else if (!strcmp(cmd, "fpga")) {
#if PLATFORM_ZYNQ
		unsigned *x = lkb_iobuffer;
		if (lkb_read(lkb, lkb_iobuffer, len)) {
			return "io error";
		}
		for (unsigned n = 0; n < len; n+= 4) {
			*x = SWAP_32(*x);
			x++;
		}
		zynq_reset_fpga();
		zynq_program_fpga(lkb_iobuffer_phys, len);
		return NULL;
#else
		return "no fpga";
#endif
	} else if (!strcmp(cmd, "boot")) {
		if (lkb_read(lkb, lkb_iobuffer, len)) {
			return "io error";
		}
		thread_resume(thread_create("ramboot", &do_ramboot, NULL,
			DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
		return NULL;
	} else if (!strcmp(cmd, "getsysparam")) {
		const void *ptr;
		size_t len;
		if (sysparam_get_ptr(arg, &ptr, &len) == 0) {
			lkb_write(lkb, ptr, len);
		}
		return NULL;	
	} else if (!strcmp(cmd, "reboot")) {
		thread_resume(thread_create("reboot", &do_reboot, NULL,
			DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
		return NULL;
	} else {
		return "unknown command";
	}
}

// vim: noexpandtab
