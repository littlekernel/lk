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
#ifndef __LIB_FS_H
#define __LIB_FS_H

void fs_init(void);

struct file_stat {
	bool is_dir;
	off_t size;
};

typedef void *filecookie;
typedef void *fscookie;

int fs_mount(const char *path, const char *device);
int fs_unmount(const char *path);

/* file api */
int fs_open_file(const char *path, filecookie *fcookie);
int fs_read_file(filecookie fcookie, void *buf, off_t offset, size_t len);
int fs_close_file(filecookie fcookie);
int fs_stat_file(filecookie fcookie, struct file_stat *);

/* convenience routines */
ssize_t fs_load_file(const char *path, void *ptr, size_t maxlen);

/* walk through a path string, removing duplicate path seperators, flattening . and .. references */
void fs_normalize_path(char *path);

#endif

