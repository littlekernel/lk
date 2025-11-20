/*
 * Copyright (c) 2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/bio.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

#define BLOCKSIZE 512

typedef struct mem_bdev {
    bdev_t dev; // base device

    void *ptr;
} mem_bdev_t;

static ssize_t mem_bdev_read(bdev_t *bdev, void *buf, off_t offset, size_t len) {
    mem_bdev_t *mem = (mem_bdev_t *)bdev;

    LTRACEF("bdev %s, buf %p, offset %lld, len %zu\n", bdev->name, buf, offset, len);

    memcpy(buf, (uint8_t *)mem->ptr + offset, len);

    return len;
}

static ssize_t mem_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count) {
    mem_bdev_t *mem = (mem_bdev_t *)bdev;

    LTRACEF("bdev %s, buf %p, block %u, count %u\n", bdev->name, buf, block, count);

    uint64_t block_offset, total_bytes;
    if (!bio_mul_u64_size(block, BLOCKSIZE, &block_offset) ||
        !bio_mul_u64_size(count, BLOCKSIZE, &total_bytes)) {
        return ERR_OUT_OF_RANGE;
    }

    memcpy(buf, (uint8_t *)mem->ptr + block_offset, total_bytes);

    return total_bytes;
}

static status_t mem_bdev_read_async(struct bdev *bdev, void *buf, off_t offset, size_t len,
                                    bio_async_callback_t callback, void *cookie) {
    // Complete synchronously then invoke the callback immediately.
    ssize_t readn = mem_bdev_read(bdev, buf, offset, len);
    if (callback) {
        callback(cookie, bdev, readn);
    }
    return NO_ERROR;
}

static ssize_t mem_bdev_write(bdev_t *bdev, const void *buf, off_t offset, size_t len) {
    mem_bdev_t *mem = (mem_bdev_t *)bdev;

    LTRACEF("bdev %s, buf %p, offset %lld, len %zu\n", bdev->name, buf, offset, len);

    memcpy((uint8_t *)mem->ptr + offset, buf, len);

    return len;
}

static ssize_t mem_bdev_write_block(struct bdev *bdev, const void *buf, bnum_t block, uint count) {
    mem_bdev_t *mem = (mem_bdev_t *)bdev;

    LTRACEF("bdev %s, buf %p, block %u, count %u\n", bdev->name, buf, block, count);

    uint64_t block_offset, total_bytes;
    if (!bio_mul_u64_size(block, BLOCKSIZE, &block_offset) ||
        !bio_mul_u64_size(count, BLOCKSIZE, &total_bytes)) {
        return ERR_OUT_OF_RANGE;
    }

    memcpy((uint8_t *)mem->ptr + block_offset, buf, total_bytes);

    return total_bytes;
}

static status_t mem_bdev_write_async(struct bdev *bdev, const void *buf, off_t offset, size_t len,
                                     bio_async_callback_t callback, void *cookie) {
    // Complete synchronously then invoke the callback immediately.
    ssize_t written = mem_bdev_write(bdev, buf, offset, len);
    if (callback) {
        callback(cookie, bdev, written);
    }
    return NO_ERROR;
}

int create_membdev(const char *name, void *ptr, size_t len) {
    mem_bdev_t *mem = malloc(sizeof(mem_bdev_t));

    /* set up the base device */
    bio_initialize_bdev(&mem->dev, name, BLOCKSIZE, len / BLOCKSIZE, 0, NULL,
                        BIO_FLAGS_NONE);

    /* our bits */
    mem->ptr = ptr;
    mem->dev.read = mem_bdev_read;
    mem->dev.read_async = mem_bdev_read_async;
    mem->dev.read_block = mem_bdev_read_block;
    mem->dev.write = mem_bdev_write;
    mem->dev.write_async = mem_bdev_write_async;
    mem->dev.write_block = mem_bdev_write_block;

    /* register it */
    bio_register_device(&mem->dev);

    return 0;
}
