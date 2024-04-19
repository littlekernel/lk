/*
 * Copyright (c) 2024, Google Inc. All rights reserved.
 * Author: codycswong@google.com (Cody Wong)
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
#include <dev/virtio/9p.h>
#include <lib/fs.h>
#include <lk/list.h>
#include <kernel/mutex.h>

typedef struct v9fs_fid {
    uint32_t fid;
    uint32_t iounit;
    virtio_9p_qid_t qid;
} v9fs_fid_t;

typedef struct v9fs {
    struct virtio_device *dev;
    bdev_t *bdev;

    uint32_t unused_fid;
    v9fs_fid_t root;
    mutex_t lock;

    struct list_node files;
    struct list_node dirs;
} v9fs_t;

#define V9FS_FILE_PAGE_BUFFER_SIZE (1 << 12)
#define V9FS_FILE_LOCK_TIMEOUT 3000

typedef struct v9fs_file {
    v9fs_t *v9fs;
    v9fs_fid_t fid;

    struct list_node node;
    mutex_t lock;

    struct fs_page_buffer {
        size_t size;
        off_t index;
        bool need_update;
        bool dirty;
        uint8_t data[V9FS_FILE_PAGE_BUFFER_SIZE];
    } pg_buf;
} v9fs_file_t;

typedef struct v9fs_dir {
    v9fs_t *v9fs;
    v9fs_fid_t fid;
    uint64_t offset;

    uint32_t head;
    uint32_t tail;
    uint8_t data[PAGE_SIZE];

    struct list_node node;
} v9fs_dir_t;

status_t v9fs_mount(bdev_t *dev, fscookie **cookie);
status_t v9fs_unmount(fscookie *cookie);
status_t v9fs_open_file(fscookie *cookie, const char *path,
                        filecookie **fcookie);
status_t v9fs_create_file(fscookie *cookie, const char *path,
                          filecookie **fcookie, uint64_t len);
ssize_t v9fs_read_file(filecookie *fcookie, void *buf, off_t offset,
                       size_t len);
ssize_t v9fs_write_file(filecookie *fcookie, const void *buf, off_t offset,
                        size_t len);
status_t v9fs_close_file(filecookie *fcookie);
status_t v9fs_stat_file(filecookie *fcookie, struct file_stat *stat);
status_t v9fs_open_dir(fscookie *cookie, const char *path, dircookie **dcookie);
status_t v9fs_mkdir(fscookie *cookie, const char *path);
status_t v9fs_read_dir(dircookie *dcookie, struct dirent *ent);
status_t v9fs_close_dir(dircookie *dcookie);

status_t path_to_wname(char *path, uint16_t *nwname, const char *wname[P9_MAXWELEM]) __NONNULL((1)) __NONNULL((2));
uint32_t get_unused_fid(v9fs_t *v9fs);
void put_fid(v9fs_t *v9fs, uint32_t fid);
