/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
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
#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <err.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <list.h>
#include <lib/bio.h>
#include <lib/cksum.h>
#include <lib/sysparam.h>
#include <lk/init.h>

/* implementation of system parameter block, stored on a block device */
/* sysparams are simple name/value pairs, with the data unstructured */
#define LOCAL_TRACE 0

#define SYSPARAM_MAGIC 'SYSP'

#define SYSPARAM_FLAG_LOCK 0x1

struct sysparam_phys {
    uint32_t magic;
    uint32_t crc32; // crc of entire structure below crc including padding
    uint32_t flags;
    uint16_t namelen;
    uint16_t datalen;

    //uint8_t name[namelen];
    // 0 padding to next multiple of 4
    //uint8_t data[data];
    // 0 padding to next multiple of 4

    uint8_t namedata[0];
};

/* a copy we keep in memory */
struct sysparam {
    struct list_node node;

    uint32_t flags;

    char *name;

    size_t datalen;
    void *data;

    /* in memory size to hold this structure, the name string, and the data */
    size_t memlen;
};

/* global state */
static struct {
    struct list_node list;

    bool dirty;

    bdev_t *bdev;
    off_t offset;
    size_t len;
} params;

static void sysparam_init(uint level)
{
    list_initialize(&params.list);
}

LK_INIT_HOOK(sysparam, &sysparam_init, LK_INIT_LEVEL_THREADING);

static inline bool sysparam_is_locked(const struct sysparam *param)
{
    return param->flags & SYSPARAM_FLAG_LOCK;
}

static inline size_t sysparam_len(const struct sysparam_phys *sp)
{
    size_t len = sizeof(struct sysparam_phys);

    len += ROUNDUP(sp->namelen, 4);
    len += ROUNDUP(sp->datalen, 4);

    return len;
}

static inline uint32_t sysparam_crc32(const struct sysparam_phys *sp)
{
    size_t len = sysparam_len(sp);

    LTRACEF("len %d\n", len);
    uint32_t sum = crc32(0, (const void *)&sp->flags, len - 8);
    LTRACEF("sum is 0x%x\n", sum);

    return sum;
}

static struct sysparam *sysparam_create(const char *name, size_t namelen, const void *data, size_t datalen, uint32_t flags)
{
    struct sysparam *param = malloc(sizeof(struct sysparam));
    if (!param)
        return NULL;

    param->flags = flags;
    param->memlen = sizeof(struct sysparam);

    param->name = malloc(namelen + 1);
    if (!param->name) {
        free(param);
        return NULL;
    }
    param->memlen += namelen + 1;

    memcpy(param->name, name, namelen);
    param->name[namelen] = '\0';

    param->datalen = datalen;
    size_t alloclen = ROUNDUP(datalen, 4); /* allocate a multiple of 4 for padding purposes */
    param->data = malloc(alloclen);
    if (!param->data) {
        free(param->name);
        free(param);
        return NULL;
    }
    param->memlen += alloclen;

    memcpy(param->data, data, datalen);
    memset((char *)param->data + datalen, 0, alloclen - datalen); /* zero out the trailing space */

    return param;
}

static struct sysparam *sysparam_read_phys(const struct sysparam_phys *sp)
{
    return sysparam_create((const char *)sp->namedata, sp->namelen, sp->namedata + ROUNDUP(sp->namelen, 4), sp->datalen, sp->flags);
}

static struct sysparam *sysparam_find(const char *name)
{
    struct sysparam *param;
    list_for_every_entry(&params.list, param, struct sysparam, node) {
        if (strcmp(name, param->name) == 0)
            return param;
    }

    return NULL;
}

