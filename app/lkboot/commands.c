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
#include <stdlib.h>
#include <debug.h>
#include <string.h>
#include <endian.h>
#include <malloc.h>
#include <arch.h>
#include <err.h>
#include <trace.h>
#include <pow2.h>

#include <kernel/thread.h>
#include <kernel/vm.h>

#include <lib/bio.h>
#include <lib/bootargs.h>
#include <lib/bootimage.h>
#include <lib/ptable.h>
#include <lib/sysparam.h>

#include <app/lkboot.h>

#if PLATFORM_ZYNQ
#include <platform/fpga.h>
#include <platform/zynq.h>
#endif

#define bootdevice "spi0"

#define LOCAL_TRACE 0

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

struct chainload_args {
    void *func;
    ulong args[4];
};

static int chainload_thread(void *arg)
{
    struct chainload_args *args = (struct chainload_args *)arg;

    thread_sleep(250);

    TRACEF("chain loading address %p, args 0x%lx 0x%lx 0x%lx 0x%lx\n",
            args->func, args->args[0], args->args[1], args->args[2], args->args[3]);
    arch_chain_load((void *)args->func, args->args[0], args->args[1], args->args[2], args->args[3]);

    for (;;);
}

static int do_boot(lkb_t *lkb, size_t len, const char **result)
{
    LTRACEF("lkb %p, len %zu, result %p\n", lkb, len, result);

    void *buf;
    paddr_t buf_phys;

    if (vmm_alloc_contiguous(vmm_get_kernel_aspace(), "lkboot_iobuf",
        len, &buf, log2_uint(1024*1024), 0, ARCH_MMU_FLAG_UNCACHED) < 0) {
        *result = "not enough memory";
        return -1;
    }
    arch_mmu_query((vaddr_t)buf, &buf_phys, NULL);
    LTRACEF("iobuffer %p (phys 0x%lx)\n", buf, buf_phys);

    if (lkb_read(lkb, buf, len)) {
        *result = "io error";
        // XXX free buffer here
        return -1;
    }

    /* construct a boot argument list */
    const size_t bootargs_size = PAGE_SIZE;
#if 0
    void *args = (void *)((uintptr_t)lkb_iobuffer + lkb_iobuffer_size - bootargs_size);
    paddr_t args_phys = lkb_iobuffer_phys + lkb_iobuffer_size - bootargs_size;
#elif PLATFORM_ZYNQ
    /* grab the top page of sram */
    /* XXX do this better */
    paddr_t args_phys = SRAM_BASE + SRAM_SIZE - bootargs_size;
    void *args = paddr_to_kvaddr(args_phys);
#else
#error need better way
#endif
    LTRACEF("boot args %p, phys 0x%lx, len %zu\n", args, args_phys, bootargs_size);

    bootargs_start(args, bootargs_size);
    bootargs_add_command_line(args, bootargs_size, "what what");
    arch_clean_cache_range((vaddr_t)args, bootargs_size);

    ulong lk_args[4];
    bootargs_generate_lk_arg_values(args_phys, lk_args);

    const void *ptr;

    /* sniff it to see if it's a bootimage or a raw image */
    bootimage_t *bi;
    if (bootimage_open(buf, len, &bi) >= 0) {
        size_t len;

        /* it's a bootimage */
        TRACEF("detected bootimage\n");

        /* find the lk image */
        if (bootimage_get_file_section(bi, TYPE_LK, &ptr, &len) >= 0) {
            TRACEF("found lk section at %p\n", ptr);

            /* add the boot image to the argument list */
            size_t bootimage_size;
            bootimage_get_range(bi, NULL, &bootimage_size);

            bootargs_add_bootimage_pointer(args, bootargs_size, "pmem", buf_phys, bootimage_size);
        }
    } else {
        /* raw image, just chain load it directly */
        TRACEF("raw image, chainloading\n");

        ptr = buf;
    }

    /* start a boot thread to complete the startup */
    static struct chainload_args cl_args;

    cl_args.func = (void *)ptr;
    cl_args.args[0] = lk_args[0];
    cl_args.args[1] = lk_args[1];
    cl_args.args[2] = lk_args[2];
    cl_args.args[3] = lk_args[3];

    thread_resume(thread_create("boot", &chainload_thread, &cl_args,
        DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));

    return 0;
}

