/*
 * Copyright (c) 2015 Travis Geiselbrecht
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

#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <err.h>
#include <trace.h>
#include <list.h>
#include <lk/init.h>
#include <lib/fs.h>
#include <kernel/mutex.h>

#define LOCAL_TRACE 0

typedef struct {
    struct list_node files;
    struct list_node dcookies;

    mutex_t lock;
} memfs_t;

typedef struct {
    struct list_node node;
    memfs_t *fs;

    // name
    char *name;

    // main data area
    uint8_t *ptr;
    size_t len;
} memfs_file_t;

struct dircookie {
    struct list_node node;
    memfs_t *fs;

    // next entry that will be returned
    memfs_file_t *next_file;
};

static memfs_file_t *find_file(memfs_t *mem, const char *name)
{
    memfs_file_t *file;
    list_for_every_entry(&mem->files, file, memfs_file_t, node) {
        if (!strcmp(name, file->name))
            return file;
    }

    return NULL;
}

static status_t memfs_mount(struct bdev *dev, fscookie **cookie)
{
    LTRACEF("dev %p, cookie %p\n", dev, cookie);

    memfs_t *mem = malloc(sizeof(*mem));
    if (!mem)
        return ERR_NO_MEMORY;

    list_initialize(&mem->files);
    list_initialize(&mem->dcookies);
    mutex_init(&mem->lock);

    *cookie = (fscookie *)mem;

    return NO_ERROR;
}

static void free_file(memfs_file_t *file)
{
    free(file->ptr);
    free(file->name);
    free(file);
}

static status_t memfs_unmount(fscookie *cookie)
{
    LTRACEF("cookie %p\n", cookie);

    memfs_t *mem = (memfs_t *)cookie;

    mutex_acquire(&mem->lock);

    // free all the files
    memfs_file_t *file;
    while ((file = list_remove_head_type(&mem->files, memfs_file_t, node))) {
        free_file(file);
    }

    mutex_release(&mem->lock);

    free(mem);

    return NO_ERROR;
}

static status_t memfs_create(fscookie *cookie, const char *name, filecookie **fcookie, uint64_t len)
{
    status_t err;

    LTRACEF("cookie %p name '%s' filecookie %p len %llu\n", cookie, name, fcookie, len);

    memfs_t *mem = (memfs_t *)cookie;

    if (len >= ULONG_MAX)
        return ERR_NO_MEMORY;

    // make sure we strip out any leading /
    name = trim_name(name);

    // we can't handle directories right now, so fail if the file has a / in its name
    if (strchr(name, '/'))
        return ERR_NOT_SUPPORTED;

    mutex_acquire(&mem->lock);

    // see if the file already exists
    if (find_file(mem, name)) {
        err = ERR_ALREADY_EXISTS;
        goto out;
    }

    // allocate a new file
    memfs_file_t *file = malloc(sizeof(*file));
    if (!file) {
        err = ERR_NO_MEMORY;
        goto out;
    }

    // allocate the space for it
    file->ptr = calloc(1, len);
    if (!file->ptr) {
        free(file);
        err = ERR_NO_MEMORY;
        goto out;
    }
    file->len = len;

    // fill in some metadata and stuff it in the file list
    file->name = strdup(name);
    file->fs = mem;

    list_add_tail(&mem->files, &file->node);

    *fcookie = (filecookie *)file;

    err = NO_ERROR;

out:
    mutex_release(&mem->lock);

    return err;
}

static status_t memfs_open(fscookie *cookie, const char *name, filecookie **fcookie)
{
    LTRACEF("cookie %p name '%s' filecookie %p\n", cookie, name, fcookie);

    memfs_t *mem = (memfs_t *)cookie;

    // make sure we strip out any leading /
    name = trim_name(name);

    mutex_acquire(&mem->lock);
    memfs_file_t *file = find_file(mem, name);
    mutex_release(&mem->lock);

    if (!file)
        return ERR_NOT_FOUND;

    *fcookie = (filecookie *)file;

    return NO_ERROR;
}

static status_t memfs_remove(fscookie *cookie, const char *name)
{
    LTRACEF("cookie %p name '%s'\n", cookie, name);

    memfs_t *mem = (memfs_t *)cookie;

    // make sure we strip out any leading /
    name = trim_name(name);

    mutex_acquire(&mem->lock);
    memfs_file_t *file = find_file(mem, name);
    if (file)
        list_delete(&file->node);
    mutex_release(&mem->lock);

    if (!file)
        return ERR_NOT_FOUND;

    // XXX make sure there are no open file handles
    free_file(file);

    return NO_ERROR;
}

static status_t memfs_close(filecookie *fcookie)
{
    memfs_file_t *file = (memfs_file_t *)fcookie;

    LTRACEF("cookie %p name '%s'\n", fcookie, file->name);

    return NO_ERROR;
}

static ssize_t memfs_read(filecookie *fcookie, void *buf, off_t off, size_t len)
{
    LTRACEF("filecookie %p buf %p offset %lld len %zu\n", fcookie, buf, off, len);

    memfs_file_t *file = (memfs_file_t *)fcookie;

    if (off < 0)
        return ERR_INVALID_ARGS;

    mutex_acquire(&file->fs->lock);

    if (off >= file->len) {
        len = 0;
    } else if (off + len > file->len) {
        len = file->len - off;
    }

    // copy that floppy
    memcpy(buf, file->ptr + off, len);

    mutex_release(&file->fs->lock);

    return len;
}

static status_t memfs_truncate(filecookie *fcookie, uint64_t len)
{
    LTRACEF("filecookie %p, len %llu\n", fcookie, len);

    status_t rc = NO_ERROR;

    memfs_file_t *file = (memfs_file_t *)fcookie;

    mutex_acquire(&file->fs->lock);

    // Can't use truncate to grow a file.
    if (len > file->len) {
        rc = ERR_INVALID_ARGS;
        goto finish;
    }

    // NOTE: Don't allow allocations smaller than 1b. Although realloc(..., 0)
    // is okay, it may yield an invalid pointer (likely NULL) which might be
    // dereferenced elsewhere.
    void *ptr = realloc(file->ptr, len == 0 ? 1 : len);
    if (unlikely(ptr == NULL)) {
        rc = ERR_NO_MEMORY;
        goto finish;
    }

    file->len = len;
    file->ptr = ptr;

finish:
    mutex_release(&file->fs->lock);
    return rc;
}

static ssize_t memfs_write(filecookie *fcookie, const void *buf, off_t off, size_t len)
{
    LTRACEF("filecookie %p buf %p offset %lld len %zu\n", fcookie, buf, off, len);

    memfs_file_t *file = (memfs_file_t *)fcookie;

    if (off < 0)
        return ERR_INVALID_ARGS;

    mutex_acquire(&file->fs->lock);

    // see if this write will extend the file
    if (off + len > file->len) {
        void *ptr = realloc(file->ptr, off + len);
        if (!ptr) {
            mutex_release(&file->fs->lock);
            return ERR_NO_MEMORY;
        }

        file->ptr = ptr;
        file->len = off + len;
    }

    memcpy(file->ptr + off, buf, len);

    mutex_release(&file->fs->lock);

    return len;
}

static status_t memfs_stat(filecookie *fcookie, struct file_stat *stat)
{
    LTRACEF("filecookie %p stat %p\n", fcookie, stat);

    memfs_file_t *file = (memfs_file_t *)fcookie;

    mutex_acquire(&file->fs->lock);

    if (stat) {
        stat->is_dir = false;
        stat->size = file->len;
    }

    mutex_release(&file->fs->lock);

    return NO_ERROR;
}

static status_t memfs_opendir(fscookie *cookie, const char *name, dircookie **dcookie)
{
    LTRACEF("cookie %p name '%s' dircookie %p\n", cookie, name, dcookie);

    memfs_t *mem = (memfs_t *)cookie;

    // make sure we strip out any leading /
    name = trim_name(name);

    // at the moment, we only support opening "" (with / stripped)
    if (strcmp("", name))
        return ERR_NOT_FOUND;

    // allocate a dir cookie, point it at the first file, and stuff it in the dircookie jar
    dircookie *dir = malloc(sizeof(*dir));
    if (!dir)
        return ERR_NO_MEMORY;

    dir->fs = mem;

    mutex_acquire(&mem->lock);
    dir->next_file = list_peek_head_type(&mem->files, memfs_file_t, node);
    list_add_head(&mem->dcookies, &dir->node);
    mutex_release(&mem->lock);

    *dcookie = dir;

    return NO_ERROR;
}

static status_t memfs_readdir(dircookie *dcookie, struct dirent *ent)
{
    status_t err;

    LTRACEF("dircookie %p ent %p\n", dcookie, ent);

    if (!ent)
        return ERR_INVALID_ARGS;

    mutex_acquire(&dcookie->fs->lock);

    // return the next file in the list and bump the cursor
    if (dcookie->next_file) {
        strlcpy(ent->name, dcookie->next_file->name, sizeof(ent->name));
        dcookie->next_file = list_next_type(&dcookie->fs->files, &dcookie->next_file->node, memfs_file_t, node);
        err = NO_ERROR;
    } else {
        err = ERR_NOT_FOUND;
    }

    mutex_release(&dcookie->fs->lock);

    return err;
}

static status_t memfs_closedir(dircookie *dcookie)
{
    LTRACEF("dircookie %p\n", dcookie);

    // free the dircookie
    mutex_acquire(&dcookie->fs->lock);
    list_delete(&dcookie->node);
    mutex_release(&dcookie->fs->lock);

    free(dcookie);

    return NO_ERROR;
}

static const struct fs_api memfs_api = {
    .mount = memfs_mount,
    .unmount = memfs_unmount,

    .create = memfs_create,
    .open = memfs_open,
    .remove = memfs_remove,
    .close = memfs_close,
    .truncate = memfs_truncate,

    .read = memfs_read,
    .write = memfs_write,

    .stat = memfs_stat,

#if 0
    status_t (*mkdir)(fscookie *, const char *);
#endif
    .opendir = memfs_opendir,
    .readdir = memfs_readdir,
    .closedir = memfs_closedir,

};

STATIC_FS_IMPL(memfs, &memfs_api);