status_t sysparam_scan(bdev_t *bdev, off_t offset, size_t len)
{
    status_t err = NO_ERROR;

    LTRACEF("bdev %p (%s), offset 0x%llx, len 0x%zx\n", bdev, bdev->name, offset, len);

    DEBUG_ASSERT(bdev);
    DEBUG_ASSERT(len > 0);
    DEBUG_ASSERT(offset + len <= bdev->total_size);
    DEBUG_ASSERT((offset % bdev->block_size) == 0);

    params.bdev = bdev;
    params.offset = offset;
    params.len = len;
    params.dirty = false;

    /* allocate a len sized block */
    uint8_t *buf = malloc(len);
    if (!buf)
        return ERR_NO_MEMORY;

    /* read in the sector at the scan offset */
    err = bio_read(bdev, buf, offset, len);
    if (err < (ssize_t)len) {
        err = ERR_IO;
        goto err;
    }

    LTRACEF("looking for sysparams in block:\n");
    if (LOCAL_TRACE)
        hexdump(buf, len);

    size_t pos = 0;
    while (pos < len) {
        struct sysparam_phys *sp = (struct sysparam_phys *)(buf + pos);

        /* examine the sysparam entry, making sure it's valid */
        if (sp->magic != SYSPARAM_MAGIC) {
            pos += 4; /* try searching in the next spot */
            //LTRACEF("failed magic check\n");
            continue;
        }

        /* looks valid, see if length is sane */
        size_t splen = sysparam_len(sp);
        if (pos + splen > offset + len) {
            /* length exceeds the size of the area */
            LTRACEF("param at 0x%x: bad length\n", pos);
            break;
        }

        pos += splen;

        /* calculate a checksum of it */
        uint32_t sum = sysparam_crc32(sp);

        if (sp->crc32 != sum) {
            /* failed checksum */
            LTRACEF("param at 0x%x: failed checksum\n", pos - splen);
            continue;
        }

        LTRACEF("got param at offset 0x%zx\n", pos - splen);

        struct sysparam *param = sysparam_read_phys(sp);
        if (!param) {
            LTRACEF("param at 0x%x: failed to make memory copy\n", pos - splen);
            err = ERR_NO_MEMORY;
            break;
        }

        list_add_tail(&params.list, &param->node);
    }


err:
    free(buf);

    LTRACE_EXIT;
    return err;
}

status_t sysparam_reload(void)
{
    if (params.bdev == NULL)
        return ERR_INVALID_ARGS;
    if (params.len == 0)
        return ERR_INVALID_ARGS;

    /* wipe out the existing memory entries */
    struct sysparam *param;
    struct sysparam *temp;
    list_for_every_entry_safe(&params.list, param, temp, struct sysparam, node) {
        list_delete(&param->node);

        free(param->name);
        free(param->data);
        free(param);
    }

    /* reset the list back to scratch */
    params.dirty = false;

    status_t err = sysparam_scan(params.bdev, params.offset, params.len);

    return err;
}

ssize_t sysparam_read(const char *name, void *data, size_t len)
{
    struct sysparam *param;

    param = sysparam_find(name);
    if (!param)
        return ERR_NOT_FOUND;

    size_t toread = MIN(len, param->datalen);
    memcpy(data, param->data, toread);

    return toread;
}

ssize_t sysparam_length(const char *name)
{
    struct sysparam *param;

    param = sysparam_find(name);
    if (!param)
        return ERR_NOT_FOUND;

    return param->datalen;
}

status_t sysparam_get_ptr(const char *name, const void **ptr, size_t *len)
{
    struct sysparam *param;

    param = sysparam_find(name);
    if (!param)
        return ERR_NOT_FOUND;

    if (ptr)
        *ptr = param->data;
    if (len)
        *len = param->datalen;

    return NO_ERROR;
}

#if SYSPARAM_ALLOW_WRITE

/* write all of the parameters in memory to the space reserved in flash */
status_t sysparam_write(void)
{
    if (params.bdev == NULL)
        return ERR_INVALID_ARGS;
    if (params.len == 0)
        return ERR_INVALID_ARGS;

    if (!params.dirty)
        return NO_ERROR;

    /* preflight the length, make sure we have enough space */
    struct sysparam *param;
    off_t total_len = 0;
    list_for_every_entry(&params.list, param, struct sysparam, node) {
        total_len += sizeof(struct sysparam_phys);
        total_len += ROUNDUP(strlen(param->name), 4);
        total_len += ROUNDUP(param->datalen, 4);
    }

    if (total_len > params.len)
        return ERR_NO_MEMORY;

    /* allocate a buffer to stage it */
    uint8_t *buf = calloc(1, params.len);
    if (!buf) {
        TRACEF("error allocating buffer to stage write\n");
        return ERR_NO_MEMORY;
    }

    /* erase the block device area this covers */
    ssize_t err = bio_erase(params.bdev, params.offset, params.len);
    if (err < (ssize_t)params.len) {
        TRACEF("error erasing sysparam area\n");
        free(buf);
        return ERR_IO;
    }

    /* serialize all of the parameters */
    off_t pos = 0;
    list_for_every_entry(&params.list, param, struct sysparam, node) {
        struct sysparam_phys phys;

        /* start filling out a struct */
        phys.magic = SYSPARAM_MAGIC;
        phys.crc32 = 0;
        phys.flags = param->flags;
        phys.namelen = strlen(param->name);
        phys.datalen = param->datalen;

        /* calculate the crc of the entire thing + padding */
        uint32_t zero = 0;
        uint32_t sum = crc32(0, (const void *)&phys.flags, 8);
        sum = crc32(sum, (const void *)param->name, strlen(param->name));
        if (strlen(param->name) % 4)
            sum = crc32(sum, (const void *)&zero, 4 - (strlen(param->name) % 4));
        sum = crc32(sum, (const void *)param->data, ROUNDUP(param->datalen, 4));
        phys.crc32 = sum;

        /* structure portion */
        memcpy(buf + pos, &phys, sizeof(struct sysparam_phys));
        pos += sizeof(struct sysparam_phys);

        /* name portion */
        memcpy(buf + pos, param->name, strlen(param->name));
        pos += ROUNDUP(strlen(param->name), 2);
        if (pos % 4) {
            /* write 2 zeros to realign */
            pos += 2;
        }

        /* data portion */
        memcpy(buf + pos, param->data, param->datalen);
        pos += ROUNDUP(param->datalen, 4);
    }

    /* write the block out */
    bio_write(params.bdev, buf, params.offset, params.len);

    free(buf);

    params.dirty = false;

    return NO_ERROR;
}

