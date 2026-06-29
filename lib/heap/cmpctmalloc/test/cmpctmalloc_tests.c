/*
 * Copyright (c) 2015 Google, Inc. All rights reserved
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "../cmpctmalloc_private.h"

#include <lib/unittest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void WasteFreeMemory(void) {
    while (theheap.remaining != 0) {
        cmpct_alloc(1);
    }
}

// If we just make a big allocation it gets rounded off.  If we actually
// want to use a reasonably accurate amount of memory for test purposes, we
// have to do many small allocations.
static void *TestTrimHelper(ssize_t target) {
    char *answer = NULL;
    size_t remaining = theheap.remaining;
    while (theheap.remaining - target > 512) {
        char *next_block = cmpct_alloc(8 + ((theheap.remaining - target) >> 2));
        *(char **)next_block = answer;
        answer = next_block;
        if (theheap.remaining > remaining) {
            return answer;
        }
        // Abandon attempt to hit particular freelist entry size if we accidentally got more memory
        // from the OS.
        remaining = theheap.remaining;
    }
    return answer;
}

static void TestTrimFreeHelper(char *block) {
    while (block) {
        char *next_block = *(char **)block;
        cmpct_free(block);
        block = next_block;
    }
}

static bool test_cmpct_trim(void) {
    BEGIN_TEST;

    WasteFreeMemory();

    static size_t test_sizes[200];
    int sizes = 0;

    for (size_t s = 1; s < PAGE_SIZE * 4; s = (s + 1) * 1.1) {
        test_sizes[sizes++] = s;
        ASSERT_LT((size_t)sizes, 200u, "test_sizes array overflow");
    }
    for (ssize_t s = -32; s <= 32; s += 8) {
        test_sizes[sizes++] = PAGE_SIZE + s;
        ASSERT_LT((size_t)sizes, 200u, "test_sizes array overflow");
    }

    // Test allocations at the start of an OS allocation.
    for (int with_second_alloc = 0; with_second_alloc < 2; with_second_alloc++) {
        for (int i = 0; i < sizes; i++) {
            size_t s = test_sizes[i];

            char *a, *a2 = NULL;
            a = cmpct_alloc(s);
            if (with_second_alloc) {
                a2 = cmpct_alloc(1);
                if (s < PAGE_SIZE >> 1) {
                    // It is the intention of the test that a is at the start of an OS allocation
                    // and that a2 is "right after" it.  Otherwise we are not testing what I
                    // thought.  OS allocations are certainly not smaller than a page, so check in
                    // that case.
                    ASSERT_LT((uintptr_t)(a2 - a), (uintptr_t)(s * 1.13 + 48),
                              "a2 should be right after a");
                }
            }
            cmpct_trim();
            size_t remaining = theheap.remaining;
            // We should have < 1 page on either side of the a allocation.
            ASSERT_LT(remaining, PAGE_SIZE * 2, "should be less than 2 pages free on both sides");
            cmpct_free(a);
            if (with_second_alloc) {
                // Now only a2 is holding onto the OS allocation.
                ASSERT_GT(theheap.remaining, remaining,
                          "reclaiming first alloc should free some memory");
            } else {
                ASSERT_EQ(0u, theheap.remaining, "all memory should be reclaimed");
            }
            remaining = theheap.remaining;
            cmpct_trim();
            ASSERT_LE(theheap.remaining, remaining, "trim should not increase remaining memory");
            // If a was at least one page then the trim should have freed up that page.
            if (s >= PAGE_SIZE && with_second_alloc) {
                ASSERT_LT(theheap.remaining, remaining, "trim should free page of a");
            }
            if (with_second_alloc) {
                cmpct_free(a2);
            }
        }
        ASSERT_EQ(0u, theheap.remaining, "all memory should be reclaimed at end of pass");
    }

    ASSERT_EQ(0u, theheap.remaining, "heap should be clean at end of trim tests");

    // Now test allocations near the end of an OS allocation.
    for (ssize_t wobble = -64; wobble <= 64; wobble += 8) {
        for (int i = 0; i < sizes; i++) {
            size_t s = test_sizes[i];

            if ((ssize_t)s + wobble < 0) {
                continue;
            }

            char *start_of_os_alloc = cmpct_alloc(1);

            // If the OS allocations are very small this test does not make sense.
            if (theheap.remaining <= s + wobble) {
                cmpct_free(start_of_os_alloc);
                continue;
            }

            char *big_bit_in_the_middle = TestTrimHelper(s + wobble);
            size_t remaining = theheap.remaining;

            // If the remaining is big we started a new OS allocation and the test
            // makes no sense.
            if (remaining > 128 + s * 1.13 + wobble) {
                cmpct_free(start_of_os_alloc);
                TestTrimFreeHelper(big_bit_in_the_middle);
                continue;
            }

            cmpct_free(start_of_os_alloc);
            remaining = theheap.remaining;

            // This trim should sometimes trim a page off the end of the OS allocation.
            cmpct_trim();
            ASSERT_LE(theheap.remaining, remaining, "trim should not increase memory");
            remaining = theheap.remaining;

            // We should have < 1 page on either side of the big allocation.
            ASSERT_LT(remaining, PAGE_SIZE * 2, "should be less than 2 pages free on both sides");

            TestTrimFreeHelper(big_bit_in_the_middle);
        }
    }

    END_TEST;
}

static bool test_cmpct_buckets(void) {
    BEGIN_TEST;

    size_t rounded;
    unsigned bucket;
    // Check for the HEAP_ALIGN-spaced buckets up to log_threshold.
    const size_t log_threshold = 16 * HEAP_ALIGN;
    for (unsigned i = 1; i <= log_threshold; i++) {
        // Round up when allocating.
        bucket = size_to_index_allocating(i, &rounded);
        unsigned expected = (ROUNDUP(i, HEAP_ALIGN) >> HEAP_ALIGN_SHIFT) - 1;
        ASSERT_EQ(expected, bucket, "bucket index mismatch");
        ASSERT_TRUE(IS_ALIGNED(rounded, HEAP_ALIGN), "rounded size should be aligned");
        ASSERT_GE(rounded, i, "rounded size should be >= requested");
        if (i >= sizeof(free_t) - sizeof(header_t)) {
            // Once we get above the size of the free area struct, we
            // won't round up much for these small sizes.
            ASSERT_LT(rounded - i, HEAP_ALIGN, "rounded size is too large");
        }
        // Only rounded sizes are freed.
        if ((i & (HEAP_ALIGN - 1)) == 0) {
            // Up to size log_threshold we have exact buckets for each multiple of HEAP_ALIGN.
            ASSERT_EQ((unsigned)size_to_index_freeing(i), bucket, "freeing index mismatch");
        }
    }
    int bucket_base = 7;
    for (unsigned j = 2 * HEAP_ALIGN; j < 1024 * HEAP_ALIGN / 8; j *= 2, bucket_base += 8) {
        // Note the "<=", which ensures that we test the powers of 2 twice to ensure
        // that both ways of calculating the bucket number match.
        for (unsigned i = j * 8; i <= j * 16; i++) {
            // Round up to j multiple in this range when allocating.
            bucket = size_to_index_allocating(i, &rounded);
            unsigned expected = bucket_base + ROUNDUP(i, j) / j;
            ASSERT_EQ(expected, bucket, "bucket index mismatch in log range");
            ASSERT_TRUE(IS_ALIGNED(rounded, j), "rounded size should be aligned in log range");
            ASSERT_GE(rounded, i, "rounded size should be >= requested in log range");
            ASSERT_LT(rounded - i, j, "rounded size is too large in log range");
            // Only HEAP_ALIGN-rounded sizes are freed or chopped off the end of a free area
            // when allocating.
            if ((i & (HEAP_ALIGN - 1)) == 0) {
                // When freeing, if we don't hit the size of the bucket precisely,
                // we have to put the free space into a smaller bucket, because
                // the buckets have entries that will always be big enough for
                // the corresponding allocation size (so we don't have to
                // traverse the free chains to find a big enough one).
                if ((i % j) == 0) {
                    ASSERT_EQ(size_to_index_freeing(i), (int)bucket,
                              "free index mismatch at boundary");
                } else {
                    ASSERT_EQ(size_to_index_freeing(i), (int)bucket - 1,
                              "free index mismatch near boundary");
                }
            }
        }
    }

    END_TEST;
}

static bool cmpct_test_get_back_newly_freed_helper(size_t size) {
    bool all_ok = true;
    void *blocker_left = cmpct_alloc(8);
    void *allocated = cmpct_alloc(size);
    if (allocated == NULL) {
        cmpct_free(blocker_left);
        return true;
    }
    void *blocker_right = cmpct_alloc(8);

    cmpct_free(allocated);
    void *allocated3 = cmpct_alloc(size);

    // Verify LIFO block reuse.
    EXPECT_EQ(allocated, allocated3, "should get the same block back");

    cmpct_free(blocker_left);
    cmpct_free(blocker_right);
    cmpct_free(allocated3);
    return all_ok;
}

static bool test_cmpct_get_back_newly_freed(void) {
    BEGIN_TEST;

    size_t increment = 2 * HEAP_ALIGN;
    for (size_t i = 16 * HEAP_ALIGN; i <= 0x8000000; i *= 2, increment *= 2) {
        for (size_t j = i; j < i * 2; j += increment) {
            EXPECT_TRUE(cmpct_test_get_back_newly_freed_helper(i - HEAP_ALIGN),
                        "LIFO reuse check failed");
            EXPECT_TRUE(cmpct_test_get_back_newly_freed_helper(i), "LIFO reuse check failed");
            EXPECT_TRUE(cmpct_test_get_back_newly_freed_helper(i + 1), "LIFO reuse check failed");
        }
    }
    for (size_t i = 1024; i <= 2048; i++) {
        EXPECT_TRUE(cmpct_test_get_back_newly_freed_helper(i), "LIFO reuse check failed");
    }

    END_TEST;
}

static bool test_cmpct_return_to_os(void) {
    BEGIN_TEST;

    cmpct_trim();
    size_t remaining = theheap.remaining;
    // This goes in a new OS allocation since the trim above removed any free
    // area big enough to contain it.
    void *a = cmpct_alloc(5000);
    void *b = cmpct_alloc(2500);
    cmpct_free(a);
    cmpct_free(b);
    // If things work as expected the new allocation is at the start of an OS
    // allocation.  There's just one sentinel and one header to the left of it.
    // It that's not the case then the allocation was met from some space in
    // the middle of an OS allocation, and our test won't work as expected, so
    // bail out.
    if (((uintptr_t)a & (PAGE_SIZE - 1)) != sizeof(header_t) * 2) {
        return true;
    }
    // No trim needed when the entire OS allocation is free.
    ASSERT_EQ(remaining, theheap.remaining, "reclaiming OS allocation should return memory");

    END_TEST;
}

static bool test_cmpct_random_allocs(void) {
    BEGIN_TEST;

    void *ptr[16];

    ptr[0] = cmpct_alloc(8);
    ptr[1] = cmpct_alloc(32);
    ptr[2] = cmpct_alloc(7);
    cmpct_trim();
    ptr[3] = cmpct_alloc(0);
    ptr[4] = cmpct_alloc(98713);
    ptr[5] = cmpct_alloc(16);

    cmpct_free(ptr[5]);
    cmpct_free(ptr[1]);
    cmpct_free(ptr[3]);
    cmpct_free(ptr[0]);
    cmpct_free(ptr[4]);
    cmpct_free(ptr[2]);

    cmpct_trim();

    int i;
    for (i = 0; i < 16; i++) {
        ptr[i] = 0;
    }

    for (i = 0; i < 32768; i++) {
        unsigned int index = (unsigned int)rand() % 16;

        if (ptr[index]) {
            cmpct_free(ptr[index]);
            ptr[index] = 0;
        }
        unsigned int align = 1 << ((unsigned int)rand() % 8);
        ptr[index] = cmpct_memalign((unsigned int)rand() % 32768, align);

        ASSERT_EQ(0u, (uintptr_t)ptr[index] % align, "alignment check in stress test");
    }

    for (i = 0; i < 16; i++) {
        if (ptr[i]) {
            cmpct_free(ptr[i]);
        }
    }

    END_TEST;
}

BEGIN_TEST_CASE(cmpctmalloc_tests)
RUN_TEST(test_cmpct_buckets)
RUN_TEST(test_cmpct_get_back_newly_freed)
RUN_TEST(test_cmpct_return_to_os)
RUN_TEST(test_cmpct_trim)
RUN_TEST(test_cmpct_random_allocs)
END_TEST_CASE(cmpctmalloc_tests)
