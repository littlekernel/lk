/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <endian.h>
#include <lib/bio.h>
#include <lib/cksum.h>
#include <lib/cmdline.h>
#include <lib/fs.h>
#include <lib/fs/rofs.h>
#include <lib/unittest.h>
#include <lk/err.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Two tiers, as for FAT and ext2. The RAM tests build images here, in memory, from a table of
// entries, mount them over a memory block device and check the tree, the reads and the
// ioctls, then refuse a corrupt copy of the good image for each rule the format has. The
// device test checks an image scripts/mkrofs.py made, on the device test.rofs.device names
// (scripts/run-rofs-tests.py attaches it, or links it in): its MANIFEST lists every directory
// and every file with its size and CRC-32, and the test checks each, read through the layer and,
// on a mapped device, in place at the image's alignment, and that every directory lists exactly
// what the manifest says it holds.

#define RAM_DEV "rofs-test"
#define RAM_MOUNT "/rofs-test"
#define RAM_ALIGN 512

// One entry of a test image, in index order: a directory's run is [first, first + count).
struct test_entry {
    const char *name;
    uint8_t type;
    uint32_t first;
    uint32_t count;
    size_t len; // a file's bytes, pattern(entry, offset)
};

// the root is the first five
static const struct test_entry tree[] = {
    {"a.txt", ROFS_TYPE_FILE, 0, 0, 12},
    {"bin", ROFS_TYPE_DIR, 5, 3, 0},
    {"empty", ROFS_TYPE_DIR, 8, 0, 0},
    {"sub", ROFS_TYPE_DIR, 8, 1, 0},
    {"z", ROFS_TYPE_FILE, 0, 0, 600},
    {"big", ROFS_TYPE_FILE, 0, 0, 9000},
    {"x", ROFS_TYPE_FILE, 0, 0, 0},
    {"y", ROFS_TYPE_FILE, 0, 0, 1},
    {"deep", ROFS_TYPE_FILE, 0, 0, 5},
};
#define TREE_ENTRIES (sizeof(tree) / sizeof(tree[0]))
#define TREE_ROOT 5

static uint8_t pattern(size_t entry, size_t off) {
    return (uint8_t)(entry * 31 + off * 7 + 1);
}

struct image {
    uint8_t *buf;
    size_t len;          // the device: the image rounded up to whole blocks
    uint64_t image_size; // the header's
    uint64_t index_offset;
    uint64_t *file_offset; // per entry
};

static void put32(uint8_t *p, uint32_t v) {
    v = LE32(v);
    memcpy(p, &v, sizeof(v));
}

static void put64(uint8_t *p, uint64_t v) {
    v = LE64(v);
    memcpy(p, &v, sizeof(v));
}

static void put16(uint8_t *p, uint16_t v) {
    v = LE16(v);
    memcpy(p, &v, sizeof(v));
}

static void image_free(struct image *img) {
    free(img->buf);
    free(img->file_offset);
    memset(img, 0, sizeof(*img));
}

// Lays out entries[0, n) the way mkrofs.py does: the header, the index right after it, then
// each file at a multiple of align.
static status_t image_build(struct image *img, const struct test_entry *entries, size_t n,
                            size_t root_count, uint32_t align) {
    memset(img, 0, sizeof(*img));
    img->file_offset = calloc(n ? n : 1, sizeof(uint64_t));
    if (!img->file_offset) {
        return ERR_NO_MEMORY;
    }
    size_t names = 0;
    for (size_t i = 0; i < n; i++) {
        names += strlen(entries[i].name) + 1;
    }
    const uint64_t index_offset = sizeof(struct rofs_header);
    const uint64_t index_size = n ? n * sizeof(struct rofs_entry) + names : 0;
    uint64_t end = ROUNDUP(index_offset + index_size, align);
    for (size_t i = 0; i < n; i++) {
        if (entries[i].type == ROFS_TYPE_FILE) {
            img->file_offset[i] = end;
            end = ROUNDUP(end + entries[i].len, align);
        }
    }
    img->image_size = end;
    img->index_offset = index_offset;
    img->len = ROUNDUP(end, 512);
    img->buf = memalign(align, img->len);
    if (!img->buf) {
        image_free(img);
        return ERR_NO_MEMORY;
    }
    memset(img->buf, 0, img->len);

    uint8_t *h = img->buf;
    put32(h + 0, ROFS_MAGIC);
    put32(h + 4, ROFS_VERSION);
    put32(h + 8, sizeof(struct rofs_header));
    put32(h + 12, align);
    put32(h + 16, (uint32_t)n);
    put32(h + 20, (uint32_t)root_count);
    put64(h + 24, index_offset);
    put64(h + 32, index_size);
    put64(h + 40, img->image_size);

    uint8_t *index = img->buf + index_offset;
    uint32_t name_offset = (uint32_t)(n * sizeof(struct rofs_entry));
    for (size_t i = 0; i < n; i++) {
        const struct test_entry *te = &entries[i];
        uint8_t *e = index + i * sizeof(struct rofs_entry);
        const size_t len = strlen(te->name);
        put32(e + 0, name_offset);
        put16(e + 4, (uint16_t)len);
        e[6] = te->type;
        if (te->type == ROFS_TYPE_DIR) {
            put64(e + 8, te->first);
            put64(e + 16, te->count);
        } else {
            put64(e + 8, img->file_offset[i]);
            put64(e + 16, te->len);
            for (size_t j = 0; j < te->len; j++) {
                img->buf[img->file_offset[i] + j] = pattern(i, j);
            }
        }
        memcpy(index + name_offset, te->name, len + 1);
        name_offset += (uint32_t)(len + 1);
    }
    return NO_ERROR;
}

