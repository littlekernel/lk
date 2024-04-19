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

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lk/err.h>
#include <lk/trace.h>

#include "v9fs_priv.h"

#define LOCAL_TRACE 0

status_t v9fs_open_file(fscookie *cookie, const char *path,
                        filecookie **fcookie)
{
    v9fs_t *v9fs = (v9fs_t *)cookie;
    char temppath[FS_MAX_PATH_LEN];
    uint32_t flags;
    int ret;

    LTRACEF("v9fs (%p) path (%s) fcookie (%p)\n", v9fs, path, fcookie);

    strlcpy(temppath, path, sizeof(temppath));

    /* create the file object */
    v9fs_file_t *file = calloc(1, sizeof(v9fs_file_t));

    if (!file) {
        ret = ERR_NO_MEMORY;
        goto err;
    }

    file->pg_buf.size = 0;
    file->pg_buf.index = 0;
    file->pg_buf.dirty = false;
    file->pg_buf.need_update = true;
    mutex_init(&file->lock);

    file->fid.fid = get_unused_fid(v9fs);

    virtio_9p_msg_t twalk = {
        .msg_type = P9_TWALK,
        .tag = P9_TAG_DEFAULT,
        .msg.twalk = {
            .fid = v9fs->root.fid, .newfid = file->fid.fid
        }
    };
    virtio_9p_msg_t rwalk = {};

    path_to_wname(temppath, &twalk.msg.twalk.nwname, twalk.msg.twalk.wname);

    if ((ret = virtio_9p_rpc(v9fs->dev, &twalk, &rwalk)) != NO_ERROR)
        goto err;

    if (rwalk.msg_type != P9_RWALK ||
            rwalk.msg.rwalk.nwqid != twalk.msg.twalk.nwname) {
        ret = ERR_NOT_FOUND;
        goto err;
    }

    // assume all file are opened as "w+"
    flags = O_RDWR;

    virtio_9p_msg_t tlopen = {
        .msg_type= P9_TLOPEN,
        .tag = P9_TAG_DEFAULT,
        .msg.tlopen = {
            .fid = file->fid.fid, .flags = flags,
        }
    };
    virtio_9p_msg_t rlopen = {};

    if ((ret = virtio_9p_rpc(v9fs->dev, &tlopen, &rlopen)) != NO_ERROR)
        goto des_rwalk;

    file->v9fs = v9fs;
    file->fid.qid = rlopen.msg.rlopen.qid;
    file->fid.iounit = rlopen.msg.rlopen.iounit;

    *fcookie = (filecookie *)file;
    list_add_tail(&v9fs->files, &file->node);

    virtio_9p_msg_destroy(&rlopen);
    virtio_9p_msg_destroy(&rwalk);

    return NO_ERROR;

des_rwalk:
    virtio_9p_msg_destroy(&rwalk);

err:
    LTRACEF("open file (%s) failed: %d\n", path, ret);
    free(file);
    return ret;
}

status_t v9fs_create_file(fscookie *cookie, const char *path,
                          filecookie **fcookie, uint64_t len)
{
    v9fs_t *v9fs = (v9fs_t *)cookie;
    char temppath[FS_MAX_PATH_LEN];
    char *filename;
    uint32_t flags, mode;
    status_t ret;

    LTRACEF("v9fs (%p) path (%s) fcookie (%p) len (%llu)\n", v9fs, path,
            fcookie, len);

    strlcpy(temppath, path, sizeof(temppath));

    /* create the file object */
    v9fs_file_t *file = calloc(1, sizeof(v9fs_file_t));

    if (!file) {
        ret = ERR_NO_MEMORY;
        goto err;
    }

    file->pg_buf.size = 0;
    file->pg_buf.index = 0;
    file->pg_buf.dirty = false;
    file->pg_buf.need_update = true;
    mutex_init(&file->lock);

    file->fid.fid = get_unused_fid(v9fs);

    virtio_9p_msg_t twalk = {
        .msg_type = P9_TWALK,
        .tag = P9_TAG_DEFAULT,
        .msg.twalk = {
            .fid = v9fs->root.fid, .newfid = file->fid.fid
        }
    };
    virtio_9p_msg_t rwalk = {};

    // separate the directory and the filename
    filename = strrchr(temppath, '/');
    if (!filename || filename == temppath) {      // create on the root dir
        filename = temppath;
        twalk.msg.twalk.nwname = 0;
    } else {                                      // create on a dir
        // parse the parent directory
        *filename++ = '\0';
        path_to_wname(temppath, &twalk.msg.twalk.nwname, twalk.msg.twalk.wname);
    }

    // walk to the parent directory
    if ((ret = virtio_9p_rpc(v9fs->dev, &twalk, &rwalk)) != NO_ERROR)
        goto err;

    if (rwalk.msg_type != P9_RWALK ||
            rwalk.msg.rwalk.nwqid != twalk.msg.twalk.nwname) {
        ret = ERR_NOT_DIR;
        goto err;
    }

    // assume the file is created as 0666
    mode = S_IRUSR | S_IWUSR |
           S_IRGRP | S_IWGRP |
           S_IROTH | S_IWOTH;
    // assume all file are opened as "w+"
    flags = O_RDWR | O_CREAT | O_TRUNC;

    virtio_9p_msg_t tlcreate = {
        .msg_type= P9_TLCREATE,
        .tag = P9_TAG_DEFAULT,
        .msg.tlcreate = {
            .fid = file->fid.fid, .flags = flags, .mode = mode,
            .name = filename,
        }
    };
    virtio_9p_msg_t rlcreate = {};

    if ((ret = virtio_9p_rpc(v9fs->dev, &tlcreate, &rlcreate)) != NO_ERROR)
        goto des_rwalk;

    file->v9fs = v9fs;
    file->fid.qid = rlcreate.msg.rlopen.qid;
    file->fid.iounit = rlcreate.msg.rlopen.iounit;

    *fcookie = (filecookie *)file;
    list_add_tail(&v9fs->files, &file->node);

    virtio_9p_msg_destroy(&rlcreate);
    virtio_9p_msg_destroy(&rwalk);

    return NO_ERROR;

des_rwalk:
    virtio_9p_msg_destroy(&rwalk);

err:
    LTRACEF("create file (%s) failed: %d\n", path, ret);
    free(file);
    return ret;
}

