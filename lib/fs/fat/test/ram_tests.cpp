/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/defines.h>
#include <lib/fs.h>
#include <lib/unittest.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <malloc.h>
#include <memory>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ramdisk.h"

// Correctness tests that stand on their own: they format a RAM backed volume
// with fs_format_device() and need no disk image, no kernel command line and no
// host tooling, so they run on every architecture in CI. That is what keeps the
// FAT tests from passing vacuously when no disk is attached.
//
// The sweep in kGeometries exists because a FAT driver is really three drivers.
// The width is chosen by cluster count alone, so two volumes a single cluster
// apart take completely different paths through the FAT accessors, and the
// 12 bit case additionally has entries that straddle sector boundaries.

namespace {

using fat_test::ram_volume;

constexpr const char *kMountPath = "/ramfat";
constexpr const char *kDeviceName = "fatram0";

// A payload whose every byte depends on its offset, so a block written to the
// wrong place is not just detectable but locatable. A run of zeroes, which is
// what a misplaced cluster usually reads back as, cannot be mistaken for this.
uint8_t pattern_byte(uint32_t seed, uint64_t offset) {
    uint32_t x = seed ^ (uint32_t)(offset * 2654435761u);
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;
    return (uint8_t)x;
}

void fill_pattern(uint8_t *buf, size_t len, uint32_t seed, uint64_t offset) {
    for (size_t i = 0; i < len; i++) {
        buf[i] = pattern_byte(seed, offset + i);
    }
}

// Returns the offset of the first mismatch, or len if the buffer matches.
size_t check_pattern(const uint8_t *buf, size_t len, uint32_t seed, uint64_t offset) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != pattern_byte(seed, offset + i)) {
            return i;
        }
    }
    return len;
}

// Write `len` bytes of pattern at `offset`, read them back and compare.
bool write_and_verify(const char *path, uint32_t seed, uint64_t offset, size_t len) {
    BEGIN_TEST;

    std::unique_ptr<uint8_t[]> wbuf(new (std::nothrow) uint8_t[len]);
    std::unique_ptr<uint8_t[]> rbuf(new (std::nothrow) uint8_t[len]);
    ASSERT_NONNULL(wbuf.get());
    ASSERT_NONNULL(rbuf.get());

    fill_pattern(wbuf.get(), len, seed, offset);

    filehandle *fh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
    auto close_fh = lk::make_auto_call([&]() { fs_close_file(fh); });

    ASSERT_EQ((ssize_t)len, fs_write_file(fh, wbuf.get(), offset, len));

    memset(rbuf.get(), 0, len);
    ASSERT_EQ((ssize_t)len, fs_read_file(fh, rbuf.get(), offset, len));

    const size_t bad = check_pattern(rbuf.get(), len, seed, offset);
    if (bad != len) {
        unittest_printf("\n        content mismatch at offset %llu (byte %zu of %zu): "
                        "expected %#x got %#x\n",
                        offset + bad, bad, len, pattern_byte(seed, offset + bad), rbuf[bad]);
    }
    EXPECT_EQ(len, bad);

    END_TEST;
}

// Reopen a file and verify a previously written range still matches. Used after
// a remount to prove the data and the directory entry both reached the device.
bool verify_file_contents(const char *path, uint32_t seed, uint64_t offset, size_t len,
                          uint64_t expected_size) {
    BEGIN_TEST;

    std::unique_ptr<uint8_t[]> rbuf(new (std::nothrow) uint8_t[len]);
    ASSERT_NONNULL(rbuf.get());

    filehandle *fh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
    auto close_fh = lk::make_auto_call([&]() { fs_close_file(fh); });

    struct file_stat st = {};
    ASSERT_EQ(NO_ERROR, fs_stat_file(fh, &st));
    EXPECT_EQ(expected_size, st.size);

    memset(rbuf.get(), 0, len);
    ASSERT_EQ((ssize_t)len, fs_read_file(fh, rbuf.get(), offset, len));

    const size_t bad = check_pattern(rbuf.get(), len, seed, offset);
    if (bad != len) {
        unittest_printf("\n        content mismatch after remount at offset %llu: "
                        "expected %#x got %#x\n",
                        offset + bad, pattern_byte(seed, offset + bad), rbuf[bad]);
    }
    EXPECT_EQ(len, bad);

    END_TEST;
}

