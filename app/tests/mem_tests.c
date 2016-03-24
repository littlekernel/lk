/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <arch.h>
#include <arch/ops.h>
#include <lib/console.h>
#include <platform.h>
#include <debug.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

static void mem_test_fail(void *ptr, uint32_t should, uint32_t is)
{
    printf("ERROR at %p: should be 0x%x, is 0x%x\n", ptr, should, is);

    ptr = (void *)ROUNDDOWN((uintptr_t)ptr, 64);
    hexdump(ptr, 128);
}

static status_t do_pattern_test(void *ptr, size_t len, uint32_t pat)
{
    volatile uint32_t *vbuf32 = ptr;
    size_t i;

    printf("\tpattern 0x%08x\n", pat);
    for (i = 0; i < len / 4; i++) {
        vbuf32[i] = pat;
    }

    for (i = 0; i < len / 4; i++) {
        if (vbuf32[i] != pat) {
            mem_test_fail((void *)&vbuf32[i], pat, vbuf32[i]);
            return ERR_GENERIC;
        }
    }

    return NO_ERROR;
}

static status_t do_moving_inversion_test(void *ptr, size_t len, uint32_t pat)
{
    volatile uint32_t *vbuf32 = ptr;
    size_t i;

    printf("\tpattern 0x%08x\n", pat);

    /* fill memory */
    for (i = 0; i < len / 4; i++) {
        vbuf32[i] = pat;
    }

    /* from the bottom, walk through each cell, inverting the value */
    //printf("\t\tbottom up invert\n");
    for (i = 0; i < len / 4; i++) {
        if (vbuf32[i] != pat) {
            mem_test_fail((void *)&vbuf32[i], pat, vbuf32[i]);
            return ERR_GENERIC;
        }

        vbuf32[i] = ~pat;
    }

    /* repeat, walking from top down */
    //printf("\t\ttop down invert\n");
    for (i = len / 4; i > 0; i--) {
        if (vbuf32[i-1] != ~pat) {
            mem_test_fail((void *)&vbuf32[i-1], ~pat, vbuf32[i-1]);
            return ERR_GENERIC;
        }

        vbuf32[i-1] = pat;
    }

    /* verify that we have the original pattern */
    //printf("\t\tfinal test\n");
    for (i = 0; i < len / 4; i++) {
        if (vbuf32[i] != pat) {
            mem_test_fail((void *)&vbuf32[i], pat, vbuf32[i]);
            return ERR_GENERIC;
        }
    }

    return NO_ERROR;
}

static void do_mem_tests(void *ptr, size_t len)
{
    size_t i;

    /* test 1: simple write address to memory, read back */
    printf("test 1: simple address write, read back\n");
    volatile uint32_t *vbuf32 = ptr;
    for (i = 0; i < len / 4; i++) {
        vbuf32[i] = i;
    }

    for (i = 0; i < len / 4; i++) {
        if (vbuf32[i] != i) {
            mem_test_fail((void *)&vbuf32[i], i, vbuf32[i]);
            goto out;
        }
    }

    /* test 2: write various patterns, read back */
    printf("test 2: write patterns, read back\n");

    static const uint32_t pat[] = {
        0x0, 0xffffffff,
        0xaaaaaaaa, 0x55555555,
    };

    for (size_t p = 0; p < countof(pat); p++) {
        if (do_pattern_test(ptr, len, pat[p]) < 0)
            goto out;
    }
    // shift bits through 32bit word
    for (uint32_t p = 1; p != 0; p <<= 1) {
        if (do_pattern_test(ptr, len, p) < 0)
            goto out;
    }
    // shift bits through 16bit word, invert top of 32bit
    for (uint16_t p = 1; p != 0; p <<= 1) {
        if (do_pattern_test(ptr, len, ((~p) << 16) | p) < 0)
            goto out;
    }

    /* test 3: moving inversion, patterns */
    printf("test 3: moving inversions with patterns\n");
    for (size_t p = 0; p < countof(pat); p++) {
        if (do_moving_inversion_test(ptr, len, pat[p]) < 0)
            goto out;

    }
    // shift bits through 32bit word
    for (uint32_t p = 1; p != 0; p <<= 1) {
        if (do_moving_inversion_test(ptr, len, p) < 0)
            goto out;
    }
    // shift bits through 16bit word, invert top of 32bit
    for (uint16_t p = 1; p != 0; p <<= 1) {
        if (do_moving_inversion_test(ptr, len, ((~p) << 16) | p) < 0)
            goto out;
    }

out:
    printf("done with tests\n");
}

static int mem_test(int argc, const cmd_args *argv)
{
    if (argc < 2) {
        printf("not enough arguments\n");
usage:
        printf("usage: %s <length>\n", argv[0].str);
        printf("usage: %s <base> <length>\n", argv[0].str);
        return -1;
    }

    if (argc == 2) {
        void *ptr;
        size_t len = argv[1].u;

#if WITH_KERNEL_VM
        /* rounding up len to the next page */
        len = PAGE_ALIGN(len);
        if (len == 0) {
            printf("invalid length\n");
            return -1;
        }

        /* allocate a region to test in */
        status_t err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), "memtest", len, &ptr, 0, 0, ARCH_MMU_FLAG_UNCACHED);
        if (err < 0) {
            printf("error %d allocating test region\n", err);
            return -1;
        }

        paddr_t pa;
        pa = vaddr_to_paddr(ptr);
        printf("physical address 0x%lx\n", pa);
#else
        /* allocate from the heap */
        ptr = malloc(len);
        if (!ptr ) {
            printf("error allocating test area from heap\n");
            return -1;
        }

#endif

        printf("got buffer at %p of length 0x%lx\n", ptr, len);

        /* run the tests */
        do_mem_tests(ptr, len);

#if WITH_KERNEL_VM
        // XXX free memory region here
        printf("NOTE: leaked memory\n");
#else
        free(ptr);
#endif
    } else if (argc == 3) {
        void *ptr = argv[1].p;
        size_t len = argv[2].u;

        /* run the tests */
        do_mem_tests(ptr, len);
    } else {
        goto usage;
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("mem_test", "test memory", &mem_test)
STATIC_COMMAND_END(mem_tests);
