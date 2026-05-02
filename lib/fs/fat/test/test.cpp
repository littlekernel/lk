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

#include "../dir.h"

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

bool is_test_device_present() {
    static bool checked = false;
    static bool present = false;

    if (!checked) {
        checked = true;
        auto bio = bio_open(test_device_name);
        if (bio) {
            present = true;
            bio_close(bio);
        }
    }
    return present;
}

#define SKIP_TEST_IF_NO_DEVICE()                                                          \
    do {                                                                                  \
        if (!is_test_device_present()) {                                                  \
            unittest_printf(" no device '%s' present, skipping test ", test_device_name); \
            return true;                                                                  \
        }                                                                                 \
    } while (0)

// helper routine that mounts the above in the /fat path and then cleans up on
// the way out.
template <typename R>
bool test_mount_wrapper(R routine) {
    BEGIN_TEST;

    SKIP_TEST_IF_NO_DEVICE();

    ASSERT_EQ(NO_ERROR, fs_mount(test_path, "fat", test_device_name));
    // clean up by unmounting no matter what happens here
    auto unmount_cleanup = lk::make_auto_call([]() { fs_unmount(test_path); });

    // all through to the inner routine
    all_ok = routine();
    if (!all_ok) {
        END_TEST;
    }

    // unmount the fs
    unmount_cleanup.cancel();
    ASSERT_EQ(NO_ERROR, fs_unmount(test_path));

    END_TEST;
}
bool test_fat_mount() {
    BEGIN_TEST;

    SKIP_TEST_IF_NO_DEVICE();

    ASSERT_EQ(NO_ERROR, fs_mount(test_path, "fat", test_device_name));
    ASSERT_EQ(NO_ERROR, fs_unmount(test_path));

    END_TEST;
}

bool test_fat_utf8_to_ucs2() {
    BEGIN_TEST;

    uint16_t out[16] = {};
    size_t out_len = 0;

    ASSERT_EQ(NO_ERROR, fat_utf8_to_ucs2("hello", out, countof(out), &out_len));
    ASSERT_EQ(5u, out_len);
    EXPECT_EQ(static_cast<uint16_t>('h'), out[0]);
    EXPECT_EQ(static_cast<uint16_t>('e'), out[1]);
    EXPECT_EQ(static_cast<uint16_t>('l'), out[2]);
    EXPECT_EQ(static_cast<uint16_t>('l'), out[3]);
    EXPECT_EQ(static_cast<uint16_t>('o'), out[4]);

    // U+00E9 and U+20AC
    ASSERT_EQ(NO_ERROR, fat_utf8_to_ucs2("\xC3\xA9\xE2\x82\xAC", out, countof(out), &out_len));
    ASSERT_EQ(2u, out_len);
    EXPECT_EQ(0x00e9u, out[0]);
    EXPECT_EQ(0x20acu, out[1]);

    // Overlong encoding for '/'
    EXPECT_EQ(ERR_INVALID_ARGS, fat_utf8_to_ucs2("\xC0\xAF", out, countof(out), &out_len));

    // Truncated multibyte sequence
    EXPECT_EQ(ERR_INVALID_ARGS, fat_utf8_to_ucs2("\xE2\x82", out, countof(out), &out_len));

    // UTF-8 surrogate (invalid scalar value)
    EXPECT_EQ(ERR_INVALID_ARGS, fat_utf8_to_ucs2("\xED\xA0\x80", out, countof(out), &out_len));

    // Non-BMP (4-byte UTF-8): unsupported by UCS-2
    EXPECT_EQ(ERR_INVALID_ARGS, fat_utf8_to_ucs2("\xF0\x9F\x98\x80", out, countof(out), &out_len));

    uint16_t small[1] = {};
    EXPECT_EQ(ERR_TOO_BIG, fat_utf8_to_ucs2("ab", small, countof(small), &out_len));

    END_TEST;
}

bool test_fat_dir_root() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

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

        END_TEST;
    });
}

// helper routine for the read file test routine below
bool test_file_read(const char *path, const unsigned char *test_file_buffer, size_t test_file_size) {
    BEGIN_TEST;

    SKIP_TEST_IF_NO_DEVICE();

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
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        // read in a few files and validate their contents
        EXPECT_TRUE(test_file_read(test_path "/hello.txt", test_file_hello, test_file_hello_size));
        EXPECT_TRUE(test_file_read(test_path "/license", test_file_license, test_file_license_size));
        EXPECT_TRUE(test_file_read(test_path "/long_filename_hello.txt", test_file_hello, test_file_hello_size));
        EXPECT_TRUE(test_file_read(test_path "/a_very_long_filename_hello_that_uses_at_least_a_few_entries.txt", test_file_hello, test_file_hello_size));
        EXPECT_TRUE(test_file_read(test_path "/dir.a/long_filename_hello.txt", test_file_hello, test_file_hello_size));

        END_TEST;
    });
}

