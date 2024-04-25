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

#include <stdlib.h>
#include <string.h>
#include <lk/trace.h>
#include <lk/init.h>
#include <lk/list.h>
#include <lk/err.h>
#include <lib/fs.h>
#include <kernel/mutex.h>

#include "v9fs_priv.h"

#define LOCAL_TRACE 0

status_t path_to_wname(char *path, uint16_t *nwname,
                       const char *wname[P9_MAXWELEM])
{
    char *cptr, *ncptr;
    *nwname = 0;

    if (path[0] == '\0')
        return NO_ERROR;

    cptr = path[0] == '/' ? path + 1 : path;

    while ((ncptr = strchr(cptr, '/')) != NULL) {
        *ncptr = '\0';
        if (strlen(cptr) != 0)
            wname[(*nwname)++] = cptr;
        cptr = ncptr + 1;

        if (*nwname == P9_MAXWELEM)
            return ERR_BAD_PATH;
    }

    wname[(*nwname)++] = strlen(cptr) != 0? cptr : ".";

    return NO_ERROR;
}

uint32_t get_unused_fid(v9fs_t *v9fs)
{
    mutex_acquire(&v9fs->lock);
    uint32_t fid = v9fs->unused_fid++;
    mutex_release(&v9fs->lock);
    return fid;
}

void put_fid(v9fs_t *v9fs, uint32_t fid)
{
    virtio_9p_msg_t tclunk = {
        .msg_type = P9_TCLUNK,
        .tag = P9_TAG_DEFAULT,
        .msg.tclunk = { .fid = fid, }
    };
    virtio_9p_msg_t rclunk = {};

    ASSERT(virtio_9p_rpc(v9fs->dev, &tclunk, &rclunk) == NO_ERROR);
    ASSERT(rclunk.msg_type == P9_RCLUNK);

    virtio_9p_msg_destroy(&rclunk);
}

status_t v9fs_mount(bdev_t *dev, fscookie **cookie)
{
    status_t ret;

    LTRACEF("bdev (%p) cookie (%p)\n", dev, cookie);

    if (!dev) {
        return ERR_INVALID_ARGS;
    }

    v9fs_t *v9fs = calloc(1, sizeof(v9fs_t));

    if (!v9fs)
        return ERR_NO_MEMORY;

    // initialize v9fs structure
    v9fs->dev = virtio_9p_bdev_to_virtio_device(dev);
    v9fs->bdev = dev;
    v9fs->unused_fid = 0;
    list_initialize(&v9fs->files);
    list_initialize(&v9fs->dirs);
    mutex_init(&v9fs->lock);
    v9fs->root.fid = get_unused_fid(v9fs);

    LTRACEF("v9fs->root.fid: %u\n", v9fs->root.fid);
    // attach to the host
    virtio_9p_msg_t tatt = {
        .msg_type = P9_TATTACH,
        .tag = P9_TAG_DEFAULT,
        .msg.tattach = {
            .fid = v9fs->root.fid,
            .afid = P9_FID_NOFID,
            .uname = "root",
            .aname = V9P_MOUNT_ANAME,
            .n_uname = P9_UNAME_NONUNAME,
        }
    };
    virtio_9p_msg_t ratt = {};

    if ((ret = virtio_9p_rpc(v9fs->dev, &tatt, &ratt)) != NO_ERROR)
        goto err;

    v9fs->root.qid = ratt.msg.rattach.qid;

    virtio_9p_msg_destroy(&ratt);

    *cookie = (fscookie *)v9fs;

    return NO_ERROR;

err:
    LTRACEF("mount 9p dev (%s) failed: %d\n", dev->name, ret);

    free(v9fs);
    v9fs = NULL;
    return ret;
}

status_t v9fs_unmount(fscookie *cookie)
{
    v9fs_t *v9fs = (v9fs_t *)cookie;

    LTRACEF("v9fs (%p)\n", v9fs);

    if (v9fs)
        free(v9fs);

    return 0;
}

static const struct fs_api v9fs_api = {
    .format = NULL,
    .fs_stat = NULL,

    .mount = v9fs_mount,
    .unmount = v9fs_unmount,
    .open = v9fs_open_file,
    .create = v9fs_create_file,
    .remove = NULL,
    .truncate = NULL,
    .stat = v9fs_stat_file,
    .read = v9fs_read_file,
    .write = v9fs_write_file,
    .close = v9fs_close_file,

    .mkdir = v9fs_mkdir,
    .opendir = v9fs_open_dir,
    .readdir = v9fs_read_dir,
    .closedir = v9fs_close_dir,

    .file_ioctl = NULL,
};

STATIC_FS_IMPL(9p, &v9fs_api);
