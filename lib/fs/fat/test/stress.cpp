/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <kernel/thread.h>
#include <lib/cmdline.h>
#include <lib/fs.h>
#include <lib/unittest.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <memory>
#include <new>
#include <rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ramdisk.h"

// Stress tests for the FAT driver.
//
// These are deliberately slower and more destructive than the correctness tests
// in test.cpp and ram_tests.cpp, so they only run when test.fat.stress=1 is on
// the kernel command line. scripts/run-fat-tests.py --stress sets it (and raises
// the QEMU timeout to match); the default boot-time `ut all` path stays fast.
//
// Everything here runs against a RAM backed volume, so no disk image is needed
// and the whole suite is available on any architecture.

namespace {

using fat_test::ram_volume;

constexpr const char *kMountPath = "/fatstress";
constexpr const char *kDeviceName = "fatstress0";

bool stress_enabled() {
    static bool checked = false;
    static bool enabled = false;
    if (!checked) {
        checked = true;
        bool val = false;
        if (cmdline_get_bool("test.fat.stress", &val) == NO_ERROR) {
            enabled = val;
        }
    }
    return enabled;
}

#define SKIP_UNLESS_STRESS()                                                  \
    do {                                                                      \
        if (!stress_enabled()) {                                              \
            unittest_printf(" set test.fat.stress=1 to run this ");           \
            return true;                                                      \
        }                                                                     \
    } while (0)

uint32_t stress_seed() {
    uint32_t seed = 0;
    if (cmdline_get_uint32("test.fat.stress.seed", &seed) == NO_ERROR && seed != 0) {
        return seed;
    }
    return 0x5eed1234;
}

uint32_t stress_ops() {
    uint32_t ops = 0;
    if (cmdline_get_uint32("test.fat.stress.ops", &ops) == NO_ERROR && ops != 0) {
        return ops;
    }
    return 600;
}

// A small deterministic PRNG so a failure is exactly reproducible from the seed
// printed alongside it. Deliberately not rand(), whose state is global and
// shared with everything else running in the system.
struct prng {
    uint32_t state;
    explicit prng(uint32_t seed) : state(seed ? seed : 1) {}
    uint32_t next() {
        // xorshift32
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    uint32_t below(uint32_t n) { return n ? (next() % n) : 0; }
};

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

size_t first_mismatch(const uint8_t *buf, size_t len, uint32_t seed, uint64_t offset) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != pattern_byte(seed, offset + i)) {
            return i;
        }
    }
    return len;
}

// Create a volume with the requested number of clusters, at 512 bytes each so
// that a few megabytes still gives thousands of clusters to fragment and fill.
// The FAT width follows from the cluster count -- asking for a width the count
// cannot produce is rejected by format(), so let the count decide.
status_t make_volume(ram_volume *vol, uint32_t clusters = 6000) {
    const uint32_t fat_bits = (clusters < 4085) ? 12 : ((clusters < 65525) ? 16 : 32);
    fat_test::geometry g = {"stress", fat_bits, 512, 1, clusters};
    const auto args = fat_test::format_args_for(g);
    return vol->create(kDeviceName, kMountPath, fat_test::volume_size_for(g), args);
}

status_t write_at(const char *path, const void *buf, off_t off, size_t len) {
    filehandle *fh = nullptr;
    status_t err = fs_open_file(path, &fh);
    if (err < 0) {
        return err;
    }
    ssize_t written = fs_write_file(fh, buf, off, len);
    fs_close_file(fh);
    if (written < 0) {
        return (status_t)written;
    }
    return ((size_t)written == len) ? NO_ERROR : ERR_IO;
}

status_t create_empty(const char *path) {
    filehandle *fh = nullptr;
    status_t err = fs_create_file(path, &fh, 0);
    if (err < 0) {
        return err;
    }
    return fs_close_file(fh);
}