// The body every geometry runs: exercise creation, growth across cluster
// boundaries, directories, readdir, unlink and persistence across a remount.
bool exercise_volume(ram_volume &vol, const fat_test::geometry &g) {
    BEGIN_TEST;

    char path[FS_MAX_PATH_LEN];

    // the mounted volume must report the geometry it was formatted with
    struct fs_stat fss = {};
    ASSERT_EQ(NO_ERROR, fs_stat_fs(vol.path(), &fss));
    const uint64_t cluster_bytes = (uint64_t)g.bytes_per_sector * g.sectors_per_cluster;
    ASSERT_NE(0u, (uint)cluster_bytes);
    // a freshly formatted volume has everything free except the FAT32 root dir
    EXPECT_LE(fss.free_space, fss.total_space);
    EXPECT_GT(fss.total_space, 0u);

    // a file smaller than one cluster
    snprintf(path, sizeof(path), "%s/small", vol.path());
    filehandle *fh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_create_file(path, &fh, 0));
    ASSERT_EQ(NO_ERROR, fs_close_file(fh));
    EXPECT_TRUE(write_and_verify(path, 0x1111, 0, 100));

    // a file spanning several clusters, written at an offset that crosses a
    // cluster boundary so the chain has to be walked
    snprintf(path, sizeof(path), "%s/multicluster", vol.path());
    fh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_create_file(path, &fh, 0));
    ASSERT_EQ(NO_ERROR, fs_close_file(fh));
    const size_t span = (size_t)MIN(cluster_bytes * 3 + 17, 64u * 1024);
    EXPECT_TRUE(write_and_verify(path, 0x2222, 0, span));

    // a long file name, which needs the LFN path in both directions
    snprintf(path, sizeof(path), "%s/a_long_file_name_for_this_geometry.txt", vol.path());
    fh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_create_file(path, &fh, 0));
    ASSERT_EQ(NO_ERROR, fs_close_file(fh));
    EXPECT_TRUE(write_and_verify(path, 0x3333, 0, 1234));

    // a subdirectory with a file in it
    char dirpath[FS_MAX_PATH_LEN];
    snprintf(dirpath, sizeof(dirpath), "%s/subdir", vol.path());
    ASSERT_EQ(NO_ERROR, fs_make_dir(dirpath));
    snprintf(path, sizeof(path), "%s/subdir/nested", vol.path());
    fh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_create_file(path, &fh, 0));
    ASSERT_EQ(NO_ERROR, fs_close_file(fh));
    EXPECT_TRUE(write_and_verify(path, 0x4444, 0, 300));

    // . and .. must both be present in a new directory
    dirhandle *dh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_open_dir(dirpath, &dh));
    bool saw_dot = false, saw_dotdot = false, saw_nested = false;
    dirent ent;
    while (fs_read_dir(dh, &ent) == NO_ERROR) {
        // "nested" fits 8.3 so it is stored as a short name with no long name
        // entry to preserve case; FAT short names are upper case, and lookup is
        // case insensitive, so compare the same way fat_find_file_in_dir does.
        if (!strcmp(ent.name, ".")) saw_dot = true;
        else if (!strcmp(ent.name, "..")) saw_dotdot = true;
        else if (!strnicmp(ent.name, "nested", sizeof("nested"))) saw_nested = true;
    }
    ASSERT_EQ(NO_ERROR, fs_close_dir(dh));
    EXPECT_TRUE(saw_dot);
    EXPECT_TRUE(saw_dotdot);
    EXPECT_TRUE(saw_nested);

    // everything written so far must survive a round trip through the device
    ASSERT_EQ(NO_ERROR, vol.remount());

    snprintf(path, sizeof(path), "%s/small", vol.path());
    EXPECT_TRUE(verify_file_contents(path, 0x1111, 0, 100, 100));
    snprintf(path, sizeof(path), "%s/multicluster", vol.path());
    EXPECT_TRUE(verify_file_contents(path, 0x2222, 0, span, span));
    snprintf(path, sizeof(path), "%s/a_long_file_name_for_this_geometry.txt", vol.path());
    EXPECT_TRUE(verify_file_contents(path, 0x3333, 0, 1234, 1234));
    snprintf(path, sizeof(path), "%s/subdir/nested", vol.path());
    EXPECT_TRUE(verify_file_contents(path, 0x4444, 0, 300, 300));

    // shrink, and confirm the tail is really gone rather than resurrected
    snprintf(path, sizeof(path), "%s/multicluster", vol.path());
    fh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
    ASSERT_EQ(NO_ERROR, fs_truncate_file(fh, 50));
    struct file_stat st = {};
    ASSERT_EQ(NO_ERROR, fs_stat_file(fh, &st));
    EXPECT_EQ(50u, st.size);
    ASSERT_EQ(NO_ERROR, fs_close_file(fh));

    ASSERT_EQ(NO_ERROR, vol.remount());
    fh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
    st = {};
    ASSERT_EQ(NO_ERROR, fs_stat_file(fh, &st));
    EXPECT_EQ(50u, st.size);
    // reading past the new end must return nothing, not the old contents
    uint8_t tail[64];
    memset(tail, 0xaa, sizeof(tail));
    EXPECT_EQ(0, fs_read_file(fh, tail, 50, sizeof(tail)));
    ASSERT_EQ(NO_ERROR, fs_close_file(fh));

    // tear it all down; a volume that cannot be emptied is a leak
    ASSERT_EQ(NO_ERROR, fs_remove_file(path));
    snprintf(path, sizeof(path), "%s/small", vol.path());
    ASSERT_EQ(NO_ERROR, fs_remove_file(path));
    snprintf(path, sizeof(path), "%s/a_long_file_name_for_this_geometry.txt", vol.path());
    ASSERT_EQ(NO_ERROR, fs_remove_file(path));
    snprintf(path, sizeof(path), "%s/subdir/nested", vol.path());
    ASSERT_EQ(NO_ERROR, fs_remove_file(path));
    ASSERT_EQ(NO_ERROR, fs_remove_dir(dirpath));

    END_TEST;
}

