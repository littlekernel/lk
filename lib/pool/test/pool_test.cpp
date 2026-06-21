// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include <lib/pool.h>
#include <lib/unittest.h>
#include <lk/cpp.h>
#include <stdint.h>

namespace {

bool test_pool_basic() {
    BEGIN_TEST;

    uint64_t *test_storage = new uint64_t[256];
    auto ac = lk::make_auto_call([&]() { delete[] test_storage; });

    pool_t pool{};
    pool_init(&pool, 8, 8, 3, test_storage);

    // First 3 allocations should succeed.
    void *i0 = pool_alloc(&pool);
    EXPECT_NONNULL(i0, "first allocation should not be null");
    EXPECT_EQ((uintptr_t)0, (uintptr_t)i0 % 8, "alignment check");

    void *i1 = pool_alloc(&pool);
    EXPECT_NONNULL(i1, "second allocation should not be null");
    EXPECT_EQ((uintptr_t)0, (uintptr_t)i1 % 8, "alignment check");

    void *i2 = pool_alloc(&pool);
    EXPECT_NONNULL(i2, "third allocation should not be null");
    EXPECT_EQ((uintptr_t)0, (uintptr_t)i2 % 8, "alignment check");

    // All objects need to be different.
    EXPECT_NE(i0, i1, "first and second allocations should be different");
    EXPECT_NE(i0, i2, "first and third allocations should be different");
    EXPECT_NE(i1, i2, "second and third allocations should be different");

    // Next allocation should fail.
    void *i3 = pool_alloc(&pool);
    EXPECT_NULL(i3, "fourth allocation should be null");

    // But after we free something it should succeed.
    pool_free(&pool, i0);
    i3 = pool_alloc(&pool);
    EXPECT_NONNULL(i3, "allocation after free should succeed");
    EXPECT_EQ((uintptr_t)0, (uintptr_t)i3 % 8, "alignment check");

    END_TEST;
}

bool test_pool_storage_align() {
    BEGIN_TEST;

    // Storage alignment should be at least the max of pointer alignment and object alignment
    constexpr size_t ptr_align = __alignof(void *);

    // When object_align is smaller than pointer alignment, should use pointer alignment
    const size_t align1 = pool_storage_align(1, 1);
    EXPECT_EQ(ptr_align, align1, "alignment should be at least pointer alignment");

    // When object_align equals pointer alignment
    const size_t align2 = pool_storage_align(8, ptr_align);
    EXPECT_EQ(ptr_align, align2, "alignment should match pointer alignment");

    // When object_align is larger than pointer alignment
    const size_t align3 = pool_storage_align(64, 64);
    EXPECT_EQ(64UL, align3, "alignment should match requested alignment when larger");

    // Alignment should always be a power of 2 and consistent
    const size_t align4 = pool_storage_align(16, 32);
    EXPECT_EQ(32UL, align4, "alignment should use larger value");

    END_TEST;
}

bool test_pool_padded_object_size() {
    BEGIN_TEST;

    // Padded size should be at least the object size, padded to alignment boundary
    const size_t padded1 = pool_padded_object_size(8, 8);
    EXPECT_GE(padded1, 8UL, "padded size should be at least object size");
    EXPECT_EQ((uintptr_t)0, padded1 % pool_storage_align(8, 8), "padded size should be aligned");

    // Small object size should be padded up to at least pointer size
    const size_t padded2 = pool_padded_object_size(1, 1);
    const size_t min_size = MAX(sizeof(void *), (size_t)1);
    EXPECT_GE(padded2, min_size, "padded size should be at least pointer size");

    // Larger object with strict alignment
    const size_t padded3 = pool_padded_object_size(100, 32);
    const size_t align3 = pool_storage_align(100, 32);
    EXPECT_EQ((uintptr_t)0, padded3 % align3, "padded size should respect alignment");

    // Multiple small objects should not overlap
    const size_t padded4 = pool_padded_object_size(4, 4);
    EXPECT_GE(padded4, (size_t)4, "padded size for 4-byte object should be at least 4");

    END_TEST;
}

bool test_pool_storage_size() {
    BEGIN_TEST;

    // Total storage should be count * padded_size
    const size_t object_size = 8;
    const size_t object_align = 8;
    const size_t count = 10;

    const size_t padded = pool_padded_object_size(object_size, object_align);
    const size_t total = pool_storage_size(object_size, object_align, count);

    EXPECT_EQ(padded * count, total, "storage size should be count times padded object size");

    // Zero count should give zero size
    const size_t zero_count_size = pool_storage_size(8, 8, 0);
    EXPECT_EQ(0UL, zero_count_size, "storage size should be 0 for 0 objects");

    // Larger counts should scale linearly
    const size_t size_100 = pool_storage_size(16, 16, 100);
    const size_t size_50 = pool_storage_size(16, 16, 50);
    EXPECT_EQ(size_100, size_50 * 2, "storage size should scale linearly with count");

    // Small objects with large alignment
    const size_t small_obj_size = pool_storage_size(1, 1, 5);
    EXPECT_GE(small_obj_size, (size_t)5, "storage for 5 1-byte objects should be reasonable");

    END_TEST;
}

bool test_pool_storage_size_define_macro() {
    BEGIN_TEST;

    // Verify that pool_storage_size and pool_storage_align work correctly
    // by computing the expected sizes and alignments
    const size_t expected_align = pool_storage_align(64, 64);
    const size_t expected_size = pool_storage_size(64, 64, 10);

    // Alignment should be 64 (or the max of pointer alignment and 64)
    EXPECT_GE(expected_align, 8UL, "alignment should be at least 8");

    // Size should be at least 10 * 64 = 640 (with some padding)
    EXPECT_GE(expected_size, 640UL, "storage size should be at least 640 bytes");

    // The calculated size should be a multiple of the alignment
    EXPECT_EQ((uintptr_t)0, expected_size % expected_align,
              "storage size should be aligned to alignment boundary");

    END_TEST;
}

BEGIN_TEST_CASE(pool_tests);
RUN_TEST(test_pool_basic);
RUN_TEST(test_pool_storage_align);
RUN_TEST(test_pool_padded_object_size);
RUN_TEST(test_pool_storage_size);
RUN_TEST(test_pool_storage_size_define_macro);
END_TEST_CASE(pool_tests);

} // namespace