// the memory device over a copy of buf[0, len), opened, so it outlives the test's references
static bdev_t *device_create(uint8_t **copy, const uint8_t *buf, size_t len) {
    *copy = memalign(RAM_ALIGN, len);
    if (!*copy) {
        return NULL;
    }
    memcpy(*copy, buf, len);
    if (create_membdev(RAM_DEV, *copy, len) < 0) {
        free(*copy);
        return NULL;
    }
    return bio_open(RAM_DEV);
}

static void device_destroy(bdev_t *dev, uint8_t *copy) {
    bio_close(dev);
    bio_unregister_device(dev);
    free(copy);
}

// the file at path, all of it, against the pattern of entry
static bool file_matches(const char *path, size_t entry, size_t len) {
    filehandle *f;
    if (fs_open_file(path, &f) < 0) {
        return false;
    }
    bool ok = true;
    struct file_stat st;
    if (fs_stat_file(f, &st) < 0 || st.is_dir || st.size != len) {
        ok = false;
    }
    uint8_t *buf = malloc(len + 16);
    if (!buf) {
        fs_close_file(f);
        return false;
    }
    // past the end too, which reads short
    if (ok && fs_read_file(f, buf, 0, len + 16) != (ssize_t)len) {
        ok = false;
    }
    for (size_t j = 0; ok && j < len; j++) {
        ok = buf[j] == pattern(entry, j);
    }
    free(buf);
    fs_close_file(f);
    return ok;
}

// fs_open_file's status for path, the file closed again
static status_t open_status(const char *path) {
    filehandle *f;
    status_t err = fs_open_file(path, &f);
    if (err == NO_ERROR) {
        fs_close_file(f);
    }
    return err;
}

static status_t opendir_status(const char *path) {
    dirhandle *d;
    status_t err = fs_open_dir(path, &d);
    if (err == NO_ERROR) {
        fs_close_dir(d);
    }
    return err;
}

// what the directory at path lists, against names[0, n) in order
static bool dir_lists(const char *path, const char *const *names, size_t n) {
    dirhandle *d;
    if (fs_open_dir(path, &d) < 0) {
        return false;
    }
    struct dirent ent;
    size_t i = 0;
    bool ok = true;
    status_t err;
    while ((err = fs_read_dir(d, &ent)) == NO_ERROR) {
        ok = ok && i < n && !strcmp(ent.name, names[i]);
        i++;
    }
    fs_close_dir(d);
    return ok && err == ERR_NOT_FOUND && i == n;
}