bool test_fat_ram_geometries() {
    BEGIN_TEST;

    uint ran = 0, skipped = 0;

    for (size_t i = 0; i < fat_test::kGeometryCount; i++) {
        const auto &g = fat_test::kGeometries[i];
        const size_t size = fat_test::volume_size_for(g);

        ram_volume vol;
        const auto args = fat_test::format_args_for(g);
        status_t err = vol.create(kDeviceName, kMountPath, size, args);
        if (err == ERR_NO_MEMORY) {
            // too large for this target: report it and move on rather than
            // failing, so the small architectures still run the rest
            unittest_printf("\n        skipping %s: %zu bytes would not allocate ",
                            g.name, size);
            skipped++;
            continue;
        }
        if (err < 0) {
            UNITTEST_FAIL_TRACEF("geometry %s (%zu bytes) failed to format/mount: %d\n",
                                 g.name, size, err);
            all_ok = false;
            continue;
        }

        if (!exercise_volume(vol, g)) {
            UNITTEST_FAIL_TRACEF("geometry %s failed\n", g.name);
            all_ok = false;
        }
        ran++;
    }

    unittest_printf("\n        %u geometries exercised, %u skipped ", ran, skipped);

    // If every single geometry skipped then this test proved nothing, which is
    // exactly the vacuous pass this file exists to prevent.
    EXPECT_GT(ran, 0u);

    END_TEST;
}

