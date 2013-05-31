/*
 * Copyright (c) 2013 Corey Tabaka
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
#ifndef __DEV_CLASS_BLOCK_H
#define __DEV_CLASS_BLOCK_H

#include <compiler.h>
#include <dev/driver.h>

/* block interface */
struct block_ops {
	struct driver_ops std;

	ssize_t (*get_block_size)(struct device *dev);
	ssize_t (*get_block_count)(struct device *dev);

	ssize_t (*write)(struct device *dev, off_t offset, const void *buf, size_t count);
	ssize_t (*read)(struct device *dev, off_t offset, void *buf, size_t count);
	
	status_t (*flush)(struct device *dev);
};

__BEGIN_CDECLS

ssize_t class_block_get_size(struct device *dev);
ssize_t class_block_get_count(struct device *dev);
ssize_t class_block_write(struct device *dev, off_t offset, const void *buf, size_t count);
ssize_t class_block_read(struct device *dev, off_t offset, void *buf, size_t count);
status_t class_block_flush(struct device *dev);

__END_CDECLS

#endif

