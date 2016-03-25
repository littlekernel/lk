/*
 * Copyright (c) 2015 Steve White
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

#ifndef __FAT32_H
#define __FAT32_H

#include <lib/bio.h>
#include <lib/fs.h>

typedef void *fsfilecookie;

status_t fat32_mount(bdev_t *dev, fscookie **cookie);
status_t fat32_unmount(fscookie *cookie);

/* file api */
status_t fat32_open_file(fscookie *cookie, const char *path, filecookie **fcookie);
ssize_t fat32_read_file(filecookie *fcookie, void *buf, off_t offset, size_t len);
status_t fat32_close_file(filecookie *fcookie);
status_t fat32_stat_file(filecookie *fcookie, struct file_stat *stat);

#endif