static bool test_rofs_tree(void) {
    BEGIN_TEST;

    struct image img;
    ASSERT_EQ(NO_ERROR, image_build(&img, tree, TREE_ENTRIES, TREE_ROOT, RAM_ALIGN), "build");
    uint8_t *mem;
    bdev_t *dev = device_create(&mem, img.buf, img.len);
    ASSERT_NONNULL(dev, "device");
    ASSERT_EQ(NO_ERROR, fs_mount(RAM_MOUNT, "rofs", RAM_DEV, FS_MOUNT_OPTION_READ_ONLY), "mount");

    // every file, whole
    EXPECT_TRUE(file_matches(RAM_MOUNT "/a.txt", 0, 12), "a.txt");
    EXPECT_TRUE(file_matches(RAM_MOUNT "/z", 4, 600), "z");
    EXPECT_TRUE(file_matches(RAM_MOUNT "/bin/big", 5, 9000), "bin/big");
    EXPECT_TRUE(file_matches(RAM_MOUNT "/bin/x", 6, 0), "bin/x");
    EXPECT_TRUE(file_matches(RAM_MOUNT "/bin/y", 7, 1), "bin/y");
    EXPECT_TRUE(file_matches(RAM_MOUNT "/sub/deep", 8, 5), "sub/deep");

    // the listings, in order, and an empty one
    static const char *const root[] = {"a.txt", "bin", "empty", "sub", "z"};
    static const char *const bin[] = {"big", "x", "y"};
    static const char *const sub[] = {"deep"};
    EXPECT_TRUE(dir_lists(RAM_MOUNT, root, 5), "root listing");
    EXPECT_TRUE(dir_lists(RAM_MOUNT "/bin", bin, 3), "bin listing");
    EXPECT_TRUE(dir_lists(RAM_MOUNT "/sub", sub, 1), "sub listing");
    EXPECT_TRUE(dir_lists(RAM_MOUNT "/empty", NULL, 0), "empty listing");

    // names that are not there, before, between and after the ones that are
    EXPECT_EQ(ERR_NOT_FOUND, open_status(RAM_MOUNT "/A"), "before");
    EXPECT_EQ(ERR_NOT_FOUND, open_status(RAM_MOUNT "/c"), "between");
    EXPECT_EQ(ERR_NOT_FOUND, open_status(RAM_MOUNT "/zz"), "after");
    EXPECT_EQ(ERR_NOT_FOUND, open_status(RAM_MOUNT "/bin/a"), "in a directory");
    EXPECT_EQ(ERR_NOT_FOUND, open_status(RAM_MOUNT "/empty/a"), "in an empty directory");
    EXPECT_NE(NO_ERROR, open_status(RAM_MOUNT "/a.txt/x"), "through a file");
    EXPECT_NE(NO_ERROR, opendir_status(RAM_MOUNT "/a.txt"), "a file as a directory");
    // the layer opens a directory as a file; the bytes are what rofs refuses
    filehandle *f;
    ASSERT_EQ(NO_ERROR, fs_open_file(RAM_MOUNT "/bin", &f), "a directory as a file");
    uint8_t byte;
    EXPECT_EQ(ERR_NOT_FILE, fs_read_file(f, &byte, 0, 1), "a directory's bytes");
    EXPECT_EQ(ERR_NOT_FILE, fs_file_ioctl(f, FS_IOCTL_GET_FILE_ADDR, &byte), "its address");
    struct file_stat st;
    EXPECT_EQ(NO_ERROR, fs_stat_file(f, &st), "stat a directory");
    EXPECT_TRUE(st.is_dir, "a directory");
    EXPECT_EQ(NO_ERROR, fs_close_file(f), "close");

    // read only
    EXPECT_NE(NO_ERROR, fs_create_file(RAM_MOUNT "/new", &f, 0), "create");
    EXPECT_NE(NO_ERROR, fs_make_dir(RAM_MOUNT "/newdir"), "mkdir");
    EXPECT_NE(NO_ERROR, fs_remove_file(RAM_MOUNT "/z"), "remove");
    EXPECT_NE(NO_ERROR, fs_remove_dir(RAM_MOUNT "/empty"), "rmdir");

    // reads at an offset, across the end and past it
    ASSERT_EQ(NO_ERROR, fs_open_file(RAM_MOUNT "/bin/big", &f), "open big");
    uint8_t *buf = malloc(1300);
    ASSERT_NONNULL(buf, "buffer");
    EXPECT_EQ(1234, fs_read_file(f, buf, 4001, 1234), "at an offset");
    bool same = true;
    for (size_t j = 0; j < 1234; j++) {
        same = same && buf[j] == pattern(5, 4001 + j);
    }
    EXPECT_TRUE(same, "offset contents");
    EXPECT_EQ(100, fs_read_file(f, buf, 8900, 1300), "across the end");
    EXPECT_EQ(0, fs_read_file(f, buf, 9000, 10), "at the end");
    EXPECT_EQ(0, fs_read_file(f, buf, 1 << 20, 10), "past the end");
    EXPECT_EQ(ERR_INVALID_ARGS, fs_read_file(f, buf, -1, 10), "negative offset");
    EXPECT_NE(NO_ERROR, fs_write_file(f, buf, 0, 1), "write");
    EXPECT_NE(NO_ERROR, fs_truncate_file(f, 0), "truncate");
    free(buf);

    // in place on a mapped device, at the image's alignment
    bool linear = false;
    EXPECT_EQ(NO_ERROR, fs_file_ioctl(f, FS_IOCTL_IS_LINEAR, &linear), "is linear");
    EXPECT_TRUE(linear, "linear");
    void *addr = NULL;
    EXPECT_EQ(NO_ERROR, fs_file_ioctl(f, FS_IOCTL_GET_FILE_ADDR, &addr), "file address");
    EXPECT_EQ((uintptr_t)mem + img.file_offset[5], (uintptr_t)addr, "address");
    EXPECT_EQ(0u, (uintptr_t)addr % RAM_ALIGN, "aligned");
    EXPECT_EQ(NO_ERROR, fs_close_file(f), "close");

    struct fs_stat fst;
    EXPECT_EQ(NO_ERROR, fs_stat_fs(RAM_MOUNT, &fst), "fs stat");
    EXPECT_EQ(img.image_size, fst.total_space, "total space");
    EXPECT_EQ(0u, fst.free_space, "free space");
    EXPECT_EQ(TREE_ENTRIES, fst.total_inodes, "entries");

    EXPECT_EQ(NO_ERROR, fs_unmount(RAM_MOUNT), "unmount");
    device_destroy(dev, mem);
    image_free(&img);

    END_TEST;
}

