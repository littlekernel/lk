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

#define LOCAL_TRACE 0

// A set of test cases run against a block device image created from the test script
// in the same directory as this. It should contain a set of known directories and
// files for the test to work with.

// pull in a few test files into rodata to test against
INCFILE(test_file_hello, test_file_hello_size, LOCAL_DIR "/hello.txt");
INCFILE(test_file_license, test_file_license_size, LOCAL_DIR "/LICENSE");

namespace {

// TODO: make this much less hard coded
const char *test_device_name = "virtio0";
#define test_path "/fat"

bool test_fat_mount() {
    BEGIN_TEST;

    LTRACEF("mounting filesystem on device '%s'\n", test_device_name);

    ASSERT_EQ(NO_ERROR, fs_mount(test_path, "fat", test_device_name));
    ASSERT_EQ(NO_ERROR, fs_unmount(test_path));

    END_TEST;
}

bool test_fat_dir_root() {
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

// helper routine for the read file test routine below
bool test_file_read(const char *path, const unsigned char *test_file_buffer, size_t test_file_size) {
    BEGIN_TEST;

    // open the file
    filehandle *handle = nullptr;
    ASSERT_EQ(NO_ERROR, fs_open_file(path, &handle));
    auto closefile_cleanup = lk::make_auto_call([&]() { fs_close_file(handle); });

    ASSERT_NONNULL(handle);

    const size_t buflen = test_file_size * 2; // should be somewhat larger than test_file_size
    char *buf = new char[buflen];
    auto delete_buffer = lk::make_auto_call([&]() { delete[] buf; });

    // try to read the file in and make sure it reads exactly the target size of bytes
    ssize_t read_len = fs_read_file(handle, buf, 0, buflen);
    ASSERT_LT(0, read_len);
    ASSERT_EQ(test_file_size, (size_t)read_len);

    EXPECT_EQ(0, memcmp(buf, test_file_buffer, read_len));
    if (memcmp(buf, test_file_buffer, read_len)) {
        printf("\nfailure in comparison\nexpected:\n");
        hexdump8(test_file_buffer, read_len);
        printf("read:\n");
        hexdump8(buf, read_len);
    }

    // close the file
    closefile_cleanup.cancel();
    ASSERT_EQ(NO_ERROR, fs_close_file(handle));

    END_TEST;
}

bool test_fat_read_file() {
    BEGIN_TEST;

    ASSERT_EQ(NO_ERROR, fs_mount(test_path, "fat", test_device_name));
    // clean up by unmounting no matter what happens here
    auto unmount_cleanup = lk::make_auto_call([]() { fs_unmount(test_path); });

    // read in a few files and validate their contents
    EXPECT_TRUE(test_file_read(test_path "/hello.txt", test_file_hello, test_file_hello_size));
    EXPECT_TRUE(test_file_read(test_path "/license", test_file_license, test_file_license_size));
    EXPECT_TRUE(test_file_read(test_path "/long_filename_hello.txt", test_file_hello, test_file_hello_size));
    EXPECT_TRUE(test_file_read(test_path "/a_very_long_filename_hello_that_uses_at_least_a_few_entries.txt", test_file_hello, test_file_hello_size));
    EXPECT_TRUE(test_file_read(test_path "/dir.a/long_filename_hello.txt", test_file_hello, test_file_hello_size));

    // unmount the fs
    unmount_cleanup.cancel();
    ASSERT_EQ(NO_ERROR, fs_unmount(test_path));

    END_TEST;
}

bool test_fat_multi_open() {
    BEGIN_TEST;

    ASSERT_EQ(NO_ERROR, fs_mount(test_path, "fat", test_device_name));
    // clean up by unmounting no matter what happens here
    auto unmount_cleanup = lk::make_auto_call([]() { fs_unmount(test_path); });

    // open a file three times simultaneously
    {
        filehandle *handle1 = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_file(test_path "/hello.txt", &handle1));
        auto closefile_cleanup1 = lk::make_auto_call([&]() { fs_close_file(handle1); });

        filehandle *handle2 = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_file(test_path "/hello.txt", &handle2));
        auto closefile_cleanup2 = lk::make_auto_call([&]() { fs_close_file(handle2); });

        filehandle *handle3 = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_file(test_path "/hello.txt", &handle3));

        // close the files in reverse order
        closefile_cleanup1.cancel();
        ASSERT_EQ(NO_ERROR, fs_close_file(handle1));
        closefile_cleanup2.cancel();
        ASSERT_EQ(NO_ERROR, fs_close_file(handle2));
        ASSERT_EQ(NO_ERROR, fs_close_file(handle3));
    }

    // open a dir three times simultaneously
    {
        dirhandle *handle1 = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_dir(test_path "/dir.a", &handle1));
        auto closedir_cleanup1 = lk::make_auto_call([&]() { fs_close_dir(handle1); });

        dirhandle *handle2 = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_dir(test_path "/dir.a", &handle2));
        auto closedir_cleanup2 = lk::make_auto_call([&]() { fs_close_dir(handle2); });

        dirhandle *handle3 = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_dir(test_path "/dir.a", &handle3));

        // close the dirs in reverse order
        closedir_cleanup1.cancel();
        ASSERT_EQ(NO_ERROR, fs_close_dir(handle1));
        closedir_cleanup2.cancel();
        ASSERT_EQ(NO_ERROR, fs_close_dir(handle2));
        ASSERT_EQ(NO_ERROR, fs_close_dir(handle3));
    }

    // unmount the fs
    unmount_cleanup.cancel();
    ASSERT_EQ(NO_ERROR, fs_unmount(test_path));

    END_TEST;
}

BEGIN_TEST_CASE(fat)
    RUN_TEST(test_fat_mount)
    RUN_TEST(test_fat_dir_root)
    RUN_TEST(test_fat_read_file)
    RUN_TEST(test_fat_multi_open)
END_TEST_CASE(fat)

} // namespace

