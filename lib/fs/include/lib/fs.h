/*
 * Copyright (c) 2009-2015 Travis Geiselbrecht
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
#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <compiler.h>

#define FS_MAX_PATH_LEN 128
#define FS_MAX_FILE_LEN 64

__BEGIN_CDECLS;

// Generic FS ioctls
enum fs_ioctl_num {
    FS_IOCTL_NULL = 0,
    FS_IOCTL_GET_FILE_ADDR,
    FS_IOCTL_IS_LINEAR,         // If supported, determine if the underlying device is in linear mode.
};

struct file_stat {
    bool is_dir;
    uint64_t size;
    uint64_t capacity;
};

struct fs_stat {
    uint64_t free_space;
    uint64_t total_space;

    uint32_t free_inodes;
    uint32_t total_inodes;
};

struct dirent {
    char name[FS_MAX_FILE_LEN];
};

typedef struct filehandle filehandle;
typedef struct dirhandle dirhandle;


status_t fs_format_device(const char *fsname, const char *device, const void *args) __NONNULL((1));
status_t fs_mount(const char *path, const char *fs, const char *device) __NONNULL((1)) __NONNULL((2));
status_t fs_unmount(const char *path) __NONNULL();
status_t fs_file_ioctl(filehandle *handle, int request, void *argp) __NONNULL((1)) __NONNULL((3));

/* file api */
status_t fs_create_file(const char *path, filehandle **handle, uint64_t len) __NONNULL();
status_t fs_open_file(const char *path, filehandle **handle) __NONNULL();
status_t fs_remove_file(const char *path) __NONNULL();
ssize_t fs_read_file(filehandle *handle, void *buf, off_t offset, size_t len) __NONNULL();
ssize_t fs_write_file(filehandle *handle, const void *buf, off_t offset, size_t len) __NONNULL();
status_t fs_close_file(filehandle *handle) __NONNULL();
status_t fs_stat_file(filehandle *handle, struct file_stat *) __NONNULL((1));
status_t fs_truncate_file(filehandle *handle, uint64_t len) __NONNULL((1));

/* dir api */
status_t fs_make_dir(const char *path) __NONNULL();
status_t fs_open_dir(const char *path, dirhandle **handle) __NONNULL();
status_t fs_read_dir(dirhandle *handle, struct dirent *ent) __NONNULL();
status_t fs_close_dir(dirhandle *handle) __NONNULL();

status_t fs_stat_fs(const char *mountpoint, struct fs_stat *stat) __NONNULL((1)) __NONNULL((2));

/* convenience routines */
ssize_t fs_load_file(const char *path, void *ptr, size_t maxlen) __NONNULL();

/* walk through a path string, removing duplicate path separators, flattening . and .. references */
void fs_normalize_path(char *path) __NONNULL();

/* Remove any leading spaces or slashes */
const char *trim_name(const char *_name);

/* file system api */
typedef struct fscookie fscookie;
typedef struct filecookie filecookie;
typedef struct dircookie dircookie;
struct bdev;

struct fs_api {
    status_t (*format)(struct bdev *, const void *);
    status_t (*fs_stat)(fscookie *, struct fs_stat *);

    status_t (*mount)(struct bdev *, fscookie **);
    status_t (*unmount)(fscookie *);
    status_t (*open)(fscookie *, const char *, filecookie **);
    status_t (*create)(fscookie *, const char *, filecookie **, uint64_t);
    status_t (*remove)(fscookie *, const char *);
    status_t (*truncate)(filecookie *, uint64_t);
    status_t (*stat)(filecookie *, struct file_stat *);
    ssize_t (*read)(filecookie *, void *, off_t, size_t);
    ssize_t (*write)(filecookie *, const void *, off_t, size_t);
    status_t (*close)(filecookie *);

    status_t (*mkdir)(fscookie *, const char *);
    status_t (*opendir)(fscookie *, const char *, dircookie **) __NONNULL();
    status_t (*readdir)(dircookie *, struct dirent *) __NONNULL();
    status_t (*closedir)(dircookie *) __NONNULL();

    status_t (*file_ioctl)(filecookie *, int, void *);
};

struct fs_impl {
    const char *name;
    const struct fs_api *api;
};

/* define in your fs implementation to register your api with the fs layer */
#define STATIC_FS_IMPL(_name, _api) const struct fs_impl __fs_impl_##_name __ALIGNED(sizeof(void *)) __SECTION(".fs_impl") = \
    { .name = #_name, .api = _api }

__END_CDECLS