// Enough names in one directory that a lookup takes several steps of the binary search.
static bool test_rofs_wide(void) {
    BEGIN_TEST;

#define WIDE 40
    struct test_entry *entries = calloc(WIDE, sizeof(*entries));
    char *names = calloc(WIDE, 8);
    ASSERT_NONNULL(entries, "entries");
    ASSERT_NONNULL(names, "names");
    for (size_t i = 0; i < WIDE; i++) {
        snprintf(names + i * 8, 8, "f%02zu", i);
        entries[i] = (struct test_entry){names + i * 8, ROFS_TYPE_FILE, 0, 0, i};
    }
    struct image img;
    ASSERT_EQ(NO_ERROR, image_build(&img, entries, WIDE, WIDE, 16), "build");
    uint8_t *mem;
    bdev_t *dev = device_create(&mem, img.buf, img.len);
    ASSERT_NONNULL(dev, "device");
    ASSERT_EQ(NO_ERROR, fs_mount(RAM_MOUNT, "rofs", RAM_DEV, FS_MOUNT_OPTION_NONE), "mount");
    char path[32];
    for (size_t i = 0; i < WIDE; i++) {
        snprintf(path, sizeof(path), RAM_MOUNT "/f%02zu", i);
        EXPECT_TRUE(file_matches(path, i, i), "a wide directory's file");
    }
    EXPECT_EQ(ERR_NOT_FOUND, open_status(RAM_MOUNT "/f40"), "past the last");
    EXPECT_EQ(ERR_NOT_FOUND, open_status(RAM_MOUNT "/f"), "a prefix");
    EXPECT_EQ(NO_ERROR, fs_unmount(RAM_MOUNT), "unmount");
    device_destroy(dev, mem);
    image_free(&img);
    free(names);
    free(entries);
#undef WIDE

    END_TEST;
}

// an image of no entries at all: the root lists nothing
static bool test_rofs_empty(void) {
    BEGIN_TEST;

    struct image img;
    ASSERT_EQ(NO_ERROR, image_build(&img, NULL, 0, 0, 8), "build");
    uint8_t *mem;
    bdev_t *dev = device_create(&mem, img.buf, img.len);
    ASSERT_NONNULL(dev, "device");
    ASSERT_EQ(NO_ERROR, fs_mount(RAM_MOUNT, "rofs", RAM_DEV, FS_MOUNT_OPTION_NONE), "mount");
    EXPECT_TRUE(dir_lists(RAM_MOUNT, NULL, 0), "empty root");
    EXPECT_EQ(ERR_NOT_FOUND, open_status(RAM_MOUNT "/a"), "nothing there");
    EXPECT_EQ(NO_ERROR, fs_unmount(RAM_MOUNT), "unmount");
    device_destroy(dev, mem);
    image_free(&img);

    END_TEST;
}

