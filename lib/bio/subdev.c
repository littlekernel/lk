/*
 * Copyright (c) 2009 Travis Geiselbrecht
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
#include <err.h>
#include <trace.h>
#include <stdlib.h>
#include <lib/bio.h>

#define LOCAL_TRACE 0

typedef struct {
    // inheirit the usual bits
    bdev_t dev;

    // we're a subdevice of this
    bdev_t *parent;

    // Storage for our erase geometry info.  Subdevices are only permitted to
    // have homogeneous erase geometry.
    bio_erase_geometry_info_t geometry;

    // we're this many blocks into it
    bnum_t offset;
} subdev_t;

static ssize_t subdev_read(struct bdev *_dev, void *buf, off_t offset, size_t len)
{
    subdev_t *subdev = (subdev_t *)_dev;

    return bio_read(subdev->parent, buf, offset + subdev->offset * subdev->dev.block_size, len);
}

static ssize_t subdev_read_block(struct bdev *_dev, void *buf, bnum_t block, uint count)
{
    subdev_t *subdev = (subdev_t *)_dev;

    return bio_read_block(subdev->parent, buf, block + subdev->offset, count);
}

static ssize_t subdev_write(struct bdev *_dev, const void *buf, off_t offset, size_t len)
{
    subdev_t *subdev = (subdev_t *)_dev;

    return bio_write(subdev->parent, buf, offset + subdev->offset * subdev->dev.block_size, len);
}

static ssize_t subdev_write_block(struct bdev *_dev, const void *buf, bnum_t block, uint count)
{
    subdev_t *subdev = (subdev_t *)_dev;

    return bio_write_block(subdev->parent, buf, block + subdev->offset, count);
}

static ssize_t subdev_erase(struct bdev *_dev, off_t offset, size_t len)
{
    subdev_t *subdev = (subdev_t *)_dev;

    return bio_erase(subdev->parent, offset + subdev->offset * subdev->dev.block_size, len);
}

static void subdev_close(struct bdev *_dev)
{
    subdev_t *subdev = (subdev_t *)_dev;

    bio_close(subdev->parent);
    subdev->parent = NULL;
}

#define BAIL(__err) do { err = __err; goto bailout; } while (0)
status_t bio_publish_subdevice(const char *parent_dev,
                               const char *subdev,
                               bnum_t startblock,
                               bnum_t block_count)
{
    status_t err = NO_ERROR;
    bdev_t *parent = NULL;
    subdev_t *sub = NULL;
    size_t geometry_count;
    bio_erase_geometry_info_t *geometry;

    LTRACEF("parent \"%s\", sub \"%s\", startblock %u, count %u\n", parent_dev, subdev, startblock, block_count);

    // Make sure our parent exists
    parent = bio_open(parent_dev);
    if (!parent) {
        LTRACEF("Failed to find parent \"%s\"\n", parent_dev);
        BAIL(ERR_NOT_FOUND);
    }

    // Allocate our sub-device.
    sub = malloc(sizeof(subdev_t));
    if (!sub) {
        LTRACEF("Failed to allocate subdevice\n");
        BAIL(ERR_NO_MEMORY);
    }

    /* Make sure we're able to do this.  If the device has a specified erase
     * geometry, the specified sub-region must exist entirely with one of our
     * parent's erase regions, and be aligned to an erase unit boundary.
     * Otherwise, the specified region must simply exist within the block range
     * of the device.
     */
    if (parent->geometry_count && parent->geometry) {
        uint64_t byte_start = ((uint64_t)startblock  << parent->block_shift);
        uint64_t byte_size  = ((uint64_t)block_count << parent->block_shift);
        const bio_erase_geometry_info_t *geo = NULL;

        LTRACEF("Searching geometry for region which contains @[0x%llx, 0x%llx)\n",
                byte_start, byte_start + byte_size);

        // Start by finding the erase region which completely contains the requested range.
        for (size_t i = 0; i < parent->geometry_count; ++i) {
            geo = parent->geometry + i;

            LTRACEF("Checking geometry @[0x%llx, 0x%llx) erase size 0x%zx\n",
                    geo->start,
                    geo->start + geo->size,
                    geo->erase_size);

            if (bio_contains_range(geo->start, geo->size, byte_start, byte_size))
                break;
        }

        if (!geo) {
            LTRACEF("No suitable erase region found\n");
            BAIL(ERR_INVALID_ARGS);
        }

        // Now check out alignment.
        uint64_t erase_mask = ((uint64_t)0x1 << geo->erase_shift) - 1;
        if ((byte_start & erase_mask) || (byte_size & erase_mask)) {
            LTRACEF("Requested region has improper alignment/length\n");
            BAIL(ERR_INVALID_ARGS);
        }

        geometry_count = 1;
        geometry = &sub->geometry;

        geometry->start       = 0;
        geometry->size        = byte_size;
        geometry->erase_size  = geo->erase_size;
        geometry->erase_shift = geo->erase_shift;
    } else {
        bnum_t endblock = startblock + block_count;

        if ((endblock < startblock) || (endblock > parent->block_count))
            BAIL(ERR_INVALID_ARGS);

        geometry_count = 0;
        geometry = NULL;
    }

    bio_initialize_bdev(&sub->dev, subdev,
                        parent->block_size, block_count,
                        geometry_count, geometry, BIO_FLAGS_NONE);

    sub->parent = parent;
    sub->offset = startblock;

    sub->dev.read = &subdev_read;
    sub->dev.read_block = &subdev_read_block;
    sub->dev.write = &subdev_write;
    sub->dev.write_block = &subdev_write_block;
    sub->dev.erase = &subdev_erase;
    sub->dev.close = &subdev_close;

    bio_register_device(&sub->dev);

bailout:
    if (err < 0) {
        if (NULL != parent)
            bio_close(parent);
        free(sub);
    }

    return err;
}
#undef BAIL