status_t sysparam_add(const char *name, const void *value, size_t len)
{
    struct sysparam *param;

    param = sysparam_find(name);
    if (param)
        return ERR_ALREADY_EXISTS;

    param = sysparam_create(name, strlen(name), value, len, 0);
    if (!param)
        return ERR_NO_MEMORY;

    list_add_tail(&params.list, &param->node);

    params.dirty = true;

    return NO_ERROR;
}

status_t sysparam_remove(const char *name)
{
    struct sysparam *param;

    param = sysparam_find(name);
    if (!param)
        return ERR_NOT_FOUND;

    if (sysparam_is_locked(param))
        return ERR_NOT_ALLOWED;

    list_delete(&param->node);

    free(param->name);
    free(param->data);
    free(param);

    params.dirty = true;

    return NO_ERROR;
}

status_t sysparam_lock(const char *name)
{
    struct sysparam *param;

    param = sysparam_find(name);
    if (!param)
        return ERR_NOT_FOUND;

    /* set the lock bit if it isn't already */
    if (!sysparam_is_locked(param)) {
        param->flags |= SYSPARAM_FLAG_LOCK;
        params.dirty = true;
    }

    return NO_ERROR;
}

#endif // SYSPARAM_ALLOW_WRITE

#define MAX_DUMP_LEN  16

void sysparam_dump(bool show_all)
{
    printf("system parameters:\n");

    size_t total_memlen = 0;

    struct sysparam *param;
    list_for_every_entry(&params.list, param, struct sysparam, node) {
        printf("________%c %-16s : ",
               (param->flags & SYSPARAM_FLAG_LOCK) ? 'L' : '_',
               param->name);

        const uint8_t *dat = (const uint8_t *)param->data;
        uint32_t pr_len = param->datalen;

        if (!show_all) {
            if (pr_len > MAX_DUMP_LEN)
                pr_len = MAX_DUMP_LEN;
        }

        for (uint i = 0; i < pr_len; i++) {
            printf("%02x", dat[i]);
        }
        if (pr_len != param->datalen)
            printf("...\n");
        else
            printf("\n");

        total_memlen += param->memlen;
    }

    printf("total in-memory usage: %zu bytes\n", total_memlen);
}

#if WITH_LIB_CONSOLE

#include <lib/console.h>
#include <ctype.h>

static ssize_t hexstr_to_val(const char *str, uint8_t **buf)
{
    /* parse the value parameter as a hex code */
    uint8_t *hexbuffer = calloc(1, strlen(str) / 2 + 1);
    uint pos;
    for (pos = 0; str[pos] != 0; pos++) {
        uint8_t c = str[pos];

        if (!isxdigit(c)) {
            free(hexbuffer);
            return ERR_NOT_VALID;
        }

        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'f')
            c -= 'a' - 0xa;
        else if (c >= 'A' && c <= 'F')
            c -= 'A' - 0xa;

        hexbuffer[pos / 2] |= (!(pos % 2)) ? (c << 4) : c;
    }
    pos = (pos + 1) / 2; /* round down, keeping partial bytes */

    *buf = hexbuffer;
    return pos;
}

