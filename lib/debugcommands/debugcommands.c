/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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

#include <ctype.h>
#include <debug.h>
#include <stdlib.h>
#include <printf.h>
#include <list.h>
#include <string.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>

#include <lib/console.h>

static int cmd_display_mem(int argc, const cmd_args *argv);
static int cmd_modify_mem(int argc, const cmd_args *argv);
static int cmd_fill_mem(int argc, const cmd_args *argv);
static int cmd_reset(int argc, const cmd_args *argv);
static int cmd_memtest(int argc, const cmd_args *argv);
static int cmd_copy_mem(int argc, const cmd_args *argv);

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 0
	{ "dw", "display memory in words", &cmd_display_mem },
	{ "dh", "display memory in halfwords", &cmd_display_mem },
	{ "db", "display memory in bytes", &cmd_display_mem },
	{ "mw", "modify word of memory", &cmd_modify_mem },
	{ "mh", "modify halfword of memory", &cmd_modify_mem },
	{ "mb", "modify byte of memory", &cmd_modify_mem },
	{ "fw", "fill range of memory by word", &cmd_fill_mem },
	{ "fh", "fill range of memory by halfword", &cmd_fill_mem },
	{ "fb", "fill range of memory by byte", &cmd_fill_mem },
	{ "mc", "copy a range of memory", &cmd_copy_mem },
#endif
#if LK_DEBUGLEVEL > 1
	{ "mtest", "simple memory test", &cmd_memtest },
#endif
STATIC_COMMAND_END(mem);

static int cmd_display_mem(int argc, const cmd_args *argv)
{
	int size;

	if (argc < 3) {
		printf("not enough arguments\n");
		printf("%s <address> <length>\n", argv[0].str);
		return -1;
	}

	if (strcmp(argv[0].str, "dw") == 0) {
		size = 4;
	} else if (strcmp(argv[0].str, "dh") == 0) {
		size = 2;
	} else {
		size = 1;
	}

	unsigned long address = argv[1].u;
	size_t len = argv[2].u;
	unsigned long stop = address + len;
	int count = 0;

	if ((address & (size - 1)) != 0) {
		printf("unaligned address, cannot display\n");
		return -1;
	}

	for ( ; address < stop; address += size) {
		if (count == 0)
			printf("0x%08lx: ", address);
		switch (size) {
			case 4:
				printf("%08x ", *(uint32_t *)address);
				break;
			case 2:
				printf("%04hx ", *(uint16_t *)address);
				break;
			case 1:
				printf("%02hhx ", *(uint8_t *)address);
				break;
		}
		count += size;
		if (count == 16) {
			printf("\n");
			count = 0;
		}
	}

	if (count != 0)
		printf("\n");

	return 0;
}

static int cmd_modify_mem(int argc, const cmd_args *argv)
{
	int size;

	if (argc < 3) {
		printf("not enough arguments\n");
		printf("%s <address> <val>\n", argv[0].str);
		return -1;
	}

	if (strcmp(argv[0].str, "mw") == 0) {
		size = 4;
	} else if (strcmp(argv[0].str, "mh") == 0) {
		size = 2;
	} else {
		size = 1;
	}

	unsigned long address = argv[1].u;
	unsigned int val = argv[2].u;

	if ((address & (size - 1)) != 0) {
		printf("unaligned address, cannot modify\n");
		return -1;
	}

	switch (size) {
		case 4:
			*(uint32_t *)address = (uint32_t)val;
			break;
		case 2:
			*(uint16_t *)address = (uint16_t)val;
			break;
		case 1:
			*(uint8_t *)address = (uint8_t)val;
			break;
	}

	return 0;
}

static int cmd_fill_mem(int argc, const cmd_args *argv)
{
	int size;

	if (argc < 4) {
		printf("not enough arguments\n");
		printf("%s <address> <len> <val>\n", argv[0].str);
		return -1;
	}

	if (strcmp(argv[0].str, "fw") == 0) {
		size = 4;
	} else if (strcmp(argv[0].str, "fh") == 0) {
		size = 2;
	} else {
		size = 1;
	}

	unsigned long address = argv[1].u;
	unsigned long len = argv[2].u;
	unsigned long stop = address + len;
	unsigned int val = argv[3].u;

	if ((address & (size - 1)) != 0) {
		printf("unaligned address, cannot modify\n");
		return -1;
	}

	for ( ; address < stop; address += size) {
		switch (size) {
			case 4:
				*(uint32_t *)address = (uint32_t)val;
				break;
			case 2:
				*(uint16_t *)address = (uint16_t)val;
				break;
			case 1:
				*(uint8_t *)address = (uint8_t)val;
				break;
		}
	}

	return 0;
}

static int cmd_copy_mem(int argc, const cmd_args *argv)
{
	if (argc < 4) {
		printf("not enough arguments\n");
		printf("%s <source address> <target address> <len>\n", argv[0].str);
		return -1;
	}

	addr_t source = argv[1].u;
	addr_t target = argv[2].u;
	size_t len = argv[3].u;

	memcpy((void *)target, (const void *)source, len);

	return 0;
}

static int cmd_memtest(int argc, const cmd_args *argv)
{
	if (argc < 3) {
		printf("not enough arguments\n");
		printf("%s <base> <len>\n", argv[0].str);
		return -1;
	}

	uint32_t *ptr;
	size_t len;

	ptr = (uint32_t *)argv[1].u;
	len = (size_t)argv[2].u;

	size_t i;
	// write out
	printf("writing first pass...");
	for (i = 0; i < len / 4; i++) {
		ptr[i] = i;
	}
	printf("done\n");

	// verify
	printf("verifying...");
	for (i = 0; i < len / 4; i++) {
		if (ptr[i] != i)
			printf("error at %p\n", &ptr[i]);
	}
	printf("done\n");

	return 0;
}

