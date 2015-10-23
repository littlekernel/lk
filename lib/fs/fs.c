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
#include <debug.h>
#include <trace.h>
#include <list.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <lib/fs.h>
#include <lib/bio.h>
#include <lk/init.h>

#define LOCAL_TRACE 0

struct fs_mount {
    struct list_node node;

    char *path;
    bdev_t *dev;
    fscookie cookie;
    int refs;
    const struct fs_api *api;
};

struct fs_file {
    filecookie cookie;
    struct fs_mount *mount;
};

struct fs {
    struct list_node node;
    const char *name;
    const struct fs_api *api;
};

static struct list_node mounts = LIST_INITIAL_VALUE(mounts);
static struct list_node fses = LIST_INITIAL_VALUE(fses);

#if 0
static struct fs_api types[] = {
#if WITH_LIB_FS_FAT32
    {
        .name = "fat32",
        .mount = fat32_mount,
        .unmount = fat32_unmount,
        .open = fat32_open_file,
        .create = fat32_create_file,
        .mkdir = fat32_make_dir,
        .stat = fat32_stat_file,
        .read = fat32_read_file,
        .write = fat32_write_file,
        .close = fat32_close_file,
    },
#endif
};
#endif

static void test_normalize(const char *in);
static struct fs_mount *find_mount(const char *path, const char **trimmed_path);

static void fs_init(uint level)
{
#if 0
    test_normalize("/");
    test_normalize("/test");
    test_normalize("/test/");
    test_normalize("test/");
    test_normalize("test");
    test_normalize("/test//");
    test_normalize("/test/foo");
    test_normalize("/test/foo/");
    test_normalize("/test/foo/bar");
    test_normalize("/test/foo/bar//");
    test_normalize("/test//foo/bar//");
    test_normalize("/test//./foo/bar//");
    test_normalize("/test//./.foo/bar//");
    test_normalize("/test//./..foo/bar//");
    test_normalize("/test//./../foo/bar//");
    test_normalize("/test/../foo");
    test_normalize("/test/bar/../foo");
    test_normalize("../foo");
    test_normalize("../foo/");
    test_normalize("/../foo");
    test_normalize("/../foo/");
    test_normalize("/../../foo");
    test_normalize("/bleh/../../foo");
    test_normalize("/bleh/bar/../../foo");
    test_normalize("/bleh/bar/../../foo/..");
    test_normalize("/bleh/bar/../../foo/../meh");
#endif
}

LK_INIT_HOOK(libfs, &fs_init, LK_INIT_LEVEL_THREADING);

status_t fs_register_type(const char *name, const struct fs_api *api)
{
    struct fs *fs;

    fs = malloc(sizeof(struct fs));
    if (!fs)
        return ERR_NO_MEMORY;

    fs->name = name;
    fs->api = api;

    list_add_head(&fses, &fs->node);

    return NO_ERROR;
}

static struct fs *find_fs(const char *name)
{
    struct fs *fs;
    list_for_every_entry(&fses, fs, struct fs, node) {
        if (!strcmp(name, fs->name))
            return fs;
    }
    return NULL;
}

static struct fs_mount *find_mount(const char *path, const char **trimmed_path)
{
    struct fs_mount *mount;
    size_t pathlen = strlen(path);

    list_for_every_entry(&mounts, mount, struct fs_mount, node) {
        size_t mountpathlen = strlen(mount->path);
        if (pathlen < mountpathlen)
            continue;

        LTRACEF("comparing %s with %s\n", path, mount->path);

        if (memcmp(path, mount->path, mountpathlen) == 0) {
            if (trimmed_path)
                *trimmed_path = &path[mountpathlen];

            return mount;
        }
    }

    return NULL;
}

static int mount(const char *path, const char *device, const struct fs_api *api)
{
    char temppath[512];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    if (temppath[0] != '/')
        return ERR_BAD_PATH;

    if (find_mount(temppath, NULL))
        return ERR_ALREADY_MOUNTED;

    bdev_t *dev = bio_open(device);
    if (!dev)
        return ERR_NOT_FOUND;

    fscookie cookie;
    int err = api->mount(dev, &cookie);
    if (err < 0) {
        bio_close(dev);
        return err;
    }

    /* create the mount structure and add it to the list */
    struct fs_mount *mount = malloc(sizeof(struct fs_mount));
    mount->path = strdup(temppath);
    mount->dev = dev;
    mount->cookie = cookie;
    mount->refs = 1;
    mount->api = api;

    list_add_head(&mounts, &mount->node);

    return 0;
}

