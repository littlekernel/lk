/*
 * Copyright (c) 2009-2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/fs.h>

#include <string.h>
#include <string.h>
#include <stdio.h>

#if WITH_LIB_UNITTEST
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

    END_TEST;
}

BEGIN_TEST_CASE(fs_tests);
RUN_TEST(test_path_normalize);
END_TEST_CASE(fs_tests);

#endif // WITH_LIB_UNITTEST