bool test_fat_multi_open() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

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

        END_TEST;
    });
}

bool test_fat_create_file() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        filehandle *handle;

        // create a few empty files
        handle = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/newfile", &handle, 0));
        ASSERT_NONNULL(handle);
        ASSERT_EQ(NO_ERROR, fs_close_file(handle));

        handle = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/newfile.txt", &handle, 0));
        ASSERT_NONNULL(handle);
        ASSERT_EQ(NO_ERROR, fs_close_file(handle));

        // create a file in a subdir
        handle = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/dir.a/newfile", &handle, 0));
        ASSERT_NONNULL(handle);
        ASSERT_EQ(NO_ERROR, fs_close_file(handle));

        // create a file that already exists
        handle = nullptr;
        ASSERT_EQ(ERR_ALREADY_EXISTS, fs_create_file(test_path "/newfile", &handle, 0));

        // create a long filename (LFN path)
        handle = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/this_is_a_long_filename_for_create.txt", &handle, 0));
        ASSERT_NONNULL(handle);
        ASSERT_EQ(NO_ERROR, fs_close_file(handle));

        handle = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_file(test_path "/this_is_a_long_filename_for_create.txt", &handle));
        ASSERT_NONNULL(handle);
        ASSERT_EQ(NO_ERROR, fs_close_file(handle));

        END_TEST;
    });
}

bool test_fat_resize_file() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        filehandle *handle;

        // create an empty file
        handle = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/reszfile", &handle, 0));
        ASSERT_NONNULL(handle);
        auto closefile_cleanup1 = lk::make_auto_call([&]() { fs_close_file(handle); });

        // resize the file
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 0));                           // same size
        EXPECT_EQ(ERR_TOO_BIG, fs_truncate_file(handle, 2UL * 1024 * 1024 * 1024)); // too big for FAT
        EXPECT_EQ(ERR_TOO_BIG, fs_truncate_file(handle, 8UL * 1024 * 1024 * 1024)); // >32bit too big for FAT
        EXPECT_EQ(ERR_TOO_BIG, fs_truncate_file(handle, -1));                       // negative should produce way out of range
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 1));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 4095)); // assumes cluster size 4k
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 4096));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 4097));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 12345));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 1002345));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 65536));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 4097));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 4096));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 1));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 0));
        EXPECT_EQ(NO_ERROR, fs_truncate_file(handle, 8192));

        END_TEST;
    });
}

bool test_fat_write_file() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        filehandle *handle = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/wrfile", &handle, 0));
        ASSERT_NONNULL(handle);
        auto closefile_cleanup = lk::make_auto_call([&]() { fs_close_file(handle); });

        const uint8_t head[] = {'a', 'b', 'c', 'd', 'e'};
        ASSERT_EQ((ssize_t)sizeof(head), fs_write_file(handle, head, 0, sizeof(head)));

        uint8_t readback[sizeof(head)] = {};
        ASSERT_EQ((ssize_t)sizeof(readback), fs_read_file(handle, readback, 0, sizeof(readback)));
        EXPECT_EQ(0, memcmp(head, readback, sizeof(head)));

        const uint8_t tail[] = {'L', 'K'};
        ASSERT_EQ((ssize_t)sizeof(tail), fs_write_file(handle, tail, 8192, sizeof(tail)));

        struct file_stat st = {};
        ASSERT_EQ(NO_ERROR, fs_stat_file(handle, &st));
        EXPECT_EQ(8194u, st.size);

        uint8_t gap[32];
        memset(gap, 0xaa, sizeof(gap));
        ASSERT_EQ((ssize_t)sizeof(gap), fs_read_file(handle, gap, 5, sizeof(gap)));
        for (size_t i = 0; i < sizeof(gap); i++) {
            EXPECT_EQ(0u, gap[i]);
        }

        uint8_t tail_read[6];
        memset(tail_read, 0xaa, sizeof(tail_read));
        ASSERT_EQ((ssize_t)sizeof(tail_read), fs_read_file(handle, tail_read, 8188, sizeof(tail_read)));
        EXPECT_EQ(0u, tail_read[0]);
        EXPECT_EQ(0u, tail_read[1]);
        EXPECT_EQ(0u, tail_read[2]);
        EXPECT_EQ(0u, tail_read[3]);
        EXPECT_EQ('L', tail_read[4]);
        EXPECT_EQ('K', tail_read[5]);

        END_TEST;
    });
}