// One rule of the format broken: width bytes at off (from the start of the image, or of the
// entry with ENTRY), replaced by value.
#define HEADER 0xffffffffu
struct corruption {
    const char *what;
    uint32_t entry;
    uint32_t off;
    uint8_t width;
    uint64_t value;
};

// fields of the header and of an entry
#define H_MAGIC 0
#define H_VERSION 4
#define H_SIZE 8
#define H_ALIGN 12
#define H_ENTRIES 16
#define H_ROOT 20
#define H_INDEX 24
#define H_INDEX_SIZE 32
#define H_IMAGE 40
#define E_NAME 0
#define E_NAME_LEN 4
#define E_TYPE 6
#define E_RESERVED 7
#define E_OFFSET 8
#define E_SIZE 16
#define NAMES (TREE_ENTRIES * sizeof(struct rofs_entry)) // the first name, "a.txt"

static const struct corruption corruptions[] = {
    {"magic", HEADER, H_MAGIC, 4, 0x53464f53},
    {"version", HEADER, H_VERSION, 4, 2},
    {"header size", HEADER, H_SIZE, 4, 40},
    {"zero align", HEADER, H_ALIGN, 4, 0},
    {"align not a power of two", HEADER, H_ALIGN, 4, 3},
    {"index inside the header", HEADER, H_INDEX, 8, 8},
    {"index not 8 aligned", HEADER, H_INDEX, 8, 52},
    {"index past the image", HEADER, H_INDEX_SIZE, 8, 1 << 20},
    {"no room for the names", HEADER, H_INDEX_SIZE, 8, NAMES},
    {"entries past the index", HEADER, H_ENTRIES, 4, 1000},
    {"root past the entries", HEADER, H_ROOT, 4, TREE_ENTRIES + 1},
    {"image past the device", HEADER, H_IMAGE, 8, 1 << 20},
    {"empty name", 0, E_NAME_LEN, 2, 0},
    {"name too long", 0, E_NAME_LEN, 2, ROFS_NAME_MAX + 1},
    {"name not terminated", 0, E_NAME_LEN, 2, 4},
    {"name inside the entries", 0, E_NAME, 4, 0},
    {"name past the index", 0, E_NAME, 4, 1 << 20},
    {"unknown type", 0, E_TYPE, 1, 3},
    {"reserved set", 0, E_RESERVED, 1, 1},
    {"file not aligned", 0, E_OFFSET, 8, RAM_ALIGN + 1},
    {"file past the image", 4, E_SIZE, 8, 1 << 20},
    {"file wrapping around", 4, E_SIZE, 8, UINT64_MAX},
    {"directory runs into itself", 1, E_OFFSET, 8, 1},
    {"directory runs backwards", 3, E_OFFSET, 8, 2},
    {"directory past the entries", 1, E_SIZE, 8, 100},
    {"directory starting past the entries", 2, E_OFFSET, 8, TREE_ENTRIES + 1},
    {"two directories sharing a run", 3, E_OFFSET, 8, 5},
    {"a run left out", 3, E_SIZE, 8, 0},
    // sub's run moved one back, onto "y": sorted, in bounds, out of order
    {"runs out of order", 3, E_OFFSET, 8, 7},
    {"a file as a directory", 0, E_TYPE, 1, ROFS_TYPE_DIR},
    // "bin" named "sub": the root out of order
    {"unsorted directory", 1, E_NAME, 4, NAMES + 16},
    // "y" named "x": bin holding a name twice
    {"duplicate name", 7, E_NAME, 4, NAMES + 26},
};

static void patch(uint8_t *p, uint8_t width, uint64_t value) {
    switch (width) {
        case 1:
            *p = (uint8_t)value;
            break;
        case 2:
            put16(p, (uint16_t)value);
            break;
        case 4:
            put32(p, (uint32_t)value);
            break;
        default:
            put64(p, value);
            break;
    }
}