/* try to boot the system from a flash partition */
status_t do_flash_boot(void)
{
    status_t err;

    LTRACE_ENTRY;

    /* construct a boot argument list */
    const size_t bootargs_size = PAGE_SIZE;
#if 0
    /* old code */
    void *args = (void *)((uintptr_t)lkb_iobuffer + lkb_iobuffer_size - bootargs_size);
    paddr_t args_phys = lkb_iobuffer_phys + lkb_iobuffer_size - bootargs_size;
#elif PLATFORM_ZYNQ
    /* grab the top page of sram */
    paddr_t args_phys = SRAM_BASE + SRAM_SIZE - bootargs_size;
    void *args = paddr_to_kvaddr(args_phys);
#else
#error need better way
#endif
    LTRACEF("boot args %p, phys 0x%lx, len %zu\n", args, args_phys, bootargs_size);

    bootargs_start(args, bootargs_size);
    bootargs_add_command_line(args, bootargs_size, "what what");
    arch_clean_cache_range((vaddr_t)args, bootargs_size);

    ulong lk_args[4];
    bootargs_generate_lk_arg_values(args_phys, lk_args);

    const void *ptr;

    if (!ptable_found_valid()) {
        TRACEF("ptable not found\n");
        return ERR_NOT_FOUND;
    }

    /* find the system partition */
    struct ptable_entry entry;
    err = ptable_find("system", &entry);
    if (err < 0) {
        TRACEF("cannot find system partition\n");
        return ERR_NOT_FOUND;
    }

    /* get a direct pointer to the device */
    bdev_t *bdev = ptable_get_device();
    if (!bdev) {
        TRACEF("error opening boot device\n");
        return ERR_NOT_FOUND;
    }

    /* convert the bdev to a memory pointer */
    err = bio_ioctl(bdev, BIO_IOCTL_GET_MEM_MAP, (void *)&ptr);
    TRACEF("err %d, ptr %p\n", err, ptr);
    if (err < 0) {
        TRACEF("error getting direct pointer to block device\n");
        return ERR_NOT_FOUND;
    }

    /* sniff it to see if it's a bootimage or a raw image */
    bootimage_t *bi;
    if (bootimage_open((char *)ptr + entry.offset, entry.length, &bi) >= 0) {
        size_t len;

        /* it's a bootimage */
        TRACEF("detected bootimage\n");

        /* find the lk image */
        if (bootimage_get_file_section(bi, TYPE_LK, &ptr, &len) >= 0) {
            TRACEF("found lk section at %p\n", ptr);

            /* add the boot image to the argument list */
            size_t bootimage_size;
            bootimage_get_range(bi, NULL, &bootimage_size);

            bootargs_add_bootimage_pointer(args, bootargs_size, bdev->name, entry.offset, bootimage_size);
        }
    } else {
        /* did not find a bootimage, abort */
        bio_ioctl(bdev, BIO_IOCTL_PUT_MEM_MAP, NULL);
        return ERR_NOT_FOUND;
    }

    TRACEF("chain loading binary at %p\n", ptr);
    arch_chain_load((void *)ptr, lk_args[0], lk_args[1], lk_args[2], lk_args[3]);

    /* put the block device back into block mode (though we never get here) */
    bio_ioctl(bdev, BIO_IOCTL_PUT_MEM_MAP, NULL);

    return NO_ERROR;
}