// A volume formatted by the driver must be describable by the same mount code
// that reads mkfs.fat images, at every width.
bool test_fat_ram_format_roundtrip() {
    BEGIN_TEST;

    struct {
        uint32_t fat_bits;
        uint32_t clusters;
    } cases[] = {{12, 1000}, {16, 5000}};

    for (auto &c : cases) {
        fat_test::geometry g = {"roundtrip", c.fat_bits, 512, 1, c.clusters};
        ram_volume vol;
        const auto args = fat_test::format_args_for(g);
        status_t err = vol.create(kDeviceName, kMountPath, fat_test::volume_size_for(g), args);
        if (err == ERR_NO_MEMORY) {
            continue;
        }
        ASSERT_EQ(NO_ERROR, err);

        struct fs_stat fss = {};
        ASSERT_EQ(NO_ERROR, fs_stat_fs(vol.path(), &fss));

        // a fresh volume of N clusters has at least N clusters free on FAT12/16
        // (FAT32 spends one on the root directory)
        EXPECT_GE(fss.free_space, (uint64_t)c.clusters * 512);
    }

    END_TEST;
}

// Requesting a width the geometry cannot produce has to fail loudly at format
// time. Silently formatting something else is how a volume ends up mounting at
// the wrong FAT width.
bool test_fat_format_rejects_impossible_geometry() {
    BEGIN_TEST;

    void *mem = memalign(CACHE_LINE, 1 * 1024 * 1024);
    ASSERT_NONNULL(mem);
    auto free_mem = lk::make_auto_call([&]() { free(mem); });

    ASSERT_EQ(0, create_membdev("fatbadfmt", mem, 1 * 1024 * 1024));
    bdev_t *dev = bio_open("fatbadfmt");
    ASSERT_NONNULL(dev);
    auto cleanup = lk::make_auto_call([&]() {
        bio_close(dev);
        bio_unregister_device(dev);
    });

    // 1MB cannot hold the 65525 clusters FAT32 requires
    fat_format_args_t args = {};
    args.fat_bits = 32;
    args.bytes_per_sector = 512;
    args.sectors_per_cluster = 1;
    EXPECT_EQ(ERR_INVALID_ARGS, fs_format_device("fat", "fatbadfmt", &args));

    // nor is it big enough to be anything but FAT12, so asking for FAT16 fails
    args.fat_bits = 16;
    EXPECT_EQ(ERR_INVALID_ARGS, fs_format_device("fat", "fatbadfmt", &args));

    // invalid parameters
    args.fat_bits = 12;
    args.bytes_per_sector = 777;
    EXPECT_EQ(ERR_INVALID_ARGS, fs_format_device("fat", "fatbadfmt", &args));

    args.bytes_per_sector = 512;
    args.sectors_per_cluster = 3; // not a power of two
    EXPECT_EQ(ERR_INVALID_ARGS, fs_format_device("fat", "fatbadfmt", &args));

    args.sectors_per_cluster = 1;
    args.root_entries = 7; // not a whole number of sectors
    EXPECT_EQ(ERR_INVALID_ARGS, fs_format_device("fat", "fatbadfmt", &args));

    // and a sane request on the same device must still succeed
    args.root_entries = 0;
    EXPECT_EQ(NO_ERROR, fs_format_device("fat", "fatbadfmt", &args));

    END_TEST;
}

