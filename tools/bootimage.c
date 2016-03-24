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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <lib/mincrypt/sha256.h>

#include "bootimage.h"

struct bootimage {
    bootentry entry[64];
    void *data[64];
    uint32_t offset[64];
    uint32_t length[64];
    unsigned count;
    uint32_t next_offset;
};

bootimage *bootimage_init(void)
{
    bootimage *img;

    if ((img = malloc(sizeof(bootimage))) == NULL) {
        return NULL;
    }
    memset(img, 0, sizeof(bootimage));
    img->count = 2;
    img->next_offset = 4096;
    memset(img->entry, 0, 4096);
    img->entry[0].file.kind = KIND_FILE;
    img->entry[0].file.type = TYPE_BOOT_IMAGE;
    img->entry[0].file.offset = 0;
    img->entry[0].file.length = 4096;
    img->entry[1].info.kind = KIND_BOOT_INFO;
    img->entry[1].info.version = BOOT_VERSION;
    memcpy(img->entry[0].file.name, BOOT_MAGIC, BOOT_MAGIC_LENGTH);
    return img;
}

bootentry_data *bootimage_add_string(bootimage *img, unsigned kind, const char *s)
{
    unsigned n = img->count;
    int len = strlen(s);
    if (img->count == 64) return NULL;
    if (len > 59) return NULL;
    img->count++;

    img->entry[n].data.kind = kind;
    strcpy((char *) img->entry[n].data.u.b, s);
    return &(img->entry[n].data);
}

bootentry_file *bootimage_add_filedata(bootimage *img, unsigned type, void *data, unsigned len)
{
    unsigned n = img->count;
    if (img->count == 64) return NULL;
    img->count++;

    // align to page boundary
    img->next_offset = (img->next_offset + 4095) & (~4095);

    img->entry[n].file.kind = KIND_FILE;
    img->entry[n].file.type = type;
    img->entry[n].file.offset = img->next_offset;
    img->entry[n].file.length = len;
    SHA256_hash(data, len, img->entry[n].file.sha256);

    img->data[n] = data;
    img->offset[n] = img->next_offset;
    img->length[n] = len;

    img->next_offset += len;

    return &(img->entry[n].file);
}

void bootimage_done(bootimage *img)
{
    unsigned sz = img->next_offset;
    if (sz & 4095) {
        sz += (4096 - (sz & 4095));
    }
    img->entry[1].info.image_size = sz;
    img->entry[1].info.entry_count = img->count;
    SHA256_hash((void *) &(img->entry[1]), 4096 - 64, img->entry[0].file.sha256);
}

static int writex(int fd, void *data, size_t len)
{
    int r;
    char *x = data;
    while (len > 0) {
        r = write(fd, x, len);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        len -= r;
        x += r;
    }
    return 0;
}

static uint8_t filler[4096] = { 0, };

int bootimage_write(bootimage *img, int fd)
{
    unsigned off = 4096;
    unsigned n, s;
    if (writex(fd, img->entry, 4096)) {
        return -1;
    }
    for (n = 1; n < 64; n++) {
        if (img->offset[n] == 0) continue;
        if (img->offset[n] < off) return -1;
        s = img->offset[n] - off;
        if (s > 4095) return -1;
        if (writex(fd, filler, s)) {
            return -1;
        }
        off += s;
        if (writex(fd, img->data[n], img->length[n])) {
            return -1;
        }
        off += img->length[n];
    }
    if (off & 4095) {
        if (writex(fd, filler, 4096 - (off & 4095))) return -1;
    }
    return 0;
}

static void *load_file(const char *fn, size_t *len)
{
    off_t sz;
    void *data = NULL;
    char *x;
    int fd, r;

    if ((fd = open(fn, O_RDONLY)) < 0) {
        return NULL;
    }

    if ((sz = lseek(fd, 0, SEEK_END)) < 0) {
        goto fail;
    }
    if (lseek(fd, 0, SEEK_SET) != 0) {
        goto fail;
    }

    if ((data = malloc(sz)) == NULL) {
        goto fail;
    }
    x = data;
    if (len) {
        *len = sz;
    }
    while (sz > 0) {
        r = read(fd, x, sz);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            goto fail;
        }
        sz -= r;
        x += r;
    }
    close(fd);
    return data;

fail:
    if (data) {
        free(data);
    }
    close(fd);
    return NULL;
}

bootentry_file *bootimage_add_file(bootimage *img, unsigned type, const char *fn)
{
    unsigned char *data;
    size_t len;

    if ((data = load_file(fn, &len)) == NULL) {
        fprintf(stderr, "error: cannot load '%s'\n", fn);
        return NULL;
    }

    /* if fpga image, trim everything before ffffffaa995566 and wordwise endian swap */
    if (type == TYPE_FPGA_IMAGE) {
        static const unsigned char pat[] = { 0xff, 0xff, 0xff, 0xff, 0xaa, 0x99, 0x55, 0x66 };

        size_t i;
        if (len < sizeof(pat)) {
            free(data);
            fprintf(stderr, "error: fpga image too short\n");
            return NULL;
        }

        for (i = 0; i < len - sizeof(pat); i++) {
            if (!memcmp(data + i, pat, sizeof(pat))) {
                /* we've found the pattern, trim everything before it */
                memmove(data, data + i, len - i);
                len -= i;
            }
        }

        /* wordwise endian swap */
#define SWAP_32(x) \
    (((uint32_t)(x) << 24) | (((uint32_t)(x) & 0xff00) << 8) |(((uint32_t)(x) & 0x00ff0000) >> 8) | ((uint32_t)(x) >> 24))
        uint32_t *w = (uint32_t *)data;
        for (i = 0; i < len / 4; i++) {
            *w = SWAP_32(*w);
            w++;
        }
#undef SWAP_32
    }

    return bootimage_add_filedata(img, type, data, len);
}

