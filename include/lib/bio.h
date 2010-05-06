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
#ifndef __LIB_BIO_H
#define __LIB_BIO_H

#include <sys/types.h>
#include <list.h>

typedef uint32_t bnum_t;

typedef struct bdev {
	struct list_node node;
	volatile int ref;

	/* info about the block device */
	char *name;
	off_t size;
	size_t block_size;
	bnum_t block_count;

	/* function pointers */
	ssize_t (*read)(struct bdev *, void *buf, off_t offset, size_t len);
	ssize_t (*read_block)(struct bdev *, void *buf, bnum_t block, uint count);
	ssize_t (*write)(struct bdev *, const void *buf, off_t offset, size_t len);
	ssize_t (*write_block)(struct bdev *, const void *buf, bnum_t block, uint count);
	ssize_t (*erase)(struct bdev *, off_t offset, size_t len);
	int (*ioctl)(struct bdev *, int request, void *argp);
	void (*close)(struct bdev *);
} bdev_t;

/* user api */
bdev_t *bio_open(const char *name);
void bio_close(bdev_t *dev);
ssize_t bio_read(bdev_t *dev, void *buf, off_t offset, size_t len);
ssize_t bio_read_block(bdev_t *dev, void *buf, bnum_t block, uint count);
ssize_t bio_write(bdev_t *dev, const void *buf, off_t offset, size_t len);
ssize_t bio_write_block(bdev_t *dev, const void *buf, bnum_t block, uint count);
ssize_t bio_erase(bdev_t *dev, off_t offset, size_t len);
int bio_ioctl(bdev_t *dev, int request, void *argp);

/* intialize the block device layer */
void bio_init(void);

/* register a block device */
void bio_register_device(bdev_t *dev);
void bio_unregister_device(bdev_t *dev);

/* used during bdev construction */
void bio_initialize_bdev(bdev_t *dev, const char *name, size_t block_size, bnum_t block_count);

/* debug stuff */
void bio_dump_devices(void);

/* subdevice support */
status_t bio_publish_subdevice(const char *parent_dev, const char *subdev, bnum_t startblock, size_t len);

/* memory based block device */
int create_membdev(const char *name, void *ptr, size_t len);

#endif

