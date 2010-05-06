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
#include <stdlib.h>
#include <debug.h>
#include <err.h>
#include <string.h>
#include <list.h>
#include <lib/bio.h>
#include <kernel/mutex.h>

#define LOCAL_TRACE 0

struct bdev_struct {
	struct list_node list;
	mutex_t lock;
};

static struct bdev_struct *bdevs;

/* default implementation is to use the read_block hook to 'deblock' the device */
static ssize_t bio_default_read(struct bdev *dev, void *_buf, off_t offset, size_t len)
{
	uint8_t *buf = (uint8_t *)_buf;
	ssize_t bytes_read = 0;
	bnum_t block;
	int err = 0;
	STACKBUF_DMA_ALIGN(temp, dev->block_size); // temporary buffer for partial block transfers

	/* find the starting block */
	block = offset / dev->block_size;

	LTRACEF("buf %p, offset %lld, block %u, len %zd\n", buf, offset, block, len);
	/* handle partial first block */
	if ((offset % dev->block_size) != 0) {
		/* read in the block */
		err = bio_read_block(dev, temp, block, 1);
		if (err < 0)
			goto err;

		/* copy what we need */
		size_t block_offset = offset % dev->block_size;
		size_t tocopy = MIN(dev->block_size - block_offset, len);
		memcpy(buf, temp + block_offset, tocopy);

		/* increment our buffers */
		buf += tocopy;
		len -= tocopy;
		bytes_read += tocopy;
		block++;
	}

	LTRACEF("buf %p, block %u, len %zd\n", buf, block, len);
	/* handle middle blocks */
	if (len >= dev->block_size) {
		/* do the middle reads */
		size_t block_count = len / dev->block_size;
		err = bio_read_block(dev, buf, block, block_count);
		if (err < 0)
			goto err;

		/* increment our buffers */
		size_t bytes = block_count * dev->block_size;
		DEBUG_ASSERT(bytes <= len);

		buf += bytes;
		len -= bytes;
		bytes_read += bytes;
		block += block_count;
	}

	LTRACEF("buf %p, block %u, len %zd\n", buf, block, len);
	/* handle partial last block */
	if (len > 0) {
		/* read the block */
		err = bio_read_block(dev, temp, block, 1);
		if (err < 0)
			goto err;

		/* copy the partial block from our temp buffer */
		memcpy(buf, temp, len);

		bytes_read += len;
	}

err:
	/* return error or bytes read */
	return (err >= 0) ? bytes_read : err;
}

static ssize_t bio_default_write(struct bdev *dev, const void *_buf, off_t offset, size_t len)
{
	const uint8_t *buf = (const uint8_t *)_buf;
	ssize_t bytes_written = 0;
	bnum_t block;
	int err = 0;
	STACKBUF_DMA_ALIGN(temp, dev->block_size); // temporary buffer for partial block transfers

	/* find the starting block */
	block = offset / dev->block_size;

	LTRACEF("buf %p, offset %lld, block %u, len %zd\n", buf, offset, block, len);
	/* handle partial first block */
	if ((offset % dev->block_size) != 0) {
		/* read in the block */
		err = bio_read_block(dev, temp, block, 1);
		if (err < 0)
			goto err;

		/* copy what we need */
		size_t block_offset = offset % dev->block_size;
		size_t tocopy = MIN(dev->block_size - block_offset, len);
		memcpy(temp + block_offset, buf, tocopy);

		/* write it back out */
		err = bio_write_block(dev, temp, block, 1);
		if (err < 0)
			goto err;

		/* increment our buffers */
		buf += tocopy;
		len -= tocopy;
		bytes_written += tocopy;
		block++;
	}

	LTRACEF("buf %p, block %u, len %zd\n", buf, block, len);
	/* handle middle blocks */
	if (len >= dev->block_size) {
		/* do the middle writes */
		size_t block_count = len / dev->block_size;
		err = bio_write_block(dev, buf, block, block_count);
		if (err < 0)
			goto err;

		/* increment our buffers */
		size_t bytes = block_count * dev->block_size;
		DEBUG_ASSERT(bytes <= len);

		buf += bytes;
		len -= bytes;
		bytes_written += bytes;
		block += block_count;
	}

	LTRACEF("buf %p, block %u, len %zd\n", buf, block, len);
	/* handle partial last block */
	if (len > 0) {
		/* read the block */
		err = bio_read_block(dev, temp, block, 1);
		if (err < 0)
			goto err;

		/* copy the partial block from our temp buffer */
		memcpy(temp, buf, len);

		/* write it back out */
		err = bio_write_block(dev, temp, block, 1);
		if (err < 0)
			goto err;

		bytes_written += len;
	}

err:
	/* return error or bytes written */
	return (err >= 0) ? bytes_written : err;
}

