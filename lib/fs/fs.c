/*
 * Copyright (c) 2009-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/fs.h>

#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/list.h>
#include <lk/err.h>
#include <string.h>
#include <stdlib.h>
#include <lib/bio.h>
#include <lk/init.h>
#include <kernel/mutex.h>

#define LOCAL_TRACE 0

struct fs_mount {
    struct list_node node;

    char *path;
    size_t pathlen; // save the strlen of path above to help with path matching
    bdev_t *dev;
    fscookie *cookie;
    int ref;
    const struct fs_impl *fs;
    const struct fs_api *api;
};

struct filehandle {
    filecookie *cookie;
    struct fs_mount *mount;
};

struct dirhandle {
    dircookie *cookie;
    struct fs_mount *mount;
};

static mutex_t mount_lock = MUTEX_INITIAL_VALUE(mount_lock);
static struct list_node mounts = LIST_INITIAL_VALUE(mounts);
static struct list_node fses = LIST_INITIAL_VALUE(fses);

// defined by the linker, wrapping all structs in the "fs_impl" section
extern const struct fs_impl __start_fs_impl __WEAK;
extern const struct fs_impl __stop_fs_impl __WEAK;

static const struct fs_impl *find_fs(const char *name) {
    for (const struct fs_impl *fs = &__start_fs_impl; fs != &__stop_fs_impl; fs++) {
        if (!strcmp(name, fs->name))
            return fs;
    }
    return NULL;
}

void fs_dump_list(void) {
    for (const struct fs_impl *fs = &__start_fs_impl; fs != &__stop_fs_impl; fs++) {
        puts(fs->name);
    }
}

void fs_dump_mounts(void) {
    printf("%-16s%s\n", "Filesystem", "Path");
    mutex_acquire(&mount_lock);
    struct fs_mount *mount;
    list_for_every_entry(&mounts, mount, struct fs_mount, node) {
        printf("%-16s%s\n", mount->fs->name, mount->path);
    }
    mutex_release(&mount_lock);
}

// find a mount structure based on the prefix of this path
// bump the ref to the mount structure before returning
static struct fs_mount *find_mount(const char *path, const char **trimmed_path) {
    // paths must be absolute and start with /
    if (path[0] != '/') {
        return NULL;
    }
    size_t pathlen = strlen(path);

    mutex_acquire(&mount_lock);
    struct fs_mount *mount;
    list_for_every_entry(&mounts, mount, struct fs_mount, node) {
        // if the path is shorter than this mount point, no point continuing
        if (pathlen < mount->pathlen) {
            continue;
        }

        LTRACEF("comparing %s with %s\n", path, mount->path);

        if (memcmp(path, mount->path, mount->pathlen) == 0) {
            // If we got a match, make sure the next element in the path is
            // a path separator or the end of the string. This keeps from
            // matching /foo2 with /foo, but /foo/bar would match correctly.
            if (path[mount->pathlen] != '/' && path[mount->pathlen] != 0) {
                continue;
            }

            // we got a match, skip forward to the next element
            if (trimmed_path) {
                *trimmed_path = &path[mount->pathlen];
                // if we matched against the end of the path, at least return
                // a "/".
                // TODO: decide if this is necessary
                if (*trimmed_path[0] == 0) {
                    *trimmed_path = "/";
                }
            }

            mount->ref++;

            mutex_release(&mount_lock);
            return mount;
        }
    }

    mutex_release(&mount_lock);
    return NULL;
}

// decrement the ref to the mount structure, which may
// cause an unmount operation
static void put_mount(struct fs_mount *mount) {
    mutex_acquire(&mount_lock);
    if ((--mount->ref) == 0) {
        LTRACEF("last ref, unmounting fs at '%s'\n", mount->path);

        list_delete(&mount->node);
        mount->api->unmount(mount->cookie);
        free(mount->path);
        if (mount->dev)
            bio_close(mount->dev);
        free(mount);
    }
    mutex_release(&mount_lock);
}

static status_t mount(const char *path, const char *device, const struct fs_impl *fs) {
    struct fs_mount *mount;
    const struct fs_api *api = fs->api;
    char temppath[FS_MAX_PATH_LEN];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    if (temppath[0] != '/')
        return ERR_BAD_PATH;

    /* see if there's already something at this path, abort if there is */
    mount = find_mount(temppath, NULL);
    if (mount) {
        put_mount(mount);
        return ERR_ALREADY_MOUNTED;
    }

    /* open a bio device if the string is nonnull */
    bdev_t *dev = NULL;
    if (device && device[0] != '\0') {
        dev = bio_open(device);
        if (!dev)
            return ERR_NOT_FOUND;
    }

    /* call into the fs implementation */
    fscookie *cookie;
    status_t err = api->mount(dev, &cookie);
    if (err < 0) {
        if (dev) bio_close(dev);
        return err;
    }

    /* create the mount structure and add it to the list */
    mount = malloc(sizeof(struct fs_mount));
    if (!mount) {
        if (dev) bio_close(dev);
        return ERR_NO_MEMORY;
    }
    mount->path = strdup(temppath);
    if (!mount->path) {
        if (dev) bio_close(dev);
        free(mount);
        return ERR_NO_MEMORY;
    }
    mount->pathlen = strlen(mount->path);
    mount->dev = dev;
    mount->cookie = cookie;
    mount->ref = 1;
    mount->fs = fs;
    mount->api = api;

    list_add_head(&mounts, &mount->node);

    return 0;

}