static bool test_rofs_corrupt(void) {
    BEGIN_TEST;

    struct image img;
    ASSERT_EQ(NO_ERROR, image_build(&img, tree, TREE_ENTRIES, TREE_ROOT, RAM_ALIGN), "build");
    // where "sub" and "x" are among the names, which the table above counts on
    const char *names = (const char *)img.buf + img.index_offset + NAMES;
    ASSERT_EQ(0, strcmp(names + 16, "sub"), "name layout");
    ASSERT_EQ(0, strcmp(names + 26, "x"), "name layout");

    // the good image mounts, so each refusal below is its one broken rule's
    uint8_t *mem;
    bdev_t *dev = device_create(&mem, img.buf, img.len);
    ASSERT_NONNULL(dev, "device");
    EXPECT_EQ(NO_ERROR, fs_mount(RAM_MOUNT, "rofs", RAM_DEV, FS_MOUNT_OPTION_NONE), "good");
    EXPECT_EQ(NO_ERROR, fs_unmount(RAM_MOUNT), "unmount");
    device_destroy(dev, mem);

    for (size_t i = 0; i < sizeof(corruptions) / sizeof(corruptions[0]); i++) {
        const struct corruption *c = &corruptions[i];
        dev = device_create(&mem, img.buf, img.len);
        ASSERT_NONNULL(dev, "device");
        const size_t at = c->entry == HEADER
                              ? c->off
                              : img.index_offset + c->entry * sizeof(struct rofs_entry) + c->off;
        patch(mem + at, c->width, c->value);
        const status_t err = fs_mount(RAM_MOUNT, "rofs", RAM_DEV, FS_MOUNT_OPTION_NONE);
        if (err != ERR_NOT_VALID) {
            unittest_printf("\n  %s: mount returned %d\n", c->what, err);
            if (err == NO_ERROR) {
                fs_unmount(RAM_MOUNT);
            }
        }
        EXPECT_EQ(ERR_NOT_VALID, err, c->what);
        device_destroy(dev, mem);
    }

    // a name holding '/' or a NUL, or "." or "..", at the root's first entry
    static const struct {
        const char *what;
        const char name[6];
        uint16_t len;
    } bad_names[] = {
        {"a slash", "a/txt", 5},
        {"a NUL", "a\0txt", 5},
        {"dot", ".", 1},
        {"dot dot", "..", 2},
    };
    for (size_t i = 0; i < sizeof(bad_names) / sizeof(bad_names[0]); i++) {
        dev = device_create(&mem, img.buf, img.len);
        ASSERT_NONNULL(dev, "device");
        uint8_t *name = mem + img.index_offset + NAMES;
        memcpy(name, bad_names[i].name, bad_names[i].len);
        name[bad_names[i].len] = '\0';
        put16(mem + img.index_offset + E_NAME_LEN, bad_names[i].len);
        const status_t err = fs_mount(RAM_MOUNT, "rofs", RAM_DEV, FS_MOUNT_OPTION_NONE);
        if (err == NO_ERROR) {
            fs_unmount(RAM_MOUNT);
        }
        EXPECT_EQ(ERR_NOT_VALID, err, bad_names[i].what);
        device_destroy(dev, mem);
    }

    // shorter than its header says
    dev = device_create(&mem, img.buf, 512);
    ASSERT_NONNULL(dev, "device");
    EXPECT_EQ(ERR_NOT_VALID, fs_mount(RAM_MOUNT, "rofs", RAM_DEV, FS_MOUNT_OPTION_NONE),
              "truncated");
    device_destroy(dev, mem);

    // and no device at all
    EXPECT_NE(NO_ERROR, fs_mount(RAM_MOUNT, "rofs", NULL, FS_MOUNT_OPTION_NONE), "no device");

    image_free(&img);

    END_TEST;
}

// the device the host runner attached, or null; with test.rofs.required, a missing one fails
static const char *test_device(bool *required) {
    static char name[32];
    size_t len = 0;
    *required = false;
    cmdline_get_bool("test.rofs.required", required);
    if (cmdline_get_string("test.rofs.device", name, sizeof(name), &len) < 0 || len == 0) {
        return NULL;
    }
    return name;
}

#define DEV_MOUNT "/rofs-dev"

// the file at path: its size and CRC-32
static bool file_sum(const char *path, uint64_t *size, uint32_t *crc) {
    *size = 0;
    *crc = 0;
    filehandle *f;
    if (fs_open_file(path, &f) < 0) {
        return false;
    }
    struct file_stat st;
    uint8_t *buf = malloc(4096);
    bool ok = buf && fs_stat_file(f, &st) == NO_ERROR && !st.is_dir;
    unsigned long sum = crc32(0, NULL, 0);
    uint64_t off = 0;
    while (ok) {
        const ssize_t n = fs_read_file(f, buf, (off_t)off, 4096);
        if (n < 0) {
            ok = false;
        } else if (n == 0) {
            break;
        } else {
            sum = crc32(sum, buf, (unsigned int)n);
            off += (uint64_t)n;
        }
    }
    free(buf);
    fs_close_file(f);
    *size = off;
    *crc = (uint32_t)sum;
    return ok && off == st.size;
}

