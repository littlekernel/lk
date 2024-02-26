/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <ctype.h>
#include <lk/debug.h>
#include <stdlib.h>
#include <stdio.h>
#include <lk/list.h>
#include <string.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>
#include <arch.h>

#include <lk/console_cmd.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

static int cmd_display_mem(int argc, const console_cmd_args *argv);
static int cmd_modify_mem(int argc, const console_cmd_args *argv);
static int cmd_fill_mem(int argc, const console_cmd_args *argv);
static int cmd_reset(int argc, const console_cmd_args *argv);
static int cmd_memtest(int argc, const console_cmd_args *argv);
static int cmd_copy_mem(int argc, const console_cmd_args *argv);
static int cmd_chain(int argc, const console_cmd_args *argv);
static int cmd_sleep(int argc, const console_cmd_args *argv);
static int cmd_time(int argc, const console_cmd_args *argv);
static int cmd_timeh(int argc, const console_cmd_args *argv);
static int cmd_crash(int argc, const console_cmd_args *argv);
static int cmd_panic(int argc, const console_cmd_args *argv);
static int cmd_stackstomp(int argc, const console_cmd_args *argv);

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
STATIC_COMMAND("panic", "intentionally panic", &cmd_panic)
STATIC_COMMAND("stackstomp", "intentionally overrun the stack", &cmd_stackstomp)
#endif
#if LK_DEBUGLEVEL > 1
STATIC_COMMAND("mtest", "simple memory test", &cmd_memtest)
#endif
STATIC_COMMAND("chain", "chain load another binary", &cmd_chain)
STATIC_COMMAND("sleep", "sleep number of seconds", &cmd_sleep)
STATIC_COMMAND("sleepm", "sleep number of milliseconds", &cmd_sleep)
STATIC_COMMAND("time", "print current time", &cmd_time)
STATIC_COMMAND("timeh", "print current time hires", &cmd_timeh)
STATIC_COMMAND_END(mem);

#define EXIT_IF_NOT_MAPPED(address) \
    do { \
      if (vaddr_to_paddr((void *)address) == 0) { \
        printf("ERROR: address 0x%lx is unmapped\n", address); \
        return -1; \
      } \
    } while (0)

#if WITH_KERNEL_VM
/*
 * Checks if the address requested is mapped. Mapping is checked at page level,
 * in the address range make sure all the pages are mapped.
 * Adjust stop address to last mapped address in the range.
 */
static int check_address_mapped(unsigned long start, unsigned long *stop) {
    unsigned long page_curr;
    if (stop == NULL) {
      return -1;
    }
    page_curr = ROUNDDOWN(start, PAGE_SIZE);

    /* if the first page itself unmapped, return error */
    EXIT_IF_NOT_MAPPED(page_curr);

    /* check rest of the pages */
    page_curr += PAGE_SIZE;
    while (page_curr < *stop) {
      if (vaddr_to_paddr((void *)page_curr) == 0) {
        /* there is an unmpaeed page in the range, adjust stop */
        *stop = page_curr; /* don't access beyond this */
        printf("INFO: access truncated to 0x%lx as no further mapping\n", *stop);
        /* this ain't an error, the requested len is not mapped */
        return 0;
      }
      page_curr += PAGE_SIZE;
    }
    return 0;
}
#endif