status_t fs_format_device(const char *fsname, const char *device, const void *args) {
    const struct fs_impl *fs = find_fs(fsname);
    if (!fs) {
        return ERR_NOT_FOUND;
    }

    if (fs->api->format == NULL) {
        return ERR_NOT_SUPPORTED;
    }

    bdev_t *dev = NULL;
    if (device && device[0] != '\0') {
        dev = bio_open(device);
        if (!dev)
            return ERR_NOT_FOUND;
    }

    return fs->api->format(dev, args);
}

status_t fs_mount(const char *path, const char *fsname, const char *device) {
    const struct fs_impl *fs = find_fs(fsname);
    if (!fs)
        return ERR_NOT_FOUND;

    return mount(path, device, fs);
}

status_t fs_unmount(const char *path) {
    char temppath[FS_MAX_PATH_LEN];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    struct fs_mount *mount = find_mount(temppath, NULL);
    if (!mount)
        return ERR_NOT_FOUND;

    // return the ref that find_mount added and one extra
    put_mount(mount);
    put_mount(mount);

    return 0;
}

status_t fs_open_file(const char *path, filehandle **handle) {
    char temppath[FS_MAX_PATH_LEN];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    LTRACEF("path %s temppath %s\n", path, temppath);

    const char *newpath;
    struct fs_mount *mount = find_mount(temppath, &newpath);
    if (!mount)
        return ERR_NOT_FOUND;

    LTRACEF("path %s temppath %s newpath %s\n", path, temppath, newpath);

    filecookie *cookie;
    status_t err = mount->api->open(mount->cookie, newpath, &cookie);
    if (err < 0) {
        put_mount(mount);
        return err;
    }

    filehandle *f = malloc(sizeof(*f));
    f->cookie = cookie;
    f->mount = mount;
    *handle = f;

    return 0;
}

status_t fs_file_ioctl(filehandle *handle, int request, void *argp) {
    LTRACEF("filehandle %p, request %d, argp, %p\n", handle, request, argp);

    if (unlikely(!handle || !handle->mount ||
                 !handle->mount->api || !handle->mount->api->file_ioctl)) {
        return ERR_INVALID_ARGS;
    }

    return handle->mount->api->file_ioctl(handle->cookie, request, argp);
}

status_t fs_create_file(const char *path, filehandle **handle, uint64_t len) {
    char temppath[FS_MAX_PATH_LEN];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    const char *newpath;
    struct fs_mount *mount = find_mount(temppath, &newpath);
    if (!mount)
        return ERR_NOT_FOUND;

    if (!mount->api->create) {
        put_mount(mount);
        return ERR_NOT_SUPPORTED;
    }

    filecookie *cookie;
    status_t err = mount->api->create(mount->cookie, newpath, &cookie, len);
    if (err < 0) {
        put_mount(mount);
        return err;
    }

    filehandle *f = malloc(sizeof(*f));
    if (!f) {
        put_mount(mount);
        return err;
    }
    f->cookie = cookie;
    f->mount = mount;
    *handle = f;

    return 0;
}