static ssize_t bio_default_erase(struct bdev *dev, off_t offset, size_t len)
{
	/* default erase operation is to just write zeros over the device */
#define ERASE_BUF_SIZE 4096
	uint8_t *zero_buf;

	zero_buf = calloc(1, ERASE_BUF_SIZE);

	size_t remaining = len;
	off_t pos = offset;
	while (remaining > 0) {
		ssize_t towrite = MIN(remaining, ERASE_BUF_SIZE);

		ssize_t written = bio_write(dev, zero_buf, pos, towrite);
		if (written < 0)
			return pos;

		pos += written;
		remaining -= written;

		if (written < towrite)
			return pos;
	}

	return len;
}

static ssize_t bio_default_read_block(struct bdev *dev, void *buf, bnum_t block, uint count)
{
	panic("%s no reasonable default operation\n", __PRETTY_FUNCTION__);
}

static ssize_t bio_default_write_block(struct bdev *dev, const void *buf, bnum_t block, uint count)
{
	panic("%s no reasonable default operation\n", __PRETTY_FUNCTION__);
}

static void bdev_inc_ref(bdev_t *dev)
{
	atomic_add(&dev->ref, 1);
}

static void bdev_dec_ref(bdev_t *dev)
{
	int oldval = atomic_add(&dev->ref, -1);
	if (oldval == 1) {
		// last ref, remove it
		DEBUG_ASSERT(!list_in_list(&dev->node));

		TRACEF("last ref, removing (%s)\n", dev->name);

		// call the close hook if it exists
		if (dev->close)
			dev->close(dev);

		free(dev->name);
		free(dev);
	}
}

bdev_t *bio_open(const char *name)
{
	bdev_t *bdev = NULL;

	/* see if it's in our list */
	bdev_t *entry;
	mutex_acquire(&bdevs->lock);
	list_for_every_entry(&bdevs->list, entry, bdev_t, node) {
		DEBUG_ASSERT(entry->ref > 0);
		if (!strcmp(entry->name, name)) {
			bdev = entry;
			bdev_inc_ref(bdev);
			break;
		}
	}
	mutex_release(&bdevs->lock);

	return bdev;
}

void bio_close(bdev_t *dev)
{
	DEBUG_ASSERT(dev);

	bdev_dec_ref(dev);
}

ssize_t bio_read(bdev_t *dev, void *buf, off_t offset, size_t len)
{
	LTRACEF("dev '%s', buf %p, offset %lld, len %zd\n", dev->name, buf, offset, len);

	DEBUG_ASSERT(dev->ref > 0);	

	/* range check */
	if (offset < 0)
		return -1;
	if (offset >= dev->size)
		return 0;
	if (len == 0)
		return 0;
	if (offset + len > dev->size)
		len = dev->size - offset;

	return dev->read(dev, buf, offset, len);
}

ssize_t bio_read_block(bdev_t *dev, void *buf, bnum_t block, uint count)
{
	LTRACEF("dev '%s', buf %p, block %d, count %u\n", dev->name, buf, block, count);
		
	DEBUG_ASSERT(dev->ref > 0);

	/* range check */
	if (block > dev->block_count)
		return 0;
	if (count == 0)
		return 0;
	if (block + count > dev->block_count)
		count = dev->block_count - block;

	return dev->read_block(dev, buf, block, count);
}