static int cmd_sysparam(int argc, const cmd_args *argv)
{
    status_t err;

    if (argc < 2) {
notenoughargs:
        printf("ERROR not enough arguments\n");
usage:
        printf("usage: %s dump\n", argv[0].str);
        printf("usage: %s list\n", argv[0].str);
        printf("usage: %s reload\n", argv[0].str);
#if SYSPARAM_ALLOW_WRITE
        printf("usage: %s add <param> <string value>\n", argv[0].str);
        printf("usage: %s addhex <param> <hex value>\n", argv[0].str);
        printf("usage: %s addlong <param>\n", argv[0].str);
        printf("usage: %s remove <param>\n", argv[0].str);
        printf("usage: %s lock <param>\n", argv[0].str);
        printf("usage: %s write\n", argv[0].str);
#endif
        printf("usage: %s length <param>\n", argv[0].str);
        printf("usage: %s read <param>\n", argv[0].str);
        return -1;
    }

    err = NO_ERROR;
    if (!strcmp(argv[1].str, "dump")) {
        sysparam_dump(true);
    } else if (!strcmp(argv[1].str, "list")) {
        struct sysparam *param;
        list_for_every_entry(&params.list, param, struct sysparam, node) {
            printf("%s\n", param->name);
        }
    } else if (!strcmp(argv[1].str, "reload")) {
        err = sysparam_reload();
#if SYSPARAM_ALLOW_WRITE
    } else if (!strcmp(argv[1].str, "add")) {
        if (argc < 4) goto notenoughargs;

        err = sysparam_add(argv[2].str, argv[3].str, strlen(argv[3].str));
    } else if (!strcmp(argv[1].str, "addhex")) {
        if (argc < 4) goto notenoughargs;

        /* parse the value parameter as a hex code */
        uint8_t *hexbuffer;

        ssize_t len = hexstr_to_val(argv[3].str, &hexbuffer);
        if (len < 0) {
            err = ERR_INVALID_ARGS;
            goto done;
        }

        err = sysparam_add(argv[2].str, hexbuffer, len);
        free(hexbuffer);
    } else if (!strcmp(argv[1].str, "addlong")) {
        if (argc < 3) goto notenoughargs;

        char *str;
        ssize_t buflen = 64;
        ssize_t len = 0;
        str = malloc(buflen + 1);
        if (!str) {
            err = ERR_NO_MEMORY;
            goto done;
        }

        for (;;) {
            int c = getchar();
            if (err < 0)
                break;
            if (c == '\r')
                continue;
            if (c == '\04') /* ^d */
                break;
            if (c == '\n')
                break;

            if (len == buflen) {
                buflen *= 2;
                char *origstr = str;
                str = realloc(str, buflen + 1);
                if (!str) {
                    err = ERR_NO_MEMORY;
                    free(origstr);
                    goto done;
                }
            }

            str[len++] = c;
        }
        str[len] = '\0';

        /* parse the value parameter as a hex code */
        uint8_t *hexbuffer;

        len = hexstr_to_val(str, &hexbuffer);

        free(str);

        if (len < 0) {
            err = ERR_INVALID_ARGS;
            goto done;
        }

        err = sysparam_add(argv[2].str, hexbuffer, len);

        free(hexbuffer);
    } else if (!strcmp(argv[1].str, "remove")) {
        if (argc < 3) goto notenoughargs;

        err = sysparam_remove(argv[2].str);
    } else if (!strcmp(argv[1].str, "lock")) {
        if (argc < 3) goto notenoughargs;

        err = sysparam_lock(argv[2].str);
    } else if (!strcmp(argv[1].str, "write")) {
        err = sysparam_write();
    } else if (!strcmp(argv[1].str, "nuke")) {
        ssize_t err = bio_erase(params.bdev, params.offset, params.len);
        printf("erase returns %d\n", (int)err);
#endif // SYSPARAM_ALLOW_WRITE
    } else if (!strcmp(argv[1].str, "length")) {
        if (argc < 3) goto notenoughargs;
        ssize_t len = sysparam_length(argv[2].str);
        if (len >= 0) {
            printf("%zu\n", (size_t)len);
        }
        err = (len >= 0) ? NO_ERROR : len;
    } else if (!strcmp(argv[1].str, "read")) {
        if (argc < 3) goto notenoughargs;
        ssize_t len = sysparam_length(argv[2].str);
        if (len < 0) {
            err = len;
            goto done;
        }
        uint8_t *buf = malloc(len);
        if (!buf) {
            err = ERR_NO_MEMORY;
            goto done;
        }
        len = sysparam_read(argv[2].str, buf, len);
        if (len < 0) {
            err = len;
            goto done;
        }
        err = NO_ERROR;

        for (int i = 0; i < len; i++)
            printf("%02x", buf[i]);
        printf("\n");
        free(buf);
    } else {
        printf("ERROR unknown command\n");
        goto usage;
    }

done:
    /* shared error reporting */
    if (err >= NO_ERROR) {
        printf("OK\n");
    } else if (err == ERR_NO_MEMORY) {
        printf("ERROR out of memory\n");
    } else if (err == ERR_ALREADY_EXISTS) {
        printf("ERROR already exists\n");
    } else if (err == ERR_INVALID_ARGS) {
        printf("ERROR invalid argument\n");
    } else if (err == ERR_NOT_FOUND) {
        printf("ERROR not found\n");
    } else if (err == ERR_NOT_ALLOWED) {
        printf("ERROR not allowed (locked)\n");
    } else {
        printf("ERROR generic error %d\n", err);
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("sysparam", "commands for manipulating system parameters", &cmd_sysparam)
STATIC_COMMAND_END(sysparam);

#endif // WITH_LIB_CONSOLE