static ssize_t read_file_impl(v9fs_file_t *file, void *buf, off_t offset,
                              size_t len)
{
    status_t err = NO_ERROR;
    ssize_t rlen = 0;
    uint32_t readcount;

    while (len > 0) {
        virtio_9p_msg_t tread = {
            .msg_type= P9_TREAD,
            .tag = P9_TAG_DEFAULT,
            .msg.tread = {
                .fid = file->fid.fid, .offset = offset,
                .count = MIN(len, PAGE_SIZE)
            }
        };
        virtio_9p_msg_t rread = {};

        if ((err = virtio_9p_rpc(file->v9fs->dev, &tread, &rread)) != NO_ERROR)
            break;

        if (rread.msg_type != P9_RREAD) {
            err = ERR_IO;
            break;
        }

        readcount = rread.msg.rread.count;

        memcpy(&((uint8_t *)buf)[rlen], rread.msg.rread.data, readcount);

        offset += readcount;
        rlen += readcount;
        len -= readcount;

        virtio_9p_msg_destroy(&rread);

        // read to the end of the file
        if (rread.msg.rread.count == 0)
            break;
    }

    return err == NO_ERROR ? rlen : err;
}

static ssize_t write_file_impl(v9fs_file_t *file, const void *buf, off_t offset,
                        size_t len)
{
    status_t err = NO_ERROR;
    ssize_t wlen = 0;
    uint32_t writecount;
    const uint8_t *cpos = buf;

    while (len > 0) {
        virtio_9p_msg_t twrite = {
            .msg_type= P9_TWRITE,
            .tag = P9_TAG_DEFAULT,
            .msg.twrite = {
                .fid = file->fid.fid, .offset = offset, .data = cpos,
                .count = MIN(len, PAGE_SIZE)
            }
        };
        virtio_9p_msg_t rwrite = {};

        if ((err = virtio_9p_rpc(file->v9fs->dev, &twrite, &rwrite)) !=
                NO_ERROR)
            break;

        if (rwrite.msg_type != P9_RWRITE) {
            err = ERR_IO;
            break;
        }

        writecount = rwrite.msg.rwrite.count;

        offset += writecount;
        cpos += writecount;
        wlen += writecount;
        len -= writecount;

        virtio_9p_msg_destroy(&rwrite);
    }

    return err == NO_ERROR ? wlen : err;
}

#define fs_page_index(off) ((off) / V9FS_FILE_PAGE_BUFFER_SIZE)
#define fs_page_start_by_offset(off) \
    ROUNDDOWN((off), V9FS_FILE_PAGE_BUFFER_SIZE)
#define fs_page_start_by_index(idx) ((idx) * V9FS_FILE_PAGE_BUFFER_SIZE)

static inline int clamp(int val, int min, int max)
{
    const int t = (val < min) ? min : val;
    return (t > max) ? max : t;
}

static bool fs_valid_page(off_t offset, off_t size)
{
    // the access size must not be larger than the size of a page buffer
    if (size >= V9FS_FILE_PAGE_BUFFER_SIZE)
        return false;

    // the access range lies in a page
    return fs_page_index(offset) == fs_page_index(offset + size - 1);
}

static bool fs_page_hit(struct fs_page_buffer *buf, off_t offset)
{
    return fs_page_index(offset) == buf->index;
}

static void fs_page_update(v9fs_file_t *file, off_t offset)
{
    ssize_t rsize;

    if (fs_page_hit(&file->pg_buf, offset) &&
            !file->pg_buf.need_update) {
        // page hit and don't need to update
        return;
    }

    if (file->pg_buf.dirty) {
        write_file_impl(file, file->pg_buf.data,
                        fs_page_start_by_index(file->pg_buf.index),
                        file->pg_buf.size);
        file->pg_buf.dirty = false;
    }

    memset(file->pg_buf.data, 0, V9FS_FILE_PAGE_BUFFER_SIZE);
    rsize =
        read_file_impl(file, file->pg_buf.data, fs_page_start_by_offset(offset),
                       V9FS_FILE_PAGE_BUFFER_SIZE);
    file->pg_buf.size = rsize;
    file->pg_buf.index = fs_page_index(offset);
    file->pg_buf.need_update = false;
    file->pg_buf.dirty = false;
}