// Round trip a set of files through a remount and verify every byte. This is the
// cheapest way to catch metadata that never actually reaches the device, which
// nothing in the suite covered before.
bool test_fat_stress_remount() {
    BEGIN_TEST;
    SKIP_UNLESS_STRESS();

    ram_volume vol;
    status_t err = make_volume(&vol);
    if (err == ERR_NO_MEMORY) {
        unittest_printf(" not enough memory for the stress volume ");
        END_TEST;
    }
    ASSERT_EQ(NO_ERROR, err);

    constexpr int kFiles = 24;
    constexpr size_t kLen = 3000;

    std::unique_ptr<uint8_t[]> buf_storage(new (std::nothrow) uint8_t[kLen]);
    ASSERT_NONNULL(buf_storage.get());
    uint8_t *buf = buf_storage.get();

    char path[FS_MAX_PATH_LEN];
    for (int i = 0; i < kFiles; i++) {
        snprintf(path, sizeof(path), "%s/remount_%02d", vol.path(), i);
        ASSERT_EQ(NO_ERROR, create_empty(path));
        fill_pattern(buf, kLen, 0x100 + i, 0);
        ASSERT_EQ(NO_ERROR, write_at(path, buf, 0, kLen));
    }

    // three round trips, verifying in full each time
    for (int round = 0; round < 3; round++) {
        ASSERT_EQ(NO_ERROR, vol.remount());

        for (int i = 0; i < kFiles; i++) {
            snprintf(path, sizeof(path), "%s/remount_%02d", vol.path(), i);

            filehandle *fh = nullptr;
            ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
            auto close_fh = lk::make_auto_call([&]() { fs_close_file(fh); });

            struct file_stat st = {};
            ASSERT_EQ(NO_ERROR, fs_stat_file(fh, &st));
            if (st.size != kLen) {
                UNITTEST_FAIL_TRACEF("round %d file %d: size %llu, expected %zu\n",
                                     round, i, st.size, kLen);
                all_ok = false;
                continue;
            }

            memset(buf, 0, kLen);
            ASSERT_EQ((ssize_t)kLen, fs_read_file(fh, buf, 0, kLen));
            const size_t bad = first_mismatch(buf, kLen, 0x100 + i, 0);
            if (bad != kLen) {
                UNITTEST_FAIL_TRACEF("round %d file %d: content differs at byte %zu\n",
                                     round, i, bad);
                all_ok = false;
            }
        }
    }

    for (int i = 0; i < kFiles; i++) {
        snprintf(path, sizeof(path), "%s/remount_%02d", vol.path(), i);
        EXPECT_EQ(NO_ERROR, fs_remove_file(path));
    }

    END_TEST;
}

// Fill the volume until it will take no more, then check that the failure was
// clean: previously written data intact, the volume still mountable, and the
// space recoverable by deleting.
bool test_fat_stress_enospc() {
    BEGIN_TEST;
    SKIP_UNLESS_STRESS();

    ram_volume vol;
    status_t err = make_volume(&vol, 2000);
    if (err == ERR_NO_MEMORY) {
        unittest_printf(" not enough memory for the stress volume ");
        END_TEST;
    }
    ASSERT_EQ(NO_ERROR, err);

    struct fs_stat before = {};
    ASSERT_EQ(NO_ERROR, fs_stat_fs(vol.path(), &before));

    constexpr size_t kChunk = 4096;
    std::unique_ptr<uint8_t[]> buf_storage(new (std::nothrow) uint8_t[kChunk]);
    ASSERT_NONNULL(buf_storage.get());
    uint8_t *buf = buf_storage.get();

    char path[FS_MAX_PATH_LEN];
    int created = 0;
    status_t fill_err = NO_ERROR;

    // Bounded so a driver that never reports full cannot spin here forever.
    const int kMaxFiles = 600;
    for (; created < kMaxFiles; created++) {
        snprintf(path, sizeof(path), "%s/fill_%03d", vol.path(), created);
        status_t e = create_empty(path);
        if (e < 0) {
            fill_err = e;
            break;
        }
        fill_pattern(buf, kChunk, 0x900 + created, 0);
        e = write_at(path, buf, 0, kChunk);
        if (e < 0) {
            fill_err = e;
            // the file exists but is short; do not count it as fully written
            break;
        }
    }

    unittest_printf("\n        filled with %d files, stopped with %d ", created, fill_err);

    // It must actually have run out rather than claiming infinite space.
    EXPECT_LT(created, kMaxFiles);
    // and the failure has to be a sane error code, not a corrupted success
    EXPECT_LT(fill_err, 0);

    // Everything fully written before the failure must still read back correctly.
    for (int i = 0; i < created; i++) {
        snprintf(path, sizeof(path), "%s/fill_%03d", vol.path(), i);
        filehandle *fh = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
        auto close_fh = lk::make_auto_call([&]() { fs_close_file(fh); });
        memset(buf, 0, kChunk);
        ASSERT_EQ((ssize_t)kChunk, fs_read_file(fh, buf, 0, kChunk));
        const size_t bad = first_mismatch(buf, kChunk, 0x900 + i, 0);
        if (bad != kChunk) {
            UNITTEST_FAIL_TRACEF("file %d damaged by the out of space condition "
                                 "at byte %zu\n", i, bad);
            all_ok = false;
            break;
        }
    }

    // A full volume must still survive a round trip through the device.
    ASSERT_EQ(NO_ERROR, vol.remount());

    // Free everything and confirm the space really comes back.
    for (int i = 0; i <= created && i < kMaxFiles; i++) {
        snprintf(path, sizeof(path), "%s/fill_%03d", vol.path(), i);
        fs_remove_file(path);
    }

    struct fs_stat after = {};
    ASSERT_EQ(NO_ERROR, fs_stat_fs(vol.path(), &after));
    EXPECT_EQ(before.free_space, after.free_space);

    END_TEST;
}