bool test_fat_mkdir() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        ASSERT_EQ(NO_ERROR, fs_make_dir(test_path "/newdir"));
        ASSERT_EQ(ERR_ALREADY_EXISTS, fs_make_dir(test_path "/newdir"));

        ASSERT_EQ(NO_ERROR, fs_make_dir(test_path "/parent"));
        ASSERT_EQ(NO_ERROR, fs_make_dir(test_path "/parent/child"));

        dirhandle *dh = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_dir(test_path "/newdir", &dh));
        ASSERT_NONNULL(dh);
        ASSERT_EQ(NO_ERROR, fs_close_dir(dh));

        filehandle *fh = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/newdir/file", &fh, 0));
        ASSERT_NONNULL(fh);
        ASSERT_EQ(NO_ERROR, fs_close_file(fh));

        ASSERT_EQ(ERR_NOT_FOUND, fs_make_dir(test_path "/does_not_exist/child"));

        ASSERT_EQ(NO_ERROR, fs_make_dir(test_path "/this_is_a_long_directory_name"));
        ASSERT_EQ(NO_ERROR, fs_open_dir(test_path "/this_is_a_long_directory_name", &dh));
        ASSERT_NONNULL(dh);
        ASSERT_EQ(NO_ERROR, fs_close_dir(dh));

        fh = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/this_is_a_long_directory_name/inside.txt", &fh, 0));
        ASSERT_NONNULL(fh);
        ASSERT_EQ(NO_ERROR, fs_close_file(fh));

        END_TEST;
    });
}

bool test_fat_remove_file() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        filehandle *fh = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/rmfile", &fh, 0));
        ASSERT_NONNULL(fh);
        ASSERT_EQ(NO_ERROR, fs_close_file(fh));

        ASSERT_EQ(NO_ERROR, fs_remove_file(test_path "/rmfile"));

        fh = nullptr;
        ASSERT_EQ(ERR_NOT_FOUND, fs_open_file(test_path "/rmfile", &fh));
        ASSERT_EQ(ERR_NOT_FOUND, fs_remove_file(test_path "/rmfile"));

        ASSERT_EQ(NO_ERROR, fs_remove_file(test_path "/long_filename_hello.txt"));
        ASSERT_EQ(ERR_NOT_FOUND, fs_open_file(test_path "/long_filename_hello.txt", &fh));

        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/busyfile", &fh, 0));
        ASSERT_NONNULL(fh);
        ASSERT_EQ(ERR_BUSY, fs_remove_file(test_path "/busyfile"));
        ASSERT_EQ(NO_ERROR, fs_close_file(fh));
        ASSERT_EQ(NO_ERROR, fs_remove_file(test_path "/busyfile"));

        ASSERT_EQ(NO_ERROR, fs_make_dir(test_path "/rmdirtgt"));
        ASSERT_EQ(ERR_NOT_FILE, fs_remove_file(test_path "/rmdirtgt"));

        END_TEST;
    });
}

bool test_fat_remove_dir() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        ASSERT_EQ(NO_ERROR, fs_make_dir(test_path "/emptydir"));
        ASSERT_EQ(NO_ERROR, fs_remove_dir(test_path "/emptydir"));

        dirhandle *dh = nullptr;
        ASSERT_EQ(ERR_NOT_FOUND, fs_open_dir(test_path "/emptydir", &dh));
        ASSERT_EQ(ERR_NOT_FOUND, fs_remove_dir(test_path "/emptydir"));

        ASSERT_EQ(NO_ERROR, fs_make_dir(test_path "/nonempty"));
        filehandle *fh = nullptr;
        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/nonempty/file", &fh, 0));
        ASSERT_NONNULL(fh);
        ASSERT_EQ(NO_ERROR, fs_close_file(fh));
        ASSERT_EQ(ERR_NOT_ALLOWED, fs_remove_dir(test_path "/nonempty"));
        ASSERT_EQ(NO_ERROR, fs_remove_file(test_path "/nonempty/file"));
        ASSERT_EQ(NO_ERROR, fs_remove_dir(test_path "/nonempty"));

        ASSERT_EQ(NO_ERROR, fs_create_file(test_path "/plainfl", &fh, 0));
        ASSERT_NONNULL(fh);
        ASSERT_EQ(NO_ERROR, fs_close_file(fh));
        ASSERT_EQ(ERR_NOT_DIR, fs_remove_dir(test_path "/plainfl"));
        ASSERT_EQ(NO_ERROR, fs_remove_file(test_path "/plainfl"));

        ASSERT_EQ(NO_ERROR, fs_make_dir(test_path "/busy"));
        ASSERT_EQ(NO_ERROR, fs_open_dir(test_path "/busy", &dh));
        ASSERT_NONNULL(dh);
        ASSERT_EQ(ERR_BUSY, fs_remove_dir(test_path "/busy"));
        ASSERT_EQ(NO_ERROR, fs_close_dir(dh));
        ASSERT_EQ(NO_ERROR, fs_remove_dir(test_path "/busy"));

        END_TEST;
    });
}