ssize_t bio_write(bdev_t *dev, const void *buf, off_t offset, size_t len)
{
	LTRACEF("dev '%s', buf %p, offset %lld, len %zd\n", dev->name, buf, offset, len);
		
	DEBUG_ASSERT(dev->ref > 0);

	/* range check */
	if (offset < 0)
		return -1;
	if (offset >= dev->size)
		return 0;
	if (len == 0)
		return 0;
	if (offset + len > dev->size)
		len = dev->size - offset;

	return dev->write(dev, buf, offset, len);
}

ssize_t bio_write_block(bdev_t *dev, const void *buf, bnum_t block, uint count)
{
	LTRACEF("dev '%s', buf %p, block %d, count %u\n", dev->name, buf, block, count);

	DEBUG_ASSERT(dev->ref > 0);

	/* range check */
	if (block > dev->block_count)
		return 0;
	if (count == 0)
		return 0;
	if (block + count > dev->block_count)
		count = dev->block_count - block;

	return dev->write_block(dev, buf, block, count);
}

ssize_t bio_erase(bdev_t *dev, off_t offset, size_t len)
{
	LTRACEF("dev '%s', offset %lld, len %zd\n", dev->name, offset, len);
		
	DEBUG_ASSERT(dev->ref > 0);

	/* range check */
	if (offset < 0)
		return -1;
	if (offset >= dev->size)
		return 0;
	if (len == 0)
		return 0;
	if (offset + len > dev->size)
		len = dev->size - offset;

	return dev->erase(dev, offset, len);
}

int bio_ioctl(bdev_t *dev, int request, void *argp)
{
	LTRACEF("dev '%s', request %08x, argp %p\n", dev->name, request, argp);

	if (dev->ioctl == NULL) {
		return ERR_NOT_SUPPORTED;
	} else {
		return dev->ioctl(dev, request, argp);
	}
}

void bio_initialize_bdev(bdev_t *dev, const char *name, size_t block_size, bnum_t block_count)
{
	DEBUG_ASSERT(dev);
	DEBUG_ASSERT(name);
	DEBUG_ASSERT(block_size == 512); // XXX can only deal with 512 for now

	list_clear_node(&dev->node);
	dev->name = strdup(name);
	dev->block_size = block_size;
	dev->block_count = block_count;
	dev->size = (off_t)block_count * block_size;
	dev->ref = 0;

	/* set up the default hooks, the sub driver should override the block operations at least */
	dev->read = bio_default_read;
	dev->read_block = bio_default_read_block;
	dev->write = bio_default_write;
	dev->write_block = bio_default_write_block;
	dev->erase = bio_default_erase;
	dev->close = NULL;
}

void bio_register_device(bdev_t *dev)
{
	DEBUG_ASSERT(dev);

	LTRACEF(" '%s'\n", dev->name);

	bdev_inc_ref(dev);

	mutex_acquire(&bdevs->lock);
	list_add_head(&bdevs->list, &dev->node);
	mutex_release(&bdevs->lock);
}

void bio_unregister_device(bdev_t *dev)
{
	DEBUG_ASSERT(dev);

	LTRACEF(" '%s'\n", dev->name);

	// remove it from the list
	mutex_acquire(&bdevs->lock);
	list_delete(&dev->node);
	mutex_release(&bdevs->lock);

	bdev_dec_ref(dev); // remove the ref the list used to have
}

void bio_dump_devices(void)
{
	printf("block devices:\n");
	bdev_t *entry;
	mutex_acquire(&bdevs->lock);
	list_for_every_entry(&bdevs->list, entry, bdev_t, node) {
		printf("\t%s, size %lld, bsize %zd, ref %d\n", entry->name, entry->size, entry->block_size, entry->ref);
	}
	mutex_release(&bdevs->lock);
}

void bio_init(void)
{
	bdevs = malloc(sizeof(*bdevs));

	list_initialize(&bdevs->list);
	mutex_init(&bdevs->lock);
}