// Grow several files in turn so their cluster chains interleave, then verify
// each one. Sequential writes never produce a fragmented chain, so nothing else
// in the suite walks a chain that is not already contiguous.
bool test_fat_stress_fragmentation() {
    BEGIN_TEST;
    SKIP_UNLESS_STRESS();

    ram_volume vol;
    status_t err = make_volume(&vol);
    if (err == ERR_NO_MEMORY) {
        unittest_printf(" not enough memory for the stress volume ");
        END_TEST;
    }
    ASSERT_EQ(NO_ERROR, err);

    constexpr int kFiles = 12;
    constexpr int kRounds = 8;
    constexpr size_t kChunk = 600; // not a multiple of the 512 byte cluster

    std::unique_ptr<uint8_t[]> buf_storage(new (std::nothrow) uint8_t[kChunk]);
    ASSERT_NONNULL(buf_storage.get());
    uint8_t *buf = buf_storage.get();

    char path[FS_MAX_PATH_LEN];
    for (int i = 0; i < kFiles; i++) {
        snprintf(path, sizeof(path), "%s/frag_%02d", vol.path(), i);
        ASSERT_EQ(NO_ERROR, create_empty(path));
    }

    // round robin: every file grows by one chunk before any grows by two, so the
    // clusters of all twelve chains end up interleaved on the volume
    for (int round = 0; round < kRounds; round++) {
        for (int i = 0; i < kFiles; i++) {
            snprintf(path, sizeof(path), "%s/frag_%02d", vol.path(), i);
            const uint64_t off = (uint64_t)round * kChunk;
            fill_pattern(buf, kChunk, 0x200 + i, off);
            ASSERT_EQ(NO_ERROR, write_at(path, buf, off, kChunk));
        }
    }

    ASSERT_EQ(NO_ERROR, vol.remount());

    auto verify_all = [&](const char *stage) {
        bool ok = true;
        for (int i = 0; i < kFiles; i++) {
            snprintf(path, sizeof(path), "%s/frag_%02d", vol.path(), i);
            filehandle *fh = nullptr;
            if (fs_open_file(path, &fh) < 0) {
                unittest_printf("\n        %s: cannot open frag_%02d ", stage, i);
                ok = false;
                continue;
            }
            for (int round = 0; round < kRounds; round++) {
                const uint64_t off = (uint64_t)round * kChunk;
                memset(buf, 0, kChunk);
                if (fs_read_file(fh, buf, off, kChunk) != (ssize_t)kChunk) {
                    unittest_printf("\n        %s: short read of frag_%02d at %llu ",
                                    stage, i, off);
                    ok = false;
                    break;
                }
                const size_t bad = first_mismatch(buf, kChunk, 0x200 + i, off);
                if (bad != kChunk) {
                    unittest_printf("\n        %s: frag_%02d differs at %llu ",
                                    stage, i, off + bad);
                    ok = false;
                    break;
                }
            }
            fs_close_file(fh);
        }
        return ok;
    };

    EXPECT_TRUE(verify_all("after interleaved growth"));

    // Punch holes by deleting every other file, then refill them. The new files
    // land in the freed clusters, which are scattered all over the volume.
    for (int i = 0; i < kFiles; i += 2) {
        snprintf(path, sizeof(path), "%s/frag_%02d", vol.path(), i);
        ASSERT_EQ(NO_ERROR, fs_remove_file(path));
    }
    for (int i = 0; i < kFiles; i += 2) {
        snprintf(path, sizeof(path), "%s/frag_%02d", vol.path(), i);
        ASSERT_EQ(NO_ERROR, create_empty(path));
        for (int round = 0; round < kRounds; round++) {
            const uint64_t off = (uint64_t)round * kChunk;
            fill_pattern(buf, kChunk, 0x200 + i, off);
            ASSERT_EQ(NO_ERROR, write_at(path, buf, off, kChunk));
        }
    }

    ASSERT_EQ(NO_ERROR, vol.remount());
    EXPECT_TRUE(verify_all("after refilling holes"));

    for (int i = 0; i < kFiles; i++) {
        snprintf(path, sizeof(path), "%s/frag_%02d", vol.path(), i);
        EXPECT_EQ(NO_ERROR, fs_remove_file(path));
    }

    END_TEST;
}