status_t fs_truncate_file(filehandle *handle, uint64_t len) {
    LTRACEF("filehandle %p, length %llu\n", handle, len);

    if (unlikely(!handle))
        return ERR_INVALID_ARGS;

    return handle->mount->api->truncate(handle->cookie, len);
}

status_t fs_remove_file(const char *path) {
    char temppath[FS_MAX_PATH_LEN];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    const char *newpath;
    struct fs_mount *mount = find_mount(temppath, &newpath);
    if (!mount)
        return ERR_NOT_FOUND;

    if (!mount->api->remove) {
        put_mount(mount);
        return ERR_NOT_SUPPORTED;
    }

    status_t err = mount->api->remove(mount->cookie, newpath);

    put_mount(mount);

    return err;
}

ssize_t fs_read_file(filehandle *handle, void *buf, off_t offset, size_t len) {
    return handle->mount->api->read(handle->cookie, buf, offset, len);
}

ssize_t fs_write_file(filehandle *handle, const void *buf, off_t offset, size_t len) {
    if (!handle->mount->api->write)
        return ERR_NOT_SUPPORTED;

    return handle->mount->api->write(handle->cookie, buf, offset, len);
}

status_t fs_close_file(filehandle *handle) {
    status_t err = handle->mount->api->close(handle->cookie);
    if (err < 0)
        return err;

    put_mount(handle->mount);
    free(handle);
    return 0;
}

status_t fs_stat_file(filehandle *handle, struct file_stat *stat) {
    return handle->mount->api->stat(handle->cookie, stat);
}

status_t fs_make_dir(const char *path) {
    char temppath[FS_MAX_PATH_LEN];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    const char *newpath;
    struct fs_mount *mount = find_mount(temppath, &newpath);
    if (!mount)
        return ERR_NOT_FOUND;

    if (!mount->api->mkdir) {
        put_mount(mount);
        return ERR_NOT_SUPPORTED;
    }

    status_t err = mount->api->mkdir(mount->cookie, newpath);

    put_mount(mount);

    return err;
}

status_t fs_open_dir(const char *path, dirhandle **handle) {
    char temppath[FS_MAX_PATH_LEN];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    LTRACEF("path %s temppath %s\n", path, temppath);

    const char *newpath;
    struct fs_mount *mount = find_mount(temppath, &newpath);
    if (!mount)
        return ERR_NOT_FOUND;

    LTRACEF("path %s temppath %s newpath %s\n", path, temppath, newpath);

    if (!mount->api->opendir) {
        put_mount(mount);
        return ERR_NOT_SUPPORTED;
    }

    dircookie *cookie;
    status_t err = mount->api->opendir(mount->cookie, newpath, &cookie);
    if (err < 0) {
        put_mount(mount);
        return err;
    }

    dirhandle *d = malloc(sizeof(*d));
    if (!d) {
        put_mount(mount);
        return ERR_NO_MEMORY;
    }
    d->cookie = cookie;
    d->mount = mount;
    *handle = d;

    return 0;
}

status_t fs_read_dir(dirhandle *handle, struct dirent *ent) {
    if (!handle->mount->api->readdir)
        return ERR_NOT_SUPPORTED;

    return handle->mount->api->readdir(handle->cookie, ent);
}

status_t fs_close_dir(dirhandle *handle) {
    if (!handle->mount->api->closedir)
        return ERR_NOT_SUPPORTED;

    status_t err = handle->mount->api->closedir(handle->cookie);
    if (err < 0)
        return err;

    put_mount(handle->mount);
    free(handle);
    return 0;
}

status_t fs_stat_fs(const char *mountpoint, struct fs_stat *stat) {
    LTRACEF("mountpoint %s stat %p\n", mountpoint, stat);

    if (!stat) {
        return ERR_INVALID_ARGS;
    }

    const char *newpath;
    struct fs_mount *mount = find_mount(mountpoint, &newpath);
    if (!mount) {
        return ERR_NOT_FOUND;
    }

    if (!mount->api->fs_stat) {
        put_mount(mount);
        return ERR_NOT_SUPPORTED;
    }

    status_t result = mount->api->fs_stat(mount->cookie, stat);

    put_mount(mount);

    return result;
}


