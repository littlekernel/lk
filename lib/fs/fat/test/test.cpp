/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <endian.h>
#include <lib/bio.h>
#include <lib/fs.h>
#include <lib/unittest.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <malloc.h>
#include <string.h>

#define LOCAL_TRACE 1

// A set of test cases run against a block device image created from the test script
// in the same directory as this. It should contain a set of known directories and
// files for the test to work with.

namespace {

// TODO: make this much less hard coded
const char *test_device_name = "virtio0";
#define test_path "/fat"

static bool fat_mount() {
    BEGIN_TEST;

    LTRACEF("mounting filesystem on device '%s'\n", test_device_name);

    ASSERT_EQ(NO_ERROR, fs_mount(test_path, "fat", test_device_name));
    ASSERT_EQ(NO_ERROR, fs_unmount(test_path));

    END_TEST;
}

static bool fat_dir_root() {
    BEGIN_TEST;

    ASSERT_EQ(NO_ERROR, fs_mount(test_path, "fat", test_device_name));

    // clean up by unmounting no matter what happens here
    auto unmount_cleanup = lk::make_auto_call([]() { fs_unmount(test_path); });

    // open and then close the root dir
    dirhandle *handle;
    ASSERT_EQ(NO_ERROR, fs_open_dir(test_path, &handle));
    ASSERT_NONNULL(handle);
    ASSERT_EQ(NO_ERROR, fs_close_dir(handle));

    // open it again
    ASSERT_EQ(NO_ERROR, fs_open_dir(test_path, &handle));
    ASSERT_NONNULL(handle);

    // close the dir handle if we abort from here on out
    auto closedir_cleanup = lk::make_auto_call([&]() { fs_close_dir(handle); });

    // read an entry
    dirent ent;
    ASSERT_EQ(NO_ERROR, fs_read_dir(handle, &ent));
    LTRACEF("read entry '%s'\n", ent.name);

    // read all of the entries until we hit an EOD
    int count = 1;
    for (;;) {
        auto err = fs_read_dir(handle, &ent);
        bool valid = (err == NO_ERROR || err == ERR_NOT_FOUND);
        ASSERT_TRUE(valid);
        count++;
        if (err == ERR_NOT_FOUND) {
            break;
        }
        LTRACEF("read entry '%s'\n", ent.name);
    }
    // make sure we saw at least 3 entries
    ASSERT_LT(2, count);

    closedir_cleanup.cancel();
    ASSERT_EQ(NO_ERROR, fs_close_dir(handle));

    unmount_cleanup.cancel();
    ASSERT_EQ(NO_ERROR, fs_unmount(test_path));

    END_TEST;
}

BEGIN_TEST_CASE(fat)
    RUN_TEST(fat_mount)
    RUN_TEST(fat_dir_root)
END_TEST_CASE(fat)

} // namespace