// return NULL for success, error string for failure
int lkb_handle_command(lkb_t *lkb, const char *cmd, const char *arg, size_t len, const char **result)
{
    *result = NULL;

    struct lkb_command *lcmd;
    for (lcmd = lkb_cmd_list; lcmd; lcmd = lcmd->next) {
        if (!strcmp(lcmd->name, cmd)) {
            *result = lcmd->handler(lkb, arg, len, lcmd->cookie);
            return 0;
        }
    }

    if (!strcmp(cmd, "flash") || !strcmp(cmd, "erase")) {
        struct ptable_entry entry;
        bdev_t *bdev;

        if (ptable_find(arg, &entry) < 0) {
            size_t plen = len;
            /* doesn't exist, make one */
#if PLATFORM_ZYNQ
            /* XXX not really the right place, should be in the ptable/bio layer */
            plen = ROUNDUP(plen, 256*1024);
#endif
            off_t off = ptable_allocate(plen, 0);
            if (off < 0) {
                *result = "no space to allocate partition";
                return -1;
            }

            if (ptable_add(arg, off, plen, 0) < 0) {
                *result = "error creating partition";
                return -1;
            }

            if (ptable_find(arg, &entry) < 0) {
                *result = "couldn't find partition after creating it";
                return -1;
            }
        }
        if (len > entry.length) {
            *result = "partition too small";
            return -1;
        }

        if (!(bdev = ptable_get_device())) {
            *result = "ptable_get_device failed";
            return -1;
        }

        printf("lkboot: erasing partition of size %llu\n", entry.length);
        if (bio_erase(bdev, entry.offset, entry.length) != (ssize_t)entry.length) {
            *result = "bio_erase failed";
            return -1;
        }

        if (!strcmp(cmd, "flash")) {
            printf("lkboot: writing to partition\n");

            void *buf = malloc(bdev->block_size);
            if (!buf) {
                *result = "memory allocation failed";
                return -1;
            }

            size_t pos = 0;
            while (pos < len) {
                size_t toread = MIN(len - pos, bdev->block_size);

                LTRACEF("offset %zu, toread %zu\n", pos, toread);

                if (lkb_read(lkb, buf, toread)) {
                    *result = "io error";
                    free(buf);
                    return -1;
                }

                if (bio_write(bdev, buf, entry.offset + pos, toread) != (ssize_t)toread) {
                    *result = "bio_write failed";
                    free(buf);
                    return -1;
                }

                pos += toread;
            }

            free(buf);
        }
    } else if (!strcmp(cmd, "remove")) {
        if (ptable_remove(arg) < 0) {
            *result = "remove failed";
            return -1;
        }
    } else if (!strcmp(cmd, "fpga")) {
#if PLATFORM_ZYNQ
        void *buf = malloc(len);
        if (!buf) {
            *result = "error allocating buffer";
            return -1;
        }

        /* translate to physical address */
        paddr_t pa = kvaddr_to_paddr(buf);
        if (pa == 0) {
            *result = "error allocating buffer";
            free(buf);
            return -1;

        }

        if (lkb_read(lkb, buf, len)) {
            *result = "io error";
            free(buf);
            return -1;
        }

        /* make sure the cache is flushed for this buffer for DMA coherency purposes */
        arch_clean_cache_range((vaddr_t)buf, len);

        /* program the fpga */
        zynq_reset_fpga();
        zynq_program_fpga(pa, len);

        free(buf);
#else
        *result = "no fpga";
        return -1;
#endif
    } else if (!strcmp(cmd, "boot")) {
        return do_boot(lkb, len, result);
    } else if (!strcmp(cmd, "getsysparam")) {
        const void *ptr;
        size_t len;
        if (sysparam_get_ptr(arg, &ptr, &len) == 0) {
            lkb_write(lkb, ptr, len);
        }
    } else if (!strcmp(cmd, "reboot")) {
        thread_resume(thread_create("reboot", &do_reboot, NULL,
            DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    } else {
        *result = "unknown command";
        return -1;
    }

    return 0;
}
