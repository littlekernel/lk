/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <endian.h>
#include <lib/bio.h>
#include <lib/fs.h>
#include <lib/fs/rofs.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/pow2.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

// A read-only file system over a pre-built image (the format is in lib/fs/rofs.h). Mount reads
// the whole index into memory, puts it in host byte order and checks every entry once, so
// lookups and directory listings never touch the device and never trust it; only file reads
// do. Nothing changes after mount, so there is no lock.
//
// A vnode's priv is the entry it stands for plus one, and null for the root, which has no
// entry; its id is the same number, the root's being one past the last entry.

typedef struct rofs {
    bdev_t *dev;
    uint64_t image_size;
    uint32_t entry_count;
    uint32_t root_count;
    uint64_t index_size;
    struct rofs_entry *entries; // the index, entries first, then the names
} rofs_t;

struct dircookie {
    const rofs_t *fs;
    uint32_t next; // the next entry to report
    uint32_t end;  // one past the run's last
};

static const char *entry_name(const rofs_t *fs, const struct rofs_entry *e) {
    return (const char *)fs->entries + e->name_offset;
}

// the entry a vnode stands for, null for the root
static const struct rofs_entry *vnode_entry(const struct fs_vnode *vn) {
    const rofs_t *fs = (const rofs_t *)vn->cookie;
    const uintptr_t n = (uintptr_t)vn->priv;
    return n ? &fs->entries[n - 1] : NULL;
}

// a directory vnode's run of entries; false for a file
static bool dir_run(const struct fs_vnode *vn, uint32_t *first, uint32_t *count) {
    const struct rofs_entry *e = vnode_entry(vn);
    if (!e) {
        *first = 0;
        *count = ((const rofs_t *)vn->cookie)->root_count;
        return true;
    }
    if (e->type != ROFS_TYPE_DIR) {
        return false;
    }
    *first = (uint32_t)e->offset;
    *count = (uint32_t)e->size;
    return true;
}

static status_t entry_vnode(const rofs_t *fs, uint32_t i, struct fs_vnode **out) {
    const enum fs_vnode_type type =
        fs->entries[i].type == ROFS_TYPE_DIR ? FS_VNODE_DIR : FS_VNODE_FILE;
    return fs_vnode_create(i + 1, type, (void *)(uintptr_t)(i + 1), out);
}

static bool valid_name(const rofs_t *fs, const struct rofs_entry *e) {
    if (e->name_len == 0 || e->name_len > ROFS_NAME_MAX) {
        return false;
    }
    // the name and its NUL inside the index, after the entries
    const uint64_t names = (uint64_t)fs->entry_count * sizeof(struct rofs_entry);
    if (e->name_offset < names || (uint64_t)e->name_offset + e->name_len >= fs->index_size) {
        return false;
    }
    const char *name = entry_name(fs, e);
    if (name[e->name_len] != '\0' || memchr(name, '\0', e->name_len) ||
        memchr(name, '/', e->name_len)) {
        return false;
    }
    return strcmp(name, ".") && strcmp(name, "..");
}

// every name in the run [first, first + count) after the one before it
static bool sorted_run(const rofs_t *fs, uint32_t first, uint32_t count) {
    for (uint32_t i = first + 1; i < first + count; i++) {
        if (strcmp(entry_name(fs, &fs->entries[i - 1]), entry_name(fs, &fs->entries[i])) >= 0) {
            return false;
        }
    }
    return true;
}

// Entry i, with *next the entry the next directory's run starts at, which a directory's moves on.
static bool valid_entry(const rofs_t *fs, uint32_t align, uint32_t i, uint32_t *next) {
    const struct rofs_entry *e = &fs->entries[i];
    if (e->reserved != 0 || !valid_name(fs, e)) {
        return false;
    }
    switch (e->type) {
        case ROFS_TYPE_FILE:
            return (e->offset & (align - 1)) == 0 && e->offset <= fs->image_size &&
                   e->size <= fs->image_size - e->offset;
        case ROFS_TYPE_DIR:
            // the runs tile the index, each after its own entry, so the tree has no cycles
            if (e->offset != *next || e->offset <= i || e->size > fs->entry_count - e->offset) {
                return false;
            }
            *next += (uint32_t)e->size;
            return sorted_run(fs, (uint32_t)e->offset, (uint32_t)e->size);
        default:
            return false;
    }
}