int fs_mount(const char *path, const char *fsname, const char *device)
{
    struct fs *fs = find_fs(fsname);
    if (!fs)
        return ERR_NOT_FOUND;

    return mount(path, device, fs->api);
}

static void put_mount(struct fs_mount *mount)
{
    if (!(--mount->refs)) {
        list_delete(&mount->node);
        mount->api->unmount(mount->cookie);
        free(mount->path);
        bio_close(mount->dev);
        free(mount);
    }
}

int fs_unmount(const char *path)
{
    char temppath[512];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    struct fs_mount *mount = find_mount(temppath, NULL);
    if (!mount)
        return ERR_NOT_FOUND;

    put_mount(mount);

    return 0;
}


int fs_open_file(const char *path, filecookie *fcookie)
{
    int err;
    char temppath[512];
    filecookie cookie;

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    LTRACEF("path %s temppath %s\n", path, temppath);

    const char *newpath;
    struct fs_mount *mount = find_mount(temppath, &newpath);
    if (!mount)
        return ERR_NOT_FOUND;

    LTRACEF("path %s temppath %s newpath %s\n", path, temppath, newpath);

    err = mount->api->open(mount->cookie, newpath, &cookie);
    if (err < 0)
        return err;

    struct fs_file *f = malloc(sizeof(*f));
    f->cookie = cookie;
    f->mount = mount;
    mount->refs++;
    *fcookie = f;

    return 0;
}

int fs_create_file(const char *path, filecookie *fcookie)
{
    int err;
    char temppath[512];
    filecookie cookie;

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    const char *newpath;
    struct fs_mount *mount = find_mount(temppath, &newpath);
    if (!mount)
        return ERR_NOT_FOUND;

    if (!mount->api->create)
        return ERR_NOT_SUPPORTED;

    err = mount->api->create(mount->cookie, newpath, &cookie);
    if (err < 0)
        return err;

    struct fs_file *f = malloc(sizeof(*f));
    f->cookie = cookie;
    f->mount = mount;
    mount->refs++;
    *fcookie = f;

    return 0;
}

int fs_make_dir(const char *path)
{
    char temppath[512];

    strlcpy(temppath, path, sizeof(temppath));
    fs_normalize_path(temppath);

    const char *newpath;
    struct fs_mount *mount = find_mount(temppath, &newpath);
    if (!mount)
        return ERR_NOT_FOUND;

    if (!mount->api->mkdir)
        return ERR_NOT_SUPPORTED;

    return mount->api->mkdir(mount->cookie, newpath);
}

int fs_read_file(filecookie fcookie, void *buf, off_t offset, size_t len)
{
    struct fs_file *f = fcookie;

    return f->mount->api->read(f->cookie, buf, offset, len);
}

int fs_write_file(filecookie fcookie, const void *buf, off_t offset, size_t len)
{
    struct fs_file *f = fcookie;

    if (!f->mount->api->write)
        return ERR_NOT_SUPPORTED;

    return f->mount->api->write(f->cookie, buf, offset, len);
}

int fs_close_file(filecookie fcookie)
{
    int err;
    struct fs_file *f = fcookie;

    err = f->mount->api->close(f->cookie);
    if (err < 0)
        return err;

    put_mount(f->mount);
    free(f);
    return 0;
}

int fs_stat_file(filecookie fcookie, struct file_stat *stat)
{
    struct fs_file *f = fcookie;

    return f->mount->api->stat(f->cookie, stat);
}

ssize_t fs_load_file(const char *path, void *ptr, size_t maxlen)
{
    int err;
    filecookie cookie;

    /* open the file */
    err = fs_open_file(path, &cookie);
    if (err < 0)
        return err;

    /* stat it for size, see how much we need to read */
    struct file_stat stat;
    fs_stat_file(cookie, &stat);

    err = fs_read_file(cookie, ptr, 0, MIN(maxlen, stat.size));

    fs_close_file(cookie);

    return err;
}

static void test_normalize(const char *in)
{
    char path[1024];

    strlcpy(path, in, sizeof(path));
    fs_normalize_path(path);
    printf("'%s' -> '%s'\n", in, path);
}

void fs_normalize_path(char *path)
{
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

    /* remove duplicate path seperators, flatten empty fields (only composed of .), backtrack fields with .., remove trailing slashes */
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

    /* dont end with trailing slashes */
    if (outpos > 0 && path[outpos - 1] == '/')
        outpos--;

    path[outpos++] = 0;
}

