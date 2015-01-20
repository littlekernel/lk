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
#include <lib/bootargs.h>

#include <trace.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <string.h>
#include <lk/init.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#define LOCAL_TRACE 0

/* saved by the main entry point in lk */
extern ulong lk_boot_args[4];

static bool boot_args_valid = false;
static struct lk_boot_arg *boot_args;

#define LK_BOOT_ARG_MAGIC 'lkbt'

/* lk style boot args are spread across the 4 incoming argument pointers as follows:
 *
 * [0] = LK_BOOT_ARG_MAGIC
 * [1] = pointer to lk_boot_arg list
 * [2] = unused
 * [3] = xor of ULONG_MAX and the previous 3
 */

#define LK_BOOT_ARG_TYPE_INITIAL 'args'
#define LK_BOOT_ARG_TYPE_COMMAND_LINE 'cmdl'
#define LK_BOOT_ARG_TYPE_BOOTIMAGE 'bimg'
#define LK_BOOT_ARG_TYPE_END 0

struct lk_boot_arg {
    uint32_t type;
    uint32_t len;

    uint8_t  data[];
};

struct lk_boot_arg_bootimage {
    uint64_t offset;
    size_t len;

    char device[];
};

static void *find_end(void *buf, size_t buf_len)
{
    uint8_t *ptr = (uint8_t *)buf;
    struct lk_boot_arg *arg = (struct lk_boot_arg *)ptr;

    while (arg->type != LK_BOOT_ARG_TYPE_END) {
        ptr += sizeof(struct lk_boot_arg) + arg->len;

        if ((uintptr_t)ptr - (uintptr_t)buf > buf_len)
            return NULL;

        arg = (struct lk_boot_arg *)ptr;
    }

    return ptr;
}

status_t bootargs_start(void *buf, size_t buf_len)
{
    if (buf_len < sizeof(struct lk_boot_arg))
        return ERR_NO_MEMORY;

    memset(buf, 0, buf_len);

    struct lk_boot_arg *arg = (struct lk_boot_arg *)buf;
    arg->type = LK_BOOT_ARG_TYPE_INITIAL;
    arg->len = 0;

    return NO_ERROR;
}

status_t bootargs_add_command_line(void *buf, size_t buf_len, const char *str)
{
    struct lk_boot_arg *arg = find_end(buf, buf_len);
    if (!arg)
        return ERR_NO_MEMORY;

    arg->type = LK_BOOT_ARG_TYPE_COMMAND_LINE;
    arg->len = ROUNDUP(strlen(str) + 1, 4);
    memset(arg->data, 0, arg->len);
    strcpy((char *)arg->data, str);

    return NO_ERROR;
}

status_t bootargs_add_bootimage_pointer(void *buf, size_t buf_len, const char *device, uint64_t offset, size_t len)
{
    struct lk_boot_arg *arg = find_end(buf, buf_len);
    if (!arg)
        return ERR_NO_MEMORY;

    arg->type = LK_BOOT_ARG_TYPE_BOOTIMAGE;
    size_t string_len = device ? ROUNDUP(strlen(device) + 1, 4) : 0;
    arg->len = string_len + sizeof(struct lk_boot_arg_bootimage);

    struct lk_boot_arg_bootimage *bi = (struct lk_boot_arg_bootimage *)arg->data;

    bi->offset = offset;
    bi->len = len;
    if (device) {
        memset(bi->device, 0, string_len);
        strcpy(bi->device, device);
    }

    return NO_ERROR;
}

void bootargs_generate_lk_arg_values(ulong buf, ulong args[4])
{
    args[0] = LK_BOOT_ARG_MAGIC;
    args[1] = buf;
    args[2] = 0;
    args[3] = ULONG_MAX ^ args[0] ^ args[1] ^ args[2];
}

static void bootargs_init_hook(uint level)
{
    LTRACE_ENTRY;

    /* see if there are any lk style boot arguments here */
    if (lk_boot_args[0] != LK_BOOT_ARG_MAGIC) {
        LTRACEF("failed magic check\n");
        return;
    }

    if (lk_boot_args[3] != (ULONG_MAX ^ lk_boot_args[0] ^ lk_boot_args[1] ^ lk_boot_args[2])) {
        LTRACEF("failed checksum\n");
        return;
    }

    /* parse the boot arg pointer */
#if WITH_KERNEL_VM
    boot_args = paddr_to_kvaddr(lk_boot_args[1]);
#else
    boot_args = (void *)lk_boot_args[1];
#endif

    if (!boot_args) {
        LTRACEF("null or invalid boot pointer\n");
        return;
    }

    /* see if the initial entry is the right one */
    if (boot_args[0].type != LK_BOOT_ARG_TYPE_INITIAL) {
        LTRACEF("bad initial arg\n");
        return;
    }

    /* looks good */
    boot_args_valid = true;

    LTRACEF("valid args found\n");
}

bool bootargs_are_valid(void)
{
    return boot_args_valid;
}

static struct lk_boot_arg *find_tag(uint32_t tag)
{
    if (!boot_args_valid)
        return NULL;

    struct lk_boot_arg *arg = boot_args;

    while (arg->type != LK_BOOT_ARG_TYPE_END) {
        if (arg->type == tag)
            return arg;

        arg = (struct lk_boot_arg *)((uintptr_t)arg + sizeof(struct lk_boot_arg) + arg->len);
    }

    return NULL;
}

const char *bootargs_get_command_line(void)
{
    struct lk_boot_arg *arg = find_tag(LK_BOOT_ARG_TYPE_COMMAND_LINE);
    if (!arg)
        return NULL;

    // XXX validate it

    return (const char *)arg->data;
}

status_t bootargs_get_bootimage_pointer(uint64_t *offset, size_t *len, const char **device)
{
    struct lk_boot_arg *arg = find_tag(LK_BOOT_ARG_TYPE_BOOTIMAGE);
    if (!arg)
        return ERR_NOT_FOUND;

    // XXX validate it

    struct lk_boot_arg_bootimage *bi = (struct lk_boot_arg_bootimage *)arg->data;

    if (device) {
        if (arg->len != sizeof(struct lk_boot_arg_bootimage)) {
            /* string is present */
            *device = bi->device;
        } else {
            *device = NULL;
        }
    }

    if (offset)
        *offset = bi->offset;
    if (len)
        *len = bi->len;

    return NO_ERROR;
}

LK_INIT_HOOK(bootargs, bootargs_init_hook, LK_INIT_LEVEL_THREADING);