// The header's fields, in host byte order, against the device and against each other.
static bool valid_header(struct rofs_header *h, const bdev_t *dev) {
    h->magic = LE32(h->magic);
    h->version = LE32(h->version);
    h->header_size = LE32(h->header_size);
    h->align = LE32(h->align);
    h->entry_count = LE32(h->entry_count);
    h->root_count = LE32(h->root_count);
    h->index_offset = LE64(h->index_offset);
    h->index_size = LE64(h->index_size);
    h->image_size = LE64(h->image_size);

    if (h->magic != ROFS_MAGIC || h->version != ROFS_VERSION ||
        h->header_size != sizeof(struct rofs_header)) {
        return false;
    }
    if (h->align == 0 || !ispow2(h->align)) {
        return false;
    }
    if (h->image_size > (uint64_t)dev->total_size || h->index_offset < sizeof(*h) ||
        (h->index_offset & 7) || h->index_offset > h->image_size ||
        h->index_size > h->image_size - h->index_offset) {
        return false;
    }
    // an empty image has no index at all
    if (h->entry_count == 0) {
        return h->index_size == 0 && h->root_count == 0;
    }
    // the entries, then the names, which name_offset reaches with 32 bits
    if (h->index_size <= (uint64_t)h->entry_count * sizeof(struct rofs_entry) ||
        h->index_size > UINT32_MAX) {
        return false;
    }
    return h->root_count <= h->entry_count;
}

static status_t rofs_mount(struct bdev *dev, enum fs_mount_options options, fscookie **cookie,
                           struct fs_vnode **root) {
    LTRACEF("dev %p, options %#x\n", dev, options);

    if (!dev) {
        return ERR_NOT_FOUND;
    }
    if (options & ~FS_MOUNT_OPTION_READ_ONLY) {
        return ERR_INVALID_ARGS;
    }

    struct rofs_header h;
    ssize_t n = bio_read(dev, &h, 0, sizeof(h));
    if (n < 0) {
        return (status_t)n;
    }
    if ((size_t)n != sizeof(h) || !valid_header(&h, dev)) {
        return ERR_NOT_VALID;
    }

    rofs_t *fs = calloc(1, sizeof(*fs));
    if (!fs) {
        return ERR_NO_MEMORY;
    }
    fs->dev = dev;
    fs->image_size = h.image_size;
    fs->entry_count = h.entry_count;
    fs->root_count = h.root_count;
    fs->index_size = h.index_size;

    status_t err = NO_ERROR;
    if (h.index_size) {
        // malloc's alignment covers the entries' 64 bit fields
        fs->entries = malloc((size_t)h.index_size);
        if (!fs->entries) {
            err = ERR_NO_MEMORY;
            goto fail;
        }
        n = bio_read(dev, fs->entries, (off_t)h.index_offset, (size_t)h.index_size);
        if (n < 0) {
            err = (status_t)n;
            goto fail;
        }
        if ((uint64_t)n != h.index_size) {
            err = ERR_NOT_VALID;
            goto fail;
        }
    }
    for (uint32_t i = 0; i < fs->entry_count; i++) {
        struct rofs_entry *e = &fs->entries[i];
        e->name_offset = LE32(e->name_offset);
        e->name_len = LE16(e->name_len);
        e->offset = LE64(e->offset);
        e->size = LE64(e->size);
    }
    if (!sorted_run(fs, 0, fs->root_count)) {
        err = ERR_NOT_VALID;
        goto fail;
    }
    uint32_t next = fs->root_count;
    for (uint32_t i = 0; i < fs->entry_count; i++) {
        if (!valid_entry(fs, h.align, i, &next)) {
            LTRACEF("entry %u is not valid\n", i);
            err = ERR_NOT_VALID;
            goto fail;
        }
    }
    // every entry outside the root in some directory
    if (next != fs->entry_count) {
        err = ERR_NOT_VALID;
        goto fail;
    }

    err = fs_vnode_create((uint64_t)fs->entry_count + 1, FS_VNODE_DIR, NULL, root);
    if (err < 0) {
        goto fail;
    }
    *cookie = (fscookie *)fs;
    return NO_ERROR;

fail:
    free(fs->entries);
    free(fs);
    return err;
}

static status_t rofs_unmount(fscookie *cookie) {
    rofs_t *fs = (rofs_t *)cookie;

    LTRACEF("cookie %p\n", cookie);

    free(fs->entries);
    free(fs);
    return NO_ERROR;
}

static status_t rofs_fs_stat(fscookie *cookie, struct fs_stat *stat) {
    const rofs_t *fs = (const rofs_t *)cookie;

    stat->total_space = fs->image_size;
    stat->free_space = 0;
    stat->total_inodes = fs->entry_count;
    stat->free_inodes = 0;
    return NO_ERROR;
}

