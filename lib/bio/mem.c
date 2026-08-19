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
#include <lk/pow2.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

#define BLOCKSIZE 512

typedef struct mem_bdev {
    bdev_t dev; // base device

    void *ptr;
} mem_bdev_t;

// A NOR-flash-like variant of the above. The erase geometry has to outlive the
// device -- bio stores the pointer rather than copying the array -- so it is
// embedded here. mem_bdev_t must stay first: the read/write hooks below are
// shared with the plain device and cast bdev_t* straight to mem_bdev_t*.
typedef struct nor_mem_bdev {
    mem_bdev_t mem;

    bio_erase_geometry_info_t geometry;
} nor_mem_bdev_t;

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

// Erase for the NOR-like device. Real flash can only erase whole sectors, so an
// unaligned request is rejected rather than quietly widened: over-erasing would
// destroy a neighbouring page, and returning the rounded-up count would break
// callers that compare the result against what they asked for.
//
// Note this models erase and program, not the bit-level behavior of NOR: a write
// here is a plain memcpy, not an AND against what is already there. Nothing in
// the tree reprograms an un-erased page, and some tests deliberately scribble
// over metadata to check recovery, so keep it that way.
static ssize_t mem_bdev_erase(struct bdev *bdev, off_t offset, size_t len) {
    mem_bdev_t *mem = (mem_bdev_t *)bdev;

    LTRACEF("bdev %s, offset %lld, len %zu\n", bdev->name, offset, len);

    DEBUG_ASSERT(bdev->geometry_count == 1 && bdev->geometry);
    const size_t erase_size = bdev->geometry->erase_size;

    if (((size_t)offset & (erase_size - 1)) || (len & (erase_size - 1))) {
        return ERR_INVALID_ARGS;
    }

    len = bio_trim_range(bdev, offset, len);
    if (len == 0) {
        return 0;
    }

    memset((uint8_t *)mem->ptr + (size_t)offset, bdev->erase_byte, len);

    return (ssize_t)len;
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

/* hooks shared by both flavors; the caller has already run bio_initialize_bdev */
static void mem_bdev_set_hooks(mem_bdev_t *mem, void *ptr) {
    mem->ptr = ptr;
    mem->dev.read = mem_bdev_read;
    mem->dev.read_async = mem_bdev_read_async;
    mem->dev.read_block = mem_bdev_read_block;
    mem->dev.write = mem_bdev_write;
    mem->dev.write_async = mem_bdev_write_async;
    mem->dev.write_block = mem_bdev_write_block;
    mem->dev.ioctl = mem_bdev_ioctl;
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
    mem_bdev_set_hooks(mem, ptr);

    /* register it */
    bio_register_device(&mem->dev);

    return 0;
}

int create_nor_membdev(const char *name, void *ptr, size_t len,
                       size_t erase_size, uint8_t erase_byte) {
    if (name == NULL || ptr == NULL) {
        return ERR_INVALID_ARGS;
    }

    /* the erase unit has to be a power of 2 and hold a whole number of blocks */
    if (erase_size == 0 || !ispow2(erase_size) || erase_size < BLOCKSIZE) {
        return ERR_INVALID_ARGS;
    }

    /* no partial erase unit at the end of the device */
    if (len == 0 || (len & (erase_size - 1))) {
        return ERR_INVALID_ARGS;
    }

    nor_mem_bdev_t *nor = malloc(sizeof(nor_mem_bdev_t));
    if (nor == NULL) {
        return ERR_NO_MEMORY;
    }

    /* one uniform erase region covering the whole device. erase_size is in
     * bytes and erase_shift is its log2, per the contract in lib/bio.h. */
    nor->geometry.start = 0;
    nor->geometry.size = (off_t)len;
    nor->geometry.erase_size = erase_size;
    nor->geometry.erase_shift = log2_uint(erase_size);

    bio_initialize_bdev(&nor->mem.dev, name, BLOCKSIZE, len / BLOCKSIZE, 1,
                        &nor->geometry, BIO_FLAGS_NONE);

    mem_bdev_set_hooks(&nor->mem, ptr);
    nor->mem.dev.erase = mem_bdev_erase;

    /* bio_initialize_bdev zeroes erase_byte, so this has to come after it */
    nor->mem.dev.erase_byte = erase_byte;

    bio_register_device(&nor->mem.dev);

    return 0;
}
