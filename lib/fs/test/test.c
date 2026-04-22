/*
 * Copyright (c) 2009-2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/fs.h>
#include <lk/err.h>

#include <string.h>
#include <string.h>
#include <stdio.h>
#include <lib/unittest.h>

// returns true if the input path passed through the path normalization
// routine matches the expected output.
static bool test_normalize(const char *in, const char *out) {
    char path[1024];

    strlcpy(path, in, sizeof(path));
    fs_normalize_path(path);
    return !strcmp(path, out);
}

static bool test_path_normalize(void) {
    BEGIN_TEST;

    EXPECT_TRUE(test_normalize("/", ""), "");
    EXPECT_TRUE(test_normalize("/test", "/test"), "");
    EXPECT_TRUE(test_normalize("/test/", "/test"), "");
    EXPECT_TRUE(test_normalize("test/", "test"), "");
    EXPECT_TRUE(test_normalize("test", "test"), "");
    EXPECT_TRUE(test_normalize("/test//", "/test"), "");
    EXPECT_TRUE(test_normalize("/test/foo", "/test/foo"), "");
    EXPECT_TRUE(test_normalize("/test/foo/", "/test/foo"), "");
    EXPECT_TRUE(test_normalize("/test/foo/bar", "/test/foo/bar"), "");
    EXPECT_TRUE(test_normalize("/test/foo/bar//", "/test/foo/bar"), "");
    EXPECT_TRUE(test_normalize("/test//foo/bar//", "/test/foo/bar"), "");
    EXPECT_TRUE(test_normalize("/test//./foo/bar//", "/test/foo/bar"), "");
    EXPECT_TRUE(test_normalize("/test//./.foo/bar//", "/test/.foo/bar"), "");
    EXPECT_TRUE(test_normalize("/test//./..foo/bar//", "/test/..foo/bar"), "");
    EXPECT_TRUE(test_normalize("/test//./../foo/bar//", "/foo/bar"), "");
    EXPECT_TRUE(test_normalize("/test/../foo", "/foo"), "");
    EXPECT_TRUE(test_normalize("/test/bar/../foo", "/test/foo"), "");
    EXPECT_TRUE(test_normalize("../foo", "foo"), "");
    EXPECT_TRUE(test_normalize("../foo/", "foo"), "");
    EXPECT_TRUE(test_normalize("/../foo", "foo"), "");
    EXPECT_TRUE(test_normalize("/../foo/", "foo"), "");
    EXPECT_TRUE(test_normalize("/../../foo", "foo"), "");
    EXPECT_TRUE(test_normalize("/bleh/../../foo", "foo"), "");
    EXPECT_TRUE(test_normalize("/bleh/bar/../../foo", "/foo"), "");
    EXPECT_TRUE(test_normalize("/bleh/bar/../../foo/..", ""), "");
    EXPECT_TRUE(test_normalize("/bleh/bar/../../foo/../meh", "/meh"), "");

    // edge cases: empty, standalone dots
    EXPECT_TRUE(test_normalize("", ""), "");
    EXPECT_TRUE(test_normalize(".", ""), "");
    EXPECT_TRUE(test_normalize("..", ""), "");
    EXPECT_TRUE(test_normalize("/.", ""), "");
    EXPECT_TRUE(test_normalize("/..", ""), "");

    // three dots is a regular filename
    EXPECT_TRUE(test_normalize("...", "..."), "");
    EXPECT_TRUE(test_normalize("/a/.../b", "/a/.../b"), "");

    // dot-prefixed filenames preserved
    EXPECT_TRUE(test_normalize(".hidden", ".hidden"), "");
    EXPECT_TRUE(test_normalize("/a/.hidden", "/a/.hidden"), "");
    EXPECT_TRUE(test_normalize("..hidden", "..hidden"), "");

    // dotdot at end of path
    EXPECT_TRUE(test_normalize("a/..", ""), "");
    EXPECT_TRUE(test_normalize("/a/..", ""), "");
    EXPECT_TRUE(test_normalize("/a/b/..", "/a"), "");

    // multiple dotdots chained
    EXPECT_TRUE(test_normalize("a/b/../../c", "c"), "");
    EXPECT_TRUE(test_normalize("/a/b/../../c", "/c"), "");
    EXPECT_TRUE(test_normalize("a/b/c/../../d", "a/d"), "");

    // dotdot past root clamped
    EXPECT_TRUE(test_normalize("a/../../b", "b"), "");
    EXPECT_TRUE(test_normalize("/a/b/../../../c", "c"), "");

    // mixed dots and dotdots
    EXPECT_TRUE(test_normalize("/a/./b/../c", "/a/c"), "");
    EXPECT_TRUE(test_normalize("a/./b/./c", "a/b/c"), "");
    EXPECT_TRUE(test_normalize("/a/b/./../c", "/a/c"), "");

    // consecutive separators in various positions
    EXPECT_TRUE(test_normalize("///", ""), "");
    EXPECT_TRUE(test_normalize("a///b", "a/b"), "");

    END_TEST;
}

#define TEST_MNT "/test"
#define TEST_FILE TEST_MNT "/stdio_file_tests.txt"

static inline void test_stdio_fs_teardown(void *ptr) {
    fs_remove_file(TEST_FILE);
    fs_unmount(TEST_MNT);
}

static bool test_stdio_fs(void) {
    __attribute__((cleanup(test_stdio_fs_teardown))) BEGIN_TEST;

    // Setup
    const char *content = "Hello World\n";
    const size_t content_len = strlen(content);
    fs_mount(TEST_MNT, "memfs", NULL);

    // Tests
    FILE *stream = fopen(TEST_FILE, "w");
    ASSERT_NE(NULL, stream, "failed to open/create file " TEST_MNT);

    char buf[1024];
    // test stdout fprintf, fputs
    EXPECT_EQ(content_len, fprintf(stdout, "%s", content), "");
    EXPECT_EQ(content_len, fputs(content, stdout), "");

    // test fwrite
    EXPECT_EQ(content_len, fwrite(content, 1, content_len, stream), "");

    // test fread
    ASSERT_EQ(NO_ERROR, fseek(stream, 0, SEEK_SET), "fseek failed");
    EXPECT_EQ(content_len, fread(buf, 1, content_len, stream), "");
    EXPECT_BYTES_EQ((const uint8_t *)content, (const uint8_t *)buf, content_len,
                    "fread content mismatched");

    // testing fputs
    ASSERT_EQ(NO_ERROR, fseek(stream, 0, SEEK_SET), "fseek failed");
    EXPECT_EQ(content_len, fputs(content, stream), "");

    // testing fgets
    ASSERT_EQ(NO_ERROR, fseek(stream, 0, SEEK_SET), "fseek failed");
    ASSERT_NE(NULL, fgets(buf, content_len + 1, stream), "");
    EXPECT_BYTES_EQ((const uint8_t *)content, (const uint8_t *)buf, content_len,
                    "fgets content mismatched");

    END_TEST;
}

static void test_rootfs_teardown(void *ptr) {
    fs_unmount("/tmp");
    fs_unmount("/data");
}

static bool test_rootfs(void) {
    __attribute__((cleanup(test_rootfs_teardown))) BEGIN_TEST;

    // root listing must work even before we add any filesystems
    dirhandle *dh;
    ASSERT_EQ(NO_ERROR, fs_open_dir("/", &dh), "open root with no mounts");
    struct dirent ent;
    // drain any pre-existing entries (other tests may have left mounts)
    while (fs_read_dir(dh, &ent) == NO_ERROR) {}
    fs_close_dir(dh);

    // mount two filesystems; both must appear as directories in root listing
    ASSERT_EQ(NO_ERROR, fs_mount("/tmp",  "memfs", NULL), "mount /tmp");
    ASSERT_EQ(NO_ERROR, fs_mount("/data", "memfs", NULL), "mount /data");

    ASSERT_EQ(NO_ERROR, fs_open_dir("/", &dh), "open root dir with mounts");
    bool found_tmp = false, found_data = false;
    while (fs_read_dir(dh, &ent) == NO_ERROR) {
        if (strcmp(ent.name, "tmp")  == 0) found_tmp  = true;
        if (strcmp(ent.name, "data") == 0) found_data = true;
    }
    fs_close_dir(dh);
    EXPECT_TRUE(found_tmp,  "tmp in root listing");
    EXPECT_TRUE(found_data, "data in root listing");

    END_TEST;
}

BEGIN_TEST_CASE(fs_tests);
RUN_TEST(test_path_normalize);
RUN_TEST(test_stdio_fs);
RUN_TEST(test_rootfs);
END_TEST_CASE(fs_tests);