static status_t rofs_lookup(struct fs_vnode *dir, const char *name, struct fs_vnode **out) {
    const rofs_t *fs = (const rofs_t *)dir->cookie;

    LTRACEF("dir %p name '%s'\n", dir->priv, name);

    uint32_t lo, count;
    if (!dir_run(dir, &lo, &count)) {
        return ERR_NOT_DIR;
    }
    uint32_t hi = lo + count;
    while (lo < hi) {
        const uint32_t mid = lo + (hi - lo) / 2;
        const int c = strcmp(name, entry_name(fs, &fs->entries[mid]));
        if (c == 0) {
            return entry_vnode(fs, mid, out);
        }
        if (c < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return ERR_NOT_FOUND;
}

static ssize_t rofs_read(struct fs_vnode *vn, void *buf, off_t off, size_t len) {
    const rofs_t *fs = (const rofs_t *)vn->cookie;
    const struct rofs_entry *e = vnode_entry(vn);

    LTRACEF("vnode %p offset %lld len %zu\n", vn->priv, off, len);

    if (!e || e->type != ROFS_TYPE_FILE) {
        return ERR_NOT_FILE;
    }
    if (off < 0) {
        return ERR_INVALID_ARGS;
    }
    if ((uint64_t)off >= e->size) {
        return 0;
    }
    if (len > e->size - (uint64_t)off) {
        len = (size_t)(e->size - (uint64_t)off);
    }
    return bio_read(fs->dev, buf, (off_t)(e->offset + (uint64_t)off), len);
}

static status_t rofs_stat(struct fs_vnode *vn, struct file_stat *stat) {
    const struct rofs_entry *e = vnode_entry(vn);

    stat->is_dir = !e || e->type == ROFS_TYPE_DIR;
    stat->size = stat->is_dir ? 0 : e->size;
    stat->capacity = stat->size;
    return NO_ERROR;
}

static status_t rofs_ioctl(struct fs_vnode *vn, int request, void *argp) {
    const rofs_t *fs = (const rofs_t *)vn->cookie;
    const struct rofs_entry *e = vnode_entry(vn);

    LTRACEF("request %d, argp %p\n", request, argp);

    if (!e || e->type != ROFS_TYPE_FILE) {
        return ERR_NOT_FILE;
    }
    switch (request) {
        case FS_IOCTL_GET_FILE_ADDR: {
            void *base;
            status_t err = bio_ioctl(fs->dev, BIO_IOCTL_GET_MAP_ADDR, &base);
            if (err < 0) {
                return err;
            }
            *(void **)argp = (uint8_t *)base + e->offset;
            return NO_ERROR;
        }
        case FS_IOCTL_IS_LINEAR: {
            // argp is a bool
            status_t err = bio_ioctl(fs->dev, BIO_IOCTL_IS_MAPPED, argp);
            return err < 0 ? err : NO_ERROR;
        }
        default:
            return ERR_NOT_SUPPORTED;
    }
}

static status_t rofs_opendir(struct fs_vnode *vn, dircookie **dcookie) {
    uint32_t first, count;
    if (!dir_run(vn, &first, &count)) {
        return ERR_NOT_DIR;
    }
    struct dircookie *dc = malloc(sizeof(*dc));
    if (!dc) {
        return ERR_NO_MEMORY;
    }
    dc->fs = (const rofs_t *)vn->cookie;
    dc->next = first;
    dc->end = first + count;
    *dcookie = dc;
    return NO_ERROR;
}

// the layer keeps the mount until every directory handle is closed
static status_t rofs_readdir(dircookie *dcookie, struct dirent *ent) {
    struct dircookie *dc = dcookie;

    if (dc->next >= dc->end) {
        return ERR_NOT_FOUND;
    }
    strlcpy(ent->name, entry_name(dc->fs, &dc->fs->entries[dc->next]), sizeof(ent->name));
    dc->next++;
    return NO_ERROR;
}

static status_t rofs_closedir(dircookie *dcookie) {
    free(dcookie);
    return NO_ERROR;
}

static const struct fs_api rofs_api = {
    .mount = rofs_mount,
    .unmount = rofs_unmount,
    .fs_stat = rofs_fs_stat,

    .lookup = rofs_lookup,

    .read = rofs_read,
    .stat = rofs_stat,
    .ioctl = rofs_ioctl,

    .opendir = rofs_opendir,
    .readdir = rofs_readdir,
    .closedir = rofs_closedir,
};

STATIC_FS_IMPL(rofs, &rofs_api);