static ssize_t fs_page_read(v9fs_file_t *file, void *buf, off_t offset,
                            size_t size)
{
    ssize_t rsize;

    fs_page_update(file, offset);

    offset %= V9FS_FILE_PAGE_BUFFER_SIZE;
    rsize = clamp(size, 0, file->pg_buf.size - offset);
    memcpy(buf, file->pg_buf.data + offset, rsize);

    return rsize;
}

static ssize_t fs_page_write(v9fs_file_t *file, const void *buf, off_t offset,
                             size_t size)
{
    fs_page_update(file, offset);

    offset %= V9FS_FILE_PAGE_BUFFER_SIZE;
    memcpy(file->pg_buf.data + offset, buf, size);
    file->pg_buf.size = offset + size;
    file->pg_buf.dirty = true;

    return size;
}

ssize_t v9fs_read_file(filecookie *fcookie, void *buf, off_t offset, size_t len)
{
    v9fs_file_t *file = (v9fs_file_t *)fcookie;
    ssize_t rsize;
    status_t ret;

    LTRACEF("file (%p) buf (%p) offset (%lld) len (%zd)\n", file, buf,
            offset, len);

    if (!fcookie && !buf)
        return ERR_INVALID_ARGS;

    if ((ret = mutex_acquire_timeout(&file->lock, V9FS_FILE_LOCK_TIMEOUT)) !=
            NO_ERROR) {
        return ret;
    }

    if (fs_valid_page(offset, len)) {
        rsize = fs_page_read(file, buf, offset, len);
    } else {
        rsize = read_file_impl(file, buf, offset, len);
    }

    mutex_release(&file->lock);

    return rsize;
}

ssize_t v9fs_write_file(filecookie *fcookie, const void *buf, off_t offset,
                        size_t len)
{
    v9fs_file_t *file = (v9fs_file_t *)fcookie;
    ssize_t rsize;
    status_t ret;

    LTRACEF("file (%p) buf (%p) offset (%lld) len (%zd)\n", file, buf,
            offset, len);

    if (!fcookie && !buf)
        return ERR_INVALID_ARGS;

    if ((ret = mutex_acquire_timeout(&file->lock, V9FS_FILE_LOCK_TIMEOUT)) !=
            NO_ERROR) {
        return ret;
    }

    if (fs_valid_page(offset, len)) {
        rsize = fs_page_write(file, buf, offset, len);
    } else {
        rsize = write_file_impl(file, buf, offset, len);
    }

    mutex_release(&file->lock);

    return rsize;
}

status_t v9fs_close_file(filecookie *fcookie)
{
    v9fs_file_t *file = (v9fs_file_t *)fcookie;
    status_t ret;

    LTRACEF("file (%p)\n", file);

    if (!file)
        return NO_ERROR;

    if ((ret = mutex_acquire_timeout(&file->lock, V9FS_FILE_LOCK_TIMEOUT)) !=
            NO_ERROR) {
        return ret;
    }

    if (file->pg_buf.dirty) {
        // writeback the dirty page
        write_file_impl(file, file->pg_buf.data,
                        fs_page_start_by_index(file->pg_buf.index),
                        file->pg_buf.size);
        file->pg_buf.dirty = false;
    }

    put_fid(file->v9fs, file->fid.fid);
    list_delete(&file->node);

    mutex_release(&file->lock);

    free(file);

    return NO_ERROR;
}

status_t v9fs_stat_file(filecookie *fcookie, struct file_stat *stat)
{
    v9fs_file_t *file = (v9fs_file_t *)fcookie;
    status_t ret;

    LTRACEF("file (%p) stat (%p)\n", file, stat);

    if ((ret = mutex_acquire_timeout(&file->lock, V9FS_FILE_LOCK_TIMEOUT)) !=
            NO_ERROR) {
        return ret;
    }

    virtio_9p_msg_t tgatt = {
        .msg_type = P9_TGETATTR,
        .tag = P9_TAG_DEFAULT,
        .msg.tgetattr = {
            .fid = file->fid.fid, .request_mask = P9_GETATTR_BASIC,
        }
    };
    virtio_9p_msg_t rgatt = {};

    if ((ret = virtio_9p_rpc(file->v9fs->dev, &tgatt, &rgatt)) != NO_ERROR)
        return ret;
    if (rgatt.msg_type != P9_RGETATTR) {
        ret = ERR_BUSY;
        goto err;
    }

    stat->size = rgatt.msg.rgetattr.size;
    stat->is_dir = S_ISDIR(rgatt.msg.rgetattr.mode);

err:
    virtio_9p_msg_destroy(&rgatt);

    mutex_release(&file->lock);

    return ret;
}
