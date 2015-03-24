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

#include <lib/bootimage.h>
#include <trace.h>
#include <err.h>
#include <debug.h>
#include <stdlib.h>
#include <string.h>

#include <lib/bootimage_struct.h>
#include <lib/mincrypt/sha256.h>

#define LOCAL_TRACE 1

struct bootimage {
    const uint8_t *ptr;
    size_t len;
};

static status_t validate_bootimage(bootimage_t *bi)
{
    if (!bi)
        return ERR_INVALID_ARGS;

    /* is it large enough to hold the first entry */
    if (bi->len < 4096) {
        LTRACEF("bootentry too short\n");
        return ERR_BAD_LEN;
    }

    bootentry *be = (bootentry *)bi->ptr;

    /* check that the first entry is a file, type boot info, and is 4096 bytes at offset 0 */
    if (be->kind != KIND_FILE ||
            be->file.type != TYPE_BOOT_IMAGE ||
            be->file.offset != 0 ||
            be->file.length != 4096 ||
            memcmp(be->file.name, BOOT_MAGIC, sizeof(be->file.name))) {
        LTRACEF("invalid first entry\n");
        return ERR_INVALID_ARGS;
    }

    /* check the sha256 of the rest of the first page */
    SHA256_CTX ctx;
    SHA256_init(&ctx);

    SHA256_update(&ctx, be + 1, 4096 - sizeof(bootentry));
    const uint8_t *hash = SHA256_final(&ctx);

    if (memcmp(hash, be->file.sha256, sizeof(be->file.sha256)) != 0) {
        LTRACEF("bad hash of first section\n");

        return ERR_CHECKSUM_FAIL;
    }

    /* look at the second entry, which should be a boot info structure */
    if (be[1].kind != KIND_BOOT_INFO) {
        LTRACEF("second entry not boot info\n");
        return ERR_INVALID_ARGS;
    }

    bootentry_info *info = &be[1].info;

    /* is the image a handled version */
    if (info->version > BOOT_VERSION) {
        LTRACEF("unhandled version 0x%x\n", info->version);
        return ERR_INVALID_ARGS;
    }

    /* is the image the right size? */
    if (info->image_size > bi->len) {
        LTRACEF("boot image block says image is too big (0x%x bytes)\n", info->image_size);
        return ERR_INVALID_ARGS;
    }

    /* trim the len to what the info block says */
    bi->len = info->image_size;

    /* iterate over the remaining entries in the list */
    for (size_t i = 2; i < info->entry_count; i++) {
        if (be[i].kind == 0)
            break;

        LTRACEF("%u: kind 0x%x\n", i, be[i].kind);

        switch (be[i].kind) {
            case KIND_BOOT_INFO:
                break;
            case KIND_BOARD:
                break;
            case KIND_BUILD:
                break;
            case KIND_FILE: {
                LTRACEF("\ttype %c%c%c%c offset 0x%x, length 0x%x\n",
                        (be[i].file.type >> 0) & 0xff, (be[i].file.type >> 8) & 0xff,
                        (be[i].file.type >> 16) & 0xff, (be[i].file.type >> 24) & 0xff,
                        be[i].file.offset, be[i].file.length);

                /* check that the file section is inside the overall image */
                uint32_t end = be[i].file.offset + be[i].file.length;
                if (end < be[i].file.offset || end > info->image_size) {
                    LTRACEF("bad file section, size too large\n");
                    return ERR_INVALID_ARGS;
                }

                /* check the sha256 hash */
                SHA256_init(&ctx);

                LTRACEF("\tvalidating SHA256 hash\n");
                SHA256_update(&ctx, (const uint8_t *)bi->ptr + be[i].file.offset, be[i].file.length);
                const uint8_t *hash = SHA256_final(&ctx);

                if (memcmp(hash, be[i].file.sha256, sizeof(be[i].file.sha256)) != 0) {
                    LTRACEF("bad hash of file section\n");

                    return ERR_CHECKSUM_FAIL;
                }

                break;
            }
            default:
                LTRACEF("unknown kind 0x%x\n", be[i].kind);
                return ERR_INVALID_ARGS;
        }
    }

    LTRACEF("image good\n");
    return NO_ERROR;
}

status_t bootimage_open(const void *ptr, size_t len, bootimage_t **bi)
{
    LTRACEF("ptr %p, len %zu\n", ptr, len);

    if (!bi)
        return ERR_INVALID_ARGS;

    *bi = calloc(1, sizeof(bootimage_t));
    if (!*bi)
        return ERR_NO_MEMORY;

    (*bi)->ptr = ptr;
    (*bi)->len = len;

    /* try to validate it */
    status_t err = validate_bootimage(*bi);
    if (err < 0) {
        bootimage_close(*bi);
        return err;
    }

    return NO_ERROR;
}

status_t bootimage_close(bootimage_t *bi)
{
    if (bi)
        free(bi);

    return NO_ERROR;
}

status_t bootimage_get_range(bootimage_t *bi, const void **ptr, size_t *len)
{
    if (!bi)
        return ERR_INVALID_ARGS;

    if (ptr)
        *ptr = bi->ptr;
    if (len)
        *len = bi->len;

    return NO_ERROR;
}

status_t bootimage_get_file_section(bootimage_t *bi, uint32_t type, const void **ptr, size_t *len)
{
    if (!bi)
        return ERR_INVALID_ARGS;

    bootentry *be = (bootentry *)bi->ptr;
    bootentry_info *info = &be[1].info;

    for (size_t i = 2; i < info->entry_count; i++) {
        if (be[i].kind == 0)
            break;

        if (be[i].kind != KIND_FILE)
            continue;

        if (type == be[i].file.type) {
            if (ptr)
                *ptr = bi->ptr + be[i].file.offset;
            if (len)
                *len = be[i].file.length;
            return NO_ERROR;
        }
    }

    return ERR_NOT_FOUND;
}