// Reads and writes at every awkward offset around sector and cluster edges.
bool test_fat_stress_boundaries() {
    BEGIN_TEST;
    SKIP_UNLESS_STRESS();

    ram_volume vol;
    status_t err = make_volume(&vol);
    if (err == ERR_NO_MEMORY) {
        unittest_printf(" not enough memory for the stress volume ");
        END_TEST;
    }
    ASSERT_EQ(NO_ERROR, err);

    struct fs_stat fss = {};
    ASSERT_EQ(NO_ERROR, fs_stat_fs(vol.path(), &fss));

    // Offsets and lengths either side of the 512 byte sector and cluster edges,
    // plus a few multi-cluster ones.
    static const uint64_t offsets[] = {0, 1, 511, 512, 513, 1023, 1024, 1025,
                                       4095, 4096, 4097, 8191, 8192, 8193};
    static const size_t lengths[] = {1, 2, 511, 512, 513, 1024, 1025, 4096};

    char path[FS_MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/boundary", vol.path());

    std::unique_ptr<uint8_t[]> wbuf_storage(new (std::nothrow) uint8_t[4096]);
    std::unique_ptr<uint8_t[]> rbuf_storage(new (std::nothrow) uint8_t[4096]);
    ASSERT_NONNULL(wbuf_storage.get());
    ASSERT_NONNULL(rbuf_storage.get());
    uint8_t *wbuf = wbuf_storage.get();
    uint8_t *rbuf = rbuf_storage.get();

    uint32_t seed = 1;
    for (auto off : offsets) {
        for (auto len : lengths) {
            seed++;

            // a fresh file each time so the write is always into a hole
            fs_remove_file(path);
            ASSERT_EQ(NO_ERROR, create_empty(path));

            fill_pattern(wbuf, len, seed, off);
            status_t e = write_at(path, wbuf, (off_t)off, len);
            if (e < 0) {
                UNITTEST_FAIL_TRACEF("write at offset %llu length %zu failed: %d\n",
                                     off, len, e);
                all_ok = false;
                continue;
            }

            filehandle *fh = nullptr;
            ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
            auto close_fh = lk::make_auto_call([&]() { fs_close_file(fh); });

            struct file_stat st = {};
            ASSERT_EQ(NO_ERROR, fs_stat_file(fh, &st));
            if (st.size != off + len) {
                UNITTEST_FAIL_TRACEF("offset %llu length %zu: size %llu, expected %llu\n",
                                     off, len, st.size, off + len);
                all_ok = false;
                continue;
            }

            memset(rbuf, 0, len);
            if (fs_read_file(fh, rbuf, (off_t)off, len) != (ssize_t)len) {
                UNITTEST_FAIL_TRACEF("short read at offset %llu length %zu\n", off, len);
                all_ok = false;
                continue;
            }
            const size_t bad = first_mismatch(rbuf, len, seed, off);
            if (bad != len) {
                UNITTEST_FAIL_TRACEF("offset %llu length %zu: differs at byte %zu\n",
                                     off, len, bad);
                all_ok = false;
                continue;
            }

            // everything before the write offset is a hole and must read zero
            if (off > 0) {
                const size_t gap = (size_t)MIN(off, 4096u);
                memset(rbuf, 0xaa, gap);
                if (fs_read_file(fh, rbuf, 0, gap) != (ssize_t)gap) {
                    UNITTEST_FAIL_TRACEF("short read of the hole before %llu\n", off);
                    all_ok = false;
                    continue;
                }
                for (size_t i = 0; i < gap; i++) {
                    if (rbuf[i] != 0) {
                        UNITTEST_FAIL_TRACEF("hole before offset %llu is not zero "
                                             "at byte %zu (%#x)\n", off, i, rbuf[i]);
                        all_ok = false;
                        break;
                    }
                }
            }
        }
    }

    fs_remove_file(path);

    END_TEST;
}

// Create a lot of long names, delete alternate ones and create more, to churn
// the directory entry slots and the short name alias generator.
bool test_fat_stress_dir_slot_reuse() {
    BEGIN_TEST;
    SKIP_UNLESS_STRESS();

    ram_volume vol;
    status_t err = make_volume(&vol);
    if (err == ERR_NO_MEMORY) {
        unittest_printf(" not enough memory for the stress volume ");
        END_TEST;
    }
    ASSERT_EQ(NO_ERROR, err);

    char dir[FS_MAX_PATH_LEN];
    snprintf(dir, sizeof(dir), "%s/slots", vol.path());
    ASSERT_EQ(NO_ERROR, fs_make_dir(dir));

    constexpr int kNames = 120;
    char path[FS_MAX_PATH_LEN];

    // names of differing length so the LFN runs are of differing size, which is
    // what makes slot reuse interesting: a freed run may not fit the next name
    auto make_name = [&](int i, int generation) {
        snprintf(path, sizeof(path), "%s/gen%d_entry_with_a_longish_name_%03d.dat",
                 dir, generation, i);
    };

    for (int i = 0; i < kNames; i++) {
        make_name(i, 0);
        ASSERT_EQ(NO_ERROR, create_empty(path));
    }

    // delete every other one, leaving gaps of varying size
    for (int i = 0; i < kNames; i += 2) {
        make_name(i, 0);
        ASSERT_EQ(NO_ERROR, fs_remove_file(path));
    }

    // fill the gaps with names of a different length
    for (int i = 0; i < kNames; i += 2) {
        make_name(i, 11);
        ASSERT_EQ(NO_ERROR, create_empty(path));
    }

    ASSERT_EQ(NO_ERROR, vol.remount());

    // everything that should exist must resolve, and nothing else
    int found = 0;
    for (int i = 0; i < kNames; i++) {
        make_name(i, (i % 2 == 0) ? 11 : 0);
        filehandle *fh = nullptr;
        if (fs_open_file(path, &fh) != NO_ERROR) {
            UNITTEST_FAIL_TRACEF("entry %d missing after slot reuse: %s\n", i, path);
            all_ok = false;
            continue;
        }
        fs_close_file(fh);
        found++;

        // the deleted generation must be gone
        make_name(i, (i % 2 == 0) ? 0 : 11);
        fh = nullptr;
        if (fs_open_file(path, &fh) == NO_ERROR) {
            fs_close_file(fh);
            UNITTEST_FAIL_TRACEF("stale entry %d still resolves: %s\n", i, path);
            all_ok = false;
        }
    }
    EXPECT_EQ(kNames, found);

    // the directory listing must agree with that: kNames entries plus . and ..
    dirhandle *dh = nullptr;
    ASSERT_EQ(NO_ERROR, fs_open_dir(dir, &dh));
    int count = 0;
    dirent ent;
    while (fs_read_dir(dh, &ent) == NO_ERROR) {
        count++;
    }
    ASSERT_EQ(NO_ERROR, fs_close_dir(dh));
    EXPECT_EQ(kNames + 2, count);

    for (int i = 0; i < kNames; i++) {
        make_name(i, (i % 2 == 0) ? 11 : 0);
        EXPECT_EQ(NO_ERROR, fs_remove_file(path));
    }
    EXPECT_EQ(NO_ERROR, fs_remove_dir(dir));

    END_TEST;
}

// Random operations against a shadow model held in memory. This is the one that
// finds things nobody thought to write a case for. The seed is printed on every
// failure so a failure can be replayed exactly with test.fat.stress.seed=.
struct model_file {
    bool exists;
    uint32_t seed;
    uint32_t size;
};

bool test_fat_stress_random_ops() {
    BEGIN_TEST;
    SKIP_UNLESS_STRESS();

    ram_volume vol;
    status_t err = make_volume(&vol);
    if (err == ERR_NO_MEMORY) {
        unittest_printf(" not enough memory for the stress volume ");
        END_TEST;
    }
    ASSERT_EQ(NO_ERROR, err);

    const uint32_t seed = stress_seed();
    const uint32_t ops = stress_ops();
    unittest_printf("\n        seed %#x, %u operations ", seed, ops);

    prng rng(seed);

    constexpr int kSlots = 16;
    constexpr uint32_t kMaxSize = 12 * 1024;
    std::unique_ptr<model_file[]> model(new (std::nothrow) model_file[kSlots]());
    ASSERT_NONNULL(model.get());

    std::unique_ptr<uint8_t[]> buf_storage(new (std::nothrow) uint8_t[kMaxSize]);
    ASSERT_NONNULL(buf_storage.get());
    uint8_t *buf = buf_storage.get();

    char path[FS_MAX_PATH_LEN];
    auto slot_path = [&](int i) {
        snprintf(path, sizeof(path), "%s/rand_%02d.dat", vol.path(), i);
    };

    // Compare one file against the model in full.
    auto verify_slot = [&](int i, const char *stage) {
        slot_path(i);
        filehandle *fh = nullptr;
        status_t e = fs_open_file(path, &fh);
        if (!model[i].exists) {
            if (e == NO_ERROR) {
                fs_close_file(fh);
                unittest_printf("\n        [seed %#x] %s: slot %d should not exist ",
                                seed, stage, i);
                return false;
            }
            return true;
        }
        if (e != NO_ERROR) {
            unittest_printf("\n        [seed %#x] %s: slot %d missing (%d) ",
                            seed, stage, i, e);
            return false;
        }
        auto close_fh = lk::make_auto_call([&]() { fs_close_file(fh); });

        struct file_stat st = {};
        if (fs_stat_file(fh, &st) != NO_ERROR || st.size != model[i].size) {
            unittest_printf("\n        [seed %#x] %s: slot %d size %llu, model %u ",
                            seed, stage, i, st.size, model[i].size);
            return false;
        }
        if (model[i].size == 0) {
            return true;
        }
        memset(buf, 0, model[i].size);
        if (fs_read_file(fh, buf, 0, model[i].size) != (ssize_t)model[i].size) {
            unittest_printf("\n        [seed %#x] %s: slot %d short read ",
                            seed, stage, i);
            return false;
        }
        const size_t bad = first_mismatch(buf, model[i].size, model[i].seed, 0);
        if (bad != model[i].size) {
            unittest_printf("\n        [seed %#x] %s: slot %d differs at byte %zu ",
                            seed, stage, i, bad);
            return false;
        }
        return true;
    };

    for (uint32_t op = 0; op < ops && all_ok; op++) {
        const int slot = (int)rng.below(kSlots);
        slot_path(slot);

        switch (rng.below(5)) {
            case 0: // create, or notice it already exists
                if (!model[slot].exists) {
                    ASSERT_EQ(NO_ERROR, create_empty(path));
                    model[slot].exists = true;
                    model[slot].seed = rng.next();
                    model[slot].size = 0;
                } else {
                    filehandle *fh = nullptr;
                    EXPECT_EQ(ERR_ALREADY_EXISTS, fs_create_file(path, &fh, 0));
                }
                break;

            case 1: { // rewrite the whole file at a new size
                if (!model[slot].exists) {
                    break;
                }
                const uint32_t size = rng.below(kMaxSize);
                const uint32_t new_seed = rng.next();
                filehandle *fh = nullptr;
                ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
                auto close_fh = lk::make_auto_call([&]() { fs_close_file(fh); });
                // truncate to zero first so the model is the whole content
                ASSERT_EQ(NO_ERROR, fs_truncate_file(fh, 0));
                if (size > 0) {
                    fill_pattern(buf, size, new_seed, 0);
                    ASSERT_EQ((ssize_t)size, fs_write_file(fh, buf, 0, size));
                }
                model[slot].seed = new_seed;
                model[slot].size = size;
                break;
            }

            case 2: { // truncate, growing or shrinking
                if (!model[slot].exists) {
                    break;
                }
                const uint32_t size = rng.below(kMaxSize);
                filehandle *fh = nullptr;
                ASSERT_EQ(NO_ERROR, fs_open_file(path, &fh));
                auto close_fh = lk::make_auto_call([&]() { fs_close_file(fh); });
                ASSERT_EQ(NO_ERROR, fs_truncate_file(fh, size));
                if (size > model[slot].size) {
                    // the grown region reads as zeroes, which the pattern model
                    // cannot express, so rewrite the whole thing instead
                    const uint32_t new_seed = rng.next();
                    if (size > 0) {
                        fill_pattern(buf, size, new_seed, 0);
                        ASSERT_EQ((ssize_t)size, fs_write_file(fh, buf, 0, size));
                    }
                    model[slot].seed = new_seed;
                }
                model[slot].size = size;
                break;
            }

            case 3: // remove
                if (model[slot].exists) {
                    ASSERT_EQ(NO_ERROR, fs_remove_file(path));
                    model[slot].exists = false;
                    model[slot].size = 0;
                } else {
                    EXPECT_EQ(ERR_NOT_FOUND, fs_remove_file(path));
                }
                break;

            case 4: // verify one slot against the model
                if (!verify_slot(slot, "mid-run")) {
                    all_ok = false;
                }
                break;
        }

        // periodically round trip through the device and re-verify everything
        if (all_ok && (op % 97) == 96) {
            ASSERT_EQ(NO_ERROR, vol.remount());
            for (int i = 0; i < kSlots; i++) {
                if (!verify_slot(i, "after remount")) {
                    all_ok = false;
                    break;
                }
            }
        }
    }

    // final full verification, once more across a remount
    if (all_ok) {
        ASSERT_EQ(NO_ERROR, vol.remount());
        for (int i = 0; i < kSlots; i++) {
            if (!verify_slot(i, "final")) {
                all_ok = false;
            }
        }
    }

    if (!all_ok) {
        unittest_printf("\n        reproduce with -A test.fat.stress.seed=%u "
                        "-A test.fat.stress.ops=%u\n", seed, ops);
    }

    for (int i = 0; i < kSlots; i++) {
        if (model[i].exists) {
            slot_path(i);
            fs_remove_file(path);
        }
    }

    END_TEST;
}

} // anonymous namespace

BEGIN_TEST_CASE(fat_stress)
RUN_TEST(test_fat_stress_remount)
RUN_TEST(test_fat_stress_enospc)
RUN_TEST(test_fat_stress_fragmentation)
RUN_TEST(test_fat_stress_boundaries)
RUN_TEST(test_fat_stress_dir_slot_reuse)
RUN_TEST(test_fat_stress_random_ops)
END_TEST_CASE(fat_stress)
