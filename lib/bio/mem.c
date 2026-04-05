/*
 * Copyright (c) 2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/bio.h>
#include <lk/err.h>
#include <lk/debug.h>
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

    len = bio_trim_range(bdev, offset, len);
    if (len == 0) {
        return 0;
    }

    memcpy(buf, (uint8_t *)mem->ptr + (size_t)offset, len);

    return (ssize_t)len;
}

static ssize_t mem_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count) {
    mem_bdev_t *mem = (mem_bdev_t *)bdev;

    LTRACEF("bdev %s, buf %p, block %u, count %u\n", bdev->name, buf, block, count);

    count = bio_trim_block_range(bdev, block, count);
    if (count == 0) {
        return 0;
    }

    size_t offset = (size_t)block * BLOCKSIZE;
    size_t bytes = (size_t)count * BLOCKSIZE;

    memcpy(buf, (uint8_t *)mem->ptr + offset, bytes);

    return (ssize_t)bytes;
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

    len = bio_trim_range(bdev, offset, len);
    if (len == 0) {
        return 0;
    }

    memcpy((uint8_t *)mem->ptr + (size_t)offset, buf, len);

    return (ssize_t)len;
}

static ssize_t mem_bdev_write_block(struct bdev *bdev, const void *buf, bnum_t block, uint count) {
    mem_bdev_t *mem = (mem_bdev_t *)bdev;

    LTRACEF("bdev %s, buf %p, block %u, count %u\n", bdev->name, buf, block, count);

    count = bio_trim_block_range(bdev, block, count);
    if (count == 0) {
        return 0;
    }

    size_t offset = (size_t)block * BLOCKSIZE;
    size_t bytes = (size_t)count * BLOCKSIZE;

    memcpy((uint8_t *)mem->ptr + offset, buf, bytes);

    return (ssize_t)bytes;
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

static int mem_bdev_ioctl(struct bdev *bdev, int request, void *argp) {
    mem_bdev_t *mem = (mem_bdev_t *)bdev;

    switch (request) {
    case BIO_IOCTL_GET_MEM_MAP:
    case BIO_IOCTL_GET_MAP_ADDR: {
        // For a memory-backed device, return a pointer to the backing memory.
        if (argp == NULL) {
            return ERR_INVALID_ARGS;
        }
        void **out = (void **)argp;
        *out = mem->ptr;
        return NO_ERROR;
    }
    case BIO_IOCTL_PUT_MEM_MAP:
        // No cleanup needed for RAM-backed device.
        return NO_ERROR;
    case BIO_IOCTL_IS_MAPPED: {
        // Memory-backed devices are always mapped.
        if (argp == NULL) {
            return ERR_INVALID_ARGS;
        }
        bool *out = (bool *)argp;
        *out = true;
        return NO_ERROR;
    }
    default:
        return ERR_NOT_SUPPORTED;
    }
}

int create_membdev(const char *name, void *ptr, size_t len) {
    if (name == NULL || ptr == NULL) {
        return ERR_INVALID_ARGS;
    }

    mem_bdev_t *mem = malloc(sizeof(mem_bdev_t));
    if (mem == NULL) {
        return ERR_NO_MEMORY;
    }

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
    mem->dev.ioctl = mem_bdev_ioctl;

    /* register it */
    bio_register_device(&mem->dev);

    return 0;
}