// On a mapped device, the file at path read in place: whether it is there, at a multiple of
// align, with the size and CRC-32 it should have. *mapped says whether the device is mapped.
static bool in_place_matches(const char *path, uint32_t align, uint64_t size, uint32_t crc,
                             bool *mapped) {
    *mapped = false;
    filehandle *f;
    if (fs_open_file(path, &f) < 0) {
        return false;
    }
    bool ok = true;
    if (fs_file_ioctl(f, FS_IOCTL_IS_LINEAR, mapped) < 0) {
        *mapped = false;
    }
    if (*mapped) {
        void *addr = NULL;
        ok = fs_file_ioctl(f, FS_IOCTL_GET_FILE_ADDR, &addr) == NO_ERROR &&
             (uintptr_t)addr % align == 0 &&
             (uint32_t)crc32(crc32(0, NULL, 0), addr, (unsigned int)size) == crc;
    }
    fs_close_file(f);
    return ok;
}

// the image's file alignment, from its header on the device
static uint32_t image_align(const char *dev) {
    bdev_t *b = bio_open(dev);
    struct rofs_header h;
    const bool read = b && bio_read(b, &h, 0, sizeof(h)) == (ssize_t)sizeof(h);
    if (b) {
        bio_close(b);
    }
    return read ? LE32(h.align) : 0;
}

struct manifest_entry {
    char type; // 'd' or 'f'
    unsigned long size;
    uint32_t crc;
    const char *path; // from the image's root, "." for the root itself
};

// The manifest's lines are "d <path>" and "f <size> <crc32 in hex> <path>", in buf (cut up in
// place). Returns how many, or -1 for a line that is neither.
static int manifest_parse(char *buf, struct manifest_entry *out, int max) {
    int n = 0;
    for (char *line = buf, *next; line && *line; line = next) {
        next = strchr(line, '\n');
        if (next) {
            *next++ = '\0';
        }
        if (n == max) {
            return -1;
        }
        struct manifest_entry *m = &out[n];
        char *p = line + 2;
        if (line[0] == 'f' && line[1] == ' ') {
            m->type = 'f';
            m->size = strtoul(p, &p, 10);
            if (*p != ' ') {
                return -1;
            }
            m->crc = (uint32_t)strtoul(p + 1, &p, 16);
            if (*p != ' ') {
                return -1;
            }
            p++;
        } else if (line[0] == 'd' && line[1] == ' ') {
            m->type = 'd';
        } else {
            return -1;
        }
        m->path = p;
        n++;
    }
    return n;
}

static const struct manifest_entry *manifest_find(const struct manifest_entry *m, int n,
                                                  const char *path) {
    for (int i = 0; i < n; i++) {
        if (!strcmp(m[i].path, path)) {
            return &m[i];
        }
    }
    return NULL;
}

// whether path's parent is dir: "." for a name with no '/'
static bool parent_is(const char *path, const char *dir) {
    const char *slash = strrchr(path, '/');
    if (!slash) {
        return !strcmp(dir, ".");
    }
    const size_t len = (size_t)(slash - path);
    return strlen(dir) == len && !strncmp(path, dir, len);
}

