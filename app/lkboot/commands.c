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

#include <lib/ptable.h>
#include <lib/bio.h>
#include <lib/sysparam.h>

#if PLATFORM_ZYNQ
#include <platform/fpga.h>
#endif

#define bootdevice "spi0"

typedef struct LKB lkb_t;

// returns 0 on success, -1 on failure (short read or io error)
int lkb_read(lkb_t *lkb, void *data, size_t len);
int lkb_write(lkb_t *lkb, const void *data, size_t len);

extern void *lkb_iobuffer;
extern paddr_t lkb_iobuffer_phys;
extern size_t lkb_iobuffer_size;

// return NULL for success, error string for failure
const char *lkb_handle_command(lkb_t *lkb, const char *cmd, const char *arg, unsigned len) {
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
	} else if (!strcmp(cmd, "getsysparam")) {
		const void *ptr;
		size_t len;
		if (sysparam_get_ptr(arg, &ptr, &len) == 0) {
			lkb_write(lkb, ptr, len);
		}
		return NULL;	
	} else {
		return "unknown command";
	}
}

// vim: noexpandtab