static int cmd_display_mem(int argc, const console_cmd_args *argv) {
    /* save the last address and len so we can continue where we left off */
    static unsigned long address;
    static size_t len;

    if (argc < 3 && len == 0) {
        printf("not enough arguments\n");
#if WITH_KERNEL_VM
        printf("%s [-l] [-b] [-p] [address] [length]\n", argv[0].str);
#else
        printf("%s [-l] [-b] [address] [length]\n", argv[0].str);
#endif
        printf("  -l  little endian\n"
               "  -b  big endian\n");
#if WITH_KERNEL_VM
        printf("  -p  physical address\n");
#endif
        return -1;
    }

    uint32_t size;
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
#if WITH_KERNEL_VM
    bool phy_addr = false;
#endif
    while (argc > argindex) {
        if (!strcmp(argv[argindex].str, "-l")) {
            byte_order = LITTLE_ENDIAN;
        } else if (!strcmp(argv[argindex].str, "-b")) {
            byte_order = BIG_ENDIAN;
#if WITH_KERNEL_VM
        } else if (!strcmp(argv[argindex].str, "-p")) {
          phy_addr = true;
#endif
        } else if (!read_address) {
            address = argv[argindex].u;
            read_address = true;
        } else {
            len = argv[argindex].u;
        }

        argindex++;
    }

#if WITH_KERNEL_VM
    if (phy_addr == true) {
        address = (unsigned long)paddr_to_kvaddr(address);
    }
#endif
    unsigned long stop = address + len;
    int count = 0;

    if ((address & (size - 1)) != 0) {
        printf("unaligned address, cannot display\n");
        return -1;
    }

#if WITH_KERNEL_VM
    if (check_address_mapped(address, &stop))
      return -1;
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

static int cmd_modify_mem(int argc, const console_cmd_args *argv) {
    uint32_t size;
    unsigned long address = 0;
    unsigned int val = 0;

    if (argc < 3) {
        printf("not enough arguments\n");
#if WITH_KERNEL_VM
        printf("%s [-p] <address> <val>\n"
               "  -p  physical address\n", argv[0].str);
#else
        printf("%s <address> <val>\n", argv[0].str);
#endif
        return -1;
    }

    if (strcmp(argv[0].str, "mw") == 0) {
        size = 4;
    } else if (strcmp(argv[0].str, "mh") == 0) {
        size = 2;
    } else {
        size = 1;
    }

    int argindex = 1;
    bool read_address = false;
#if WITH_KERNEL_VM
    bool phy_addr = false;
#endif

    while (argc > argindex) {
#if WITH_KERNEL_VM
        if (!strcmp(argv[argindex].str, "-p")) {
            phy_addr = true;
            argindex++;
            continue;
        }
#endif
        if (!read_address) {
            address = argv[argindex].u;
            read_address = true;
        } else {
            val = argv[argindex].u;
        }

        argindex++;
    }

#if WITH_KERNEL_VM
    if (phy_addr == true) {
        address = (unsigned long)paddr_to_kvaddr(address);
    }
#endif
    if ((address & (size - 1)) != 0) {
        printf("unaligned address, cannot modify\n");
        return -1;
    }

#if WITH_KERNEL_VM
    /* preflight the page start address to see if it's mapped */
    EXIT_IF_NOT_MAPPED(ROUNDDOWN(address, PAGE_SIZE));
#endif

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

static int cmd_fill_mem(int argc, const console_cmd_args *argv) {
    uint32_t size;

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

#if WITH_KERNEL_VM
    if (check_address_mapped(address, &stop))
      return -1;
#endif

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

static int cmd_copy_mem(int argc, const console_cmd_args *argv) {
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

static int cmd_memtest(int argc, const console_cmd_args *argv) {
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

static int cmd_chain(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
        printf("not enough arguments\n");
        printf("%s <address>\n", argv[0].str);
        return -1;
    }

    arch_chain_load(argv[1].p, 0, 0, 0, 0);

    return 0;
}

static int cmd_sleep(int argc, const console_cmd_args *argv) {
    lk_time_t t = 1000; /* default to 1 second */

    if (argc >= 2) {
        t = argv[1].u;
        if (!strcmp(argv[0].str, "sleep"))
            t *= 1000;
    }

    thread_sleep(t);

    return 0;
}

static int cmd_time(int argc, const console_cmd_args *argv) {

    lk_time_t t = current_time();
    printf("Current time: %u\n", t);

    return 0;
}

static int cmd_timeh(int argc, const console_cmd_args *argv) {

    lk_bigtime_t t = current_time_hires();
    printf("Current time hires: %llu\n", t);

    return 0;
}

/* fix warning for the near-null pointer dereference below with gcc 12.x+ */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
static int cmd_crash(int argc, const console_cmd_args *argv) {
#if ARCH_ARM && ARM_ONLY_THUMB
    /* a branch directly to an aligned address should trigger a fault */
    asm("bx %0":: "r"(0));
#else
    /* should crash */
    volatile uint32_t *ptr = (void *)1;
    *ptr = 1;
#endif

    /* if it didn't, panic the system */
    panic("crash");

    return 0;
}
#pragma GCC diagnostic pop

static int cmd_panic(int argc, const console_cmd_args *argv) {
    panic("Test panic\n");
    return 0;
}

static int cmd_stackstomp(int argc, const console_cmd_args *argv) {
    for (size_t i = 0; i < DEFAULT_STACK_SIZE * 2; i++) {
        uint8_t death[i];

        memset(death, 0xaa, i);
        thread_sleep(1);
    }

    printf("survived.\n");

    return 0;
}