// every name the directory lists is in the manifest with its kind, and it lists as many as the
// manifest says it holds; the root also holds MANIFEST, which does not list itself
static bool dir_matches(const struct manifest_entry *m, int n, const char *dir, char *path,
                        char *child) {
    const bool root = !strcmp(dir, ".");
    snprintf(path, FS_MAX_PATH_LEN, "%s%s%s", DEV_MOUNT, root ? "" : "/", root ? "" : dir);
    dirhandle *d;
    if (fs_open_dir(path, &d) < 0) {
        return false;
    }
    int listed = 0, expected = root ? 1 : 0;
    bool ok = true;
    struct dirent ent;
    while (fs_read_dir(d, &ent) == NO_ERROR) {
        listed++;
        if (root && !strcmp(ent.name, "MANIFEST")) {
            continue;
        }
        snprintf(child, FS_MAX_PATH_LEN, "%s%s%s", root ? "" : dir, root ? "" : "/", ent.name);
        const struct manifest_entry *e = manifest_find(m, n, child);
        if (!e) {
            unittest_printf("\n  %s is not in the manifest\n", child);
            ok = false;
            continue;
        }
        char full[FS_MAX_PATH_LEN];
        snprintf(full, sizeof(full), DEV_MOUNT "/%s", child);
        // the layer opens a directory as a file too, so the kind comes from its stat
        filehandle *f;
        struct file_stat st;
        bool is_dir = false;
        if (fs_open_file(full, &f) == NO_ERROR) {
            is_dir = fs_stat_file(f, &st) == NO_ERROR && st.is_dir;
            fs_close_file(f);
        }
        ok = ok && is_dir == (e->type == 'd');
    }
    fs_close_dir(d);
    for (int i = 0; i < n; i++) {
        if (strcmp(m[i].path, ".") && parent_is(m[i].path, dir)) {
            expected++;
        }
    }
    return ok && listed == expected;
}

#define MANIFEST_MAX 512

static bool test_rofs_device(void) {
    BEGIN_TEST;

    bool required;
    const char *dev = test_device(&required);
    if (!dev) {
        if (required) {
            UNITTEST_FAIL_TRACEF("test.rofs.required is set but test.rofs.device is not\n");
            return false;
        }
        unittest_printf(" ROFS-TEST-SKIPPED(no test.rofs.device) ");
        return true;
    }
    ASSERT_EQ(NO_ERROR, fs_mount(DEV_MOUNT, "rofs", dev, FS_MOUNT_OPTION_READ_ONLY), "mount");

    filehandle *f;
    ASSERT_EQ(NO_ERROR, fs_open_file(DEV_MOUNT "/MANIFEST", &f), "manifest");
    struct file_stat st;
    ASSERT_EQ(NO_ERROR, fs_stat_file(f, &st), "manifest stat");
    char *buf = malloc((size_t)st.size + 1);
    struct manifest_entry *m = calloc(MANIFEST_MAX, sizeof(*m));
    char *path = malloc(FS_MAX_PATH_LEN);
    char *child = malloc(FS_MAX_PATH_LEN);
    ASSERT_TRUE(buf && m && path && child, "buffers");
    EXPECT_EQ((ssize_t)st.size, fs_read_file(f, buf, 0, (size_t)st.size), "manifest read");
    buf[st.size] = '\0';
    fs_close_file(f);

    const int n = manifest_parse(buf, m, MANIFEST_MAX);
    ASSERT_GT(n, 0, "manifest lines");
    const uint32_t align = image_align(dev);
    ASSERT_NE(0u, align, "the image's alignment");
    int files = 0, dirs = 0, in_place = 0;
    for (int i = 0; i < n; i++) {
        if (m[i].type == 'f') {
            snprintf(path, FS_MAX_PATH_LEN, DEV_MOUNT "/%s", m[i].path);
            uint64_t size;
            uint32_t crc;
            const bool ok = file_sum(path, &size, &crc);
            if (!ok || size != m[i].size || crc != m[i].crc) {
                unittest_printf("\n  %s: size %llu crc %08x\n", m[i].path,
                                (unsigned long long)size, (unsigned int)crc);
            }
            EXPECT_TRUE(ok && size == m[i].size && crc == m[i].crc, m[i].path);
            bool mapped;
            EXPECT_TRUE(in_place_matches(path, align, m[i].size, m[i].crc, &mapped), m[i].path);
            in_place += mapped;
            files++;
        } else {
            EXPECT_TRUE(dir_matches(m, n, m[i].path, path, child), m[i].path);
            dirs++;
        }
    }
    unittest_printf(" %d files (%d read in place) and %d directories ", files, in_place, dirs);
    EXPECT_GT(dirs, 0, "the root at least");

    free(child);
    free(path);
    free(m);
    free(buf);
    EXPECT_EQ(NO_ERROR, fs_unmount(DEV_MOUNT), "unmount");

    END_TEST;
}

BEGIN_TEST_CASE(rofs_tests)
RUN_TEST(test_rofs_tree)
RUN_TEST(test_rofs_wide)
RUN_TEST(test_rofs_empty)
RUN_TEST(test_rofs_corrupt)
RUN_TEST(test_rofs_device)
END_TEST_CASE(rofs_tests)
