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
#include <stdlib.h>
#include <lib/bio.h>

#define LOCAL_TRACE 0

typedef struct {
	// inheirit the usual bits
	bdev_t dev;

	// we're a subdevice of this
	bdev_t *parent;

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

status_t bio_publish_subdevice(const char *parent_dev, const char *subdev, bnum_t startblock, size_t len)
{
	LTRACEF("parent %s, sub %s, startblock %u, len %zd\n", parent_dev, subdev, startblock, len);

	bdev_t *parent = bio_open(parent_dev);
	if (!parent)
		return -1;

	/* make sure we're able to do this */
	if (startblock + len > parent->block_count)
		return -1;

	subdev_t *sub = malloc(sizeof(subdev_t));
	bio_initialize_bdev(&sub->dev, subdev, parent->block_size, len);

	sub->parent = parent;
	sub->offset = startblock;

	sub->dev.read = &subdev_read;
	sub->dev.read_block = &subdev_read_block;
	sub->dev.write = &subdev_write;
	sub->dev.write_block = &subdev_write_block;
	sub->dev.erase = &subdev_erase;
	sub->dev.close = &subdev_close;

	bio_register_device(&sub->dev);

	return 0;
}