// Mounting a corrupt volume must fail cleanly. Before the device size check went
// in, a BPB claiming more sectors than the device holds sent every subsequent
// cluster computation off the end of it. These are cheap to construct on a RAM
// disk and impossible to construct with mkfs.fat.
bool test_fat_mount_rejects_malformed() {
    BEGIN_TEST;

    constexpr size_t kSize = 2 * 1024 * 1024;
    constexpr const char *kDev = "fatbadbpb";

    void *mem = memalign(CACHE_LINE, kSize);
    ASSERT_NONNULL(mem);
    auto free_mem = lk::make_auto_call([&]() { free(mem); });

    ASSERT_EQ(0, create_membdev(kDev, mem, kSize));
    bdev_t *dev = bio_open(kDev);
    ASSERT_NONNULL(dev);
    auto cleanup = lk::make_auto_call([&]() {
        bio_close(dev);
        bio_unregister_device(dev);
    });

    fat_format_args_t args = {};
    args.fat_bits = 12;
    args.bytes_per_sector = 512;
    args.sectors_per_cluster = 1;

    // A pristine volume, kept aside so each case starts from a good image.
    ASSERT_EQ(NO_ERROR, fs_format_device("fat", kDev, &args));
    std::unique_ptr<uint8_t[]> pristine(new (std::nothrow) uint8_t[512]);
    ASSERT_NONNULL(pristine.get());
    ASSERT_EQ(512, bio_read(dev, pristine.get(), 0, 512));

    // it must mount before we start breaking it, or the test proves nothing
    ASSERT_EQ(NO_ERROR, fs_mount("/badbpb", "fat", kDev, FS_MOUNT_OPTION_NONE));
    ASSERT_EQ(NO_ERROR, fs_unmount("/badbpb"));

    struct {
        const char *what;
        uint32_t offset;
        uint32_t width; // 1, 2 or 4 bytes
        uint32_t value;
    } const cases[] = {
        {"total sectors larger than the device", 0x20, 4, 0x00ffffff},
        {"total sectors (16 bit) larger than the device", 0x13, 2, 0xfffe},
        {"zero sectors per cluster", 0x0d, 1, 0},
        {"sectors per cluster not a power of two", 0x0d, 1, 3},
        {"sectors per cluster absurdly large", 0x0d, 1, 0xff},
        {"unsupported sector size", 0x0b, 2, 777},
        {"zero FAT count", 0x10, 1, 0},
        {"absurd FAT count", 0x10, 1, 200},
        {"bad media descriptor", 0x15, 1, 0xf0},
        {"root entries not a whole sector", 0x11, 2, 7},
        {"zero sectors per FAT", 0x16, 2, 0},
    };

    std::unique_ptr<uint8_t[]> sector(new (std::nothrow) uint8_t[512]);
    ASSERT_NONNULL(sector.get());

    for (auto &c : cases) {
        memcpy(sector.get(), pristine.get(), 512);
        switch (c.width) {
            case 1:
                sector[c.offset] = (uint8_t)c.value;
                break;
            case 2:
                sector[c.offset] = (uint8_t)c.value;
                sector[c.offset + 1] = (uint8_t)(c.value >> 8);
                break;
            default:
                for (int i = 0; i < 4; i++) {
                    sector[c.offset + i] = (uint8_t)(c.value >> (i * 8));
                }
                break;
        }
        // a 32 bit total sector count is only consulted when the 16 bit one is
        // zero, so clear it for that case
        if (c.offset == 0x20) {
            sector[0x13] = sector[0x14] = 0;
        }
        ASSERT_EQ(512, bio_write(dev, sector.get(), 0, 512));

        status_t err = fs_mount("/badbpb", "fat", kDev, FS_MOUNT_OPTION_NONE);
        if (err == NO_ERROR) {
            fs_unmount("/badbpb");
            UNITTEST_FAIL_TRACEF("mounted a volume with %s\n", c.what);
            all_ok = false;
        }
    }

    // restore, and confirm the volume is still good: a rejected mount must not
    // have left anything behind
    ASSERT_EQ(512, bio_write(dev, pristine.get(), 0, 512));
    ASSERT_EQ(NO_ERROR, fs_mount("/badbpb", "fat", kDev, FS_MOUNT_OPTION_NONE));
    ASSERT_EQ(NO_ERROR, fs_unmount("/badbpb"));

    END_TEST;
}

} // anonymous namespace

BEGIN_TEST_CASE(fat_ram)
RUN_TEST(test_fat_ram_geometries)
RUN_TEST(test_fat_ram_format_roundtrip)
RUN_TEST(test_fat_format_rejects_impossible_geometry)
RUN_TEST(test_fat_mount_rejects_malformed)
END_TEST_CASE(fat_ram)