ssize_t fs_load_file(const char *path, void *ptr, size_t maxlen) {
    filehandle *handle;

    /* open the file */
    status_t err = fs_open_file(path, &handle);
    if (err < 0)
        return err;

    /* stat it for size, see how much we need to read */
    struct file_stat stat;
    fs_stat_file(handle, &stat);

    ssize_t read_bytes = fs_read_file(handle, ptr, 0, MIN(maxlen, stat.size));

    fs_close_file(handle);

    return read_bytes;
}

const char *trim_name(const char *_name) {
    const char *name = &_name[0];
    // chew up leading spaces
    while (*name == ' ')
        name++;

    // chew up leading slashes
    while (*name == '/')
        name++;

    return name;
}


void fs_normalize_path(char *path) {
    int outpos;
    int pos;
    char c;
    bool done;
    enum {
        INITIAL,
        FIELD_START,
        IN_FIELD,
        SEP,
        SEEN_SEP,
        DOT,
        SEEN_DOT,
        DOTDOT,
        SEEN_DOTDOT,
    } state;

    state = INITIAL;
    pos = 0;
    outpos = 0;
    done = false;

    /* remove duplicate path separators, flatten empty fields (only composed of .), backtrack fields with .., remove trailing slashes */
    while (!done) {
        c = path[pos];
        switch (state) {
            case INITIAL:
                if (c == '/') {
                    state = SEP;
                } else if (c == '.') {
                    state = DOT;
                } else {
                    state = FIELD_START;
                }
                break;
            case FIELD_START:
                if (c == '.') {
                    state = DOT;
                } else if (c == 0) {
                    done = true;
                } else {
                    state = IN_FIELD;
                }
                break;
            case IN_FIELD:
                if (c == '/') {
                    state = SEP;
                } else if (c == 0) {
                    done = true;
                } else {
                    path[outpos++] = c;
                    pos++;
                }
                break;
            case SEP:
                pos++;
                path[outpos++] = '/';
                state = SEEN_SEP;
                break;
            case SEEN_SEP:
                if (c == '/') {
                    // eat it
                    pos++;
                } else if (c == 0) {
                    done = true;
                } else {
                    state = FIELD_START;
                }
                break;
            case DOT:
                pos++; // consume the dot
                state = SEEN_DOT;
                break;
            case SEEN_DOT:
                if (c == '.') {
                    // dotdot now
                    state = DOTDOT;
                } else if (c == '/') {
                    // a field composed entirely of a .
                    // consume the / and move directly to the SEEN_SEP state
                    pos++;
                    state = SEEN_SEP;
                } else if (c == 0) {
                    done = true;
                } else {
                    // a field prefixed with a .
                    // emit a . and move directly into the IN_FIELD state
                    path[outpos++] = '.';
                    state = IN_FIELD;
                }
                break;
            case DOTDOT:
                pos++; // consume the dot
                state = SEEN_DOTDOT;
                break;
            case SEEN_DOTDOT:
                if (c == '/' || c == 0) {
                    // a field composed entirely of '..'
                    // search back and consume a field we've already emitted
                    if (outpos > 0) {
                        // we have already consumed at least one field
                        outpos--;

                        // walk backwards until we find the next field boundary
                        while (outpos > 0) {
                            if (path[outpos - 1] == '/') {
                                break;
                            }
                            outpos--;
                        }
                    }
                    pos++;
                    state = SEEN_SEP;
                    if (c == 0)
                        done = true;
                } else {
                    // a field prefixed with ..
                    // emit the .. and move directly to the IN_FIELD state
                    path[outpos++] = '.';
                    path[outpos++] = '.';
                    state = IN_FIELD;
                }
                break;
        }
    }

    /* don't end with trailing slashes */
    if (outpos > 0 && path[outpos - 1] == '/')
        outpos--;

    path[outpos++] = 0;
}