bool test_fat_dir_growth() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        const char *dirname = test_path "/growdir";
        ASSERT_EQ(NO_ERROR, fs_make_dir(dirname));

        // Create enough files to force directory growth beyond one cluster.
        // Assuming 4KB clusters and 32-byte entries, one cluster holds 128 entries.
        // Creating 1000 files should force growth.
        for (int i = 0; i < 1000; i++) {
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/f%03d", dirname, i);
            filehandle *fh = nullptr;
            ASSERT_EQ(NO_ERROR, fs_create_file(filename, &fh, 0));
            ASSERT_NONNULL(fh);
            ASSERT_EQ(NO_ERROR, fs_close_file(fh));
        }

        // Verify all 1000 files exist.
        for (int i = 0; i < 1000; i++) {
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/f%03d", dirname, i);
            filehandle *fh = nullptr;
            ASSERT_EQ(NO_ERROR, fs_open_file(filename, &fh));
            ASSERT_NONNULL(fh);
            ASSERT_EQ(NO_ERROR, fs_close_file(fh));
        }

        // List the directory and count the entries (should be 1002: 1000 files + . + ..).
        dirhandle *dh = nullptr;
        ASSERT_EQ(NO_ERROR, fs_open_dir(dirname, &dh));
        ASSERT_NONNULL(dh);
        int count = 0;
        dirent ent;
        while (fs_read_dir(dh, &ent) == NO_ERROR) {
            count++;
        }
        ASSERT_EQ(NO_ERROR, fs_close_dir(dh));
        EXPECT_EQ(1002, count);

        // Remove all files.
        for (int i = 0; i < 1000; i++) {
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/f%03d", dirname, i);
            ASSERT_EQ(NO_ERROR, fs_remove_file(filename));
        }

        // Finally remove the directory.
        ASSERT_EQ(NO_ERROR, fs_remove_dir(dirname));

        END_TEST;
    });
}

bool test_fat_lfn_ordinal_rollover() {
    return test_mount_wrapper([]() {
        BEGIN_TEST;

        // Test SFN alias ordinal rollover with multiple files that collide on their base SFN.
        filehandle *fh = nullptr;
        char *filename_buf = new char[256];

        // Create 10 files with colliding long names to verify ordinal generation
        for (int i = 0; i < 10; i++) {
            snprintf(filename_buf, 256, test_path "/this_is_a_collision_test_file_%02d.txt", i);
            fh = nullptr;
            ASSERT_EQ(NO_ERROR, fs_create_file(filename_buf, &fh, 0));
            ASSERT_NONNULL(fh);
            ASSERT_EQ(NO_ERROR, fs_close_file(fh));

            // Verify immediately after creation
            fh = nullptr;
            ASSERT_EQ(NO_ERROR, fs_open_file(filename_buf, &fh));
            ASSERT_NONNULL(fh);
            ASSERT_EQ(NO_ERROR, fs_close_file(fh));
        }

        // Clean up all files
        for (int i = 0; i < 10; i++) {
            snprintf(filename_buf, 256, test_path "/this_is_a_collision_test_file_%02d.txt", i);
            int ret = fs_remove_file(filename_buf);
            if (ret != NO_ERROR) {
                unittest_printf("FAILED to remove %s: %d\n", filename_buf, ret);
            } else {
                unittest_printf("Successfully removed %s\n", filename_buf);
            }
            ASSERT_EQ(NO_ERROR, ret);
        }

        delete[] filename_buf;

        END_TEST;
    });
}

BEGIN_TEST_CASE(fat)
RUN_TEST(test_fat_mount)
RUN_TEST(test_fat_utf8_to_ucs2)
RUN_TEST(test_fat_dir_root)
RUN_TEST(test_fat_read_file)
RUN_TEST(test_fat_multi_open)
RUN_TEST(test_fat_create_file)
RUN_TEST(test_fat_resize_file)
RUN_TEST(test_fat_write_file)
RUN_TEST(test_fat_mkdir)
RUN_TEST(test_fat_remove_file)
RUN_TEST(test_fat_remove_dir)
RUN_TEST(test_fat_dir_growth)
RUN_TEST(test_fat_lfn_ordinal_rollover)
END_TEST_CASE(fat)

} // namespace
