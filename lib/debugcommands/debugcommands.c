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
#include <stdio.h>
#include <list.h>
#include <string.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>
#include <arch.h>

#include <lib/console.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

static int cmd_display_mem(int argc, const cmd_args *argv);
static int cmd_modify_mem(int argc, const cmd_args *argv);
static int cmd_fill_mem(int argc, const cmd_args *argv);
static int cmd_reset(int argc, const cmd_args *argv);
static int cmd_memtest(int argc, const cmd_args *argv);
static int cmd_copy_mem(int argc, const cmd_args *argv);
static int cmd_chain(int argc, const cmd_args *argv);
static int cmd_sleep(int argc, const cmd_args *argv);
static int cmd_crash(int argc, const cmd_args *argv);
static int cmd_stackstomp(int argc, const cmd_args *argv);

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 0
STATIC_COMMAND_MASKED("dw", "display memory in words", &cmd_display_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("dh", "display memory in halfwords", &cmd_display_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("db", "display memory in bytes", &cmd_display_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("mw", "modify word of memory", &cmd_modify_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("mh", "modify halfword of memory", &cmd_modify_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("mb", "modify byte of memory", &cmd_modify_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("fw", "fill range of memory by word", &cmd_fill_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("fh", "fill range of memory by halfword", &cmd_fill_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("fb", "fill range of memory by byte", &cmd_fill_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_MASKED("mc", "copy a range of memory", &cmd_copy_mem, CMD_AVAIL_ALWAYS)
STATIC_COMMAND("crash", "intentionally crash", &cmd_crash)
STATIC_COMMAND("stackstomp", "intentionally overrun the stack", &cmd_stackstomp)
#endif
#if LK_DEBUGLEVEL > 1
STATIC_COMMAND("mtest", "simple memory test", &cmd_memtest)
#endif
STATIC_COMMAND("chain", "chain load another binary", &cmd_chain)
STATIC_COMMAND("sleep", "sleep number of seconds", &cmd_sleep)
STATIC_COMMAND("sleepm", "sleep number of milliseconds", &cmd_sleep)
STATIC_COMMAND_END(mem);

static int cmd_display_mem(int argc, const cmd_args *argv)
{
    /* save the last address and len so we can continue where we left off */
    static unsigned long address;
    static size_t len;

    if (argc < 3 && len == 0) {
        printf("not enough arguments\n");
        printf("%s [-l] [-b] [address] [length]\n", argv[0].str);
        return -1;
    }

    int size;
    if (strcmp(argv[0].str, "dw") == 0) {
        size = 4;
    } else if (strcmp(argv[0].str, "dh") == 0) {
        size = 2;
    } else {
        size = 1;
    }

    uint byte_order = BYTE_ORDER;
    int argindex = 1;
    bool read_address = false;
    while (argc > argindex) {
        if (!strcmp(argv[argindex].str, "-l")) {
            byte_order = LITTLE_ENDIAN;
        } else if (!strcmp(argv[argindex].str, "-b")) {
            byte_order = BIG_ENDIAN;
        } else if (!read_address) {
            address = argv[argindex].u;
            read_address = true;
        } else {
            len = argv[argindex].u;
        }

        argindex++;
    }

    unsigned long stop = address + len;
    int count = 0;

    if ((address & (size - 1)) != 0) {
        printf("unaligned address, cannot display\n");
        return -1;
    }

#if WITH_KERNEL_VM
    /* preflight the start address to see if it's mapped */
    if (vaddr_to_paddr((void *)address) == 0) {
        printf("ERROR: address 0x%lx is unmapped\n", address);
        return -1;
    }
#endif

    for ( ; address < stop; address += size) {
        if (count == 0)
            printf("0x%08lx: ", address);
        switch (size) {
            case 4: {
                uint32_t val = (byte_order != BYTE_ORDER) ?
                               SWAP_32(*(uint32_t *)address) :
                               *(uint32_t *)address;
                printf("%08x ", val);
                break;
            }
            case 2: {
                uint16_t val = (byte_order != BYTE_ORDER) ?
                               SWAP_16(*(uint16_t *)address) :
                               *(uint16_t *)address;
                printf("%04hx ", val);
                break;
            }
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

static int cmd_chain(int argc, const cmd_args *argv)
{
    if (argc < 2) {
        printf("not enough arguments\n");
        printf("%s <address>\n", argv[0].str);
        return -1;
    }

    arch_chain_load(argv[1].p, 0, 0, 0, 0);

    return 0;
}

static int cmd_sleep(int argc, const cmd_args *argv)
{
    lk_time_t t = 1000; /* default to 1 second */

    if (argc >= 2) {
        t = argv[1].u;
        if (!strcmp(argv[0].str, "sleep"))
            t *= 1000;
    }

    thread_sleep(t);

    return 0;
}

static int cmd_crash(int argc, const cmd_args *argv)
{
    /* should crash */
    volatile uint32_t *ptr = (void *)1;
    *ptr = 1;

    /* if it didn't, panic the system */
    panic("crash");

    return 0;
}

static int cmd_stackstomp(int argc, const cmd_args *argv)
{
    for (size_t i = 0; i < DEFAULT_STACK_SIZE * 2; i++) {
        uint8_t death[i];

        memset(death, 0xaa, i);
        thread_sleep(1);
    }

    printf("survived.\n");

    return 0;
}


