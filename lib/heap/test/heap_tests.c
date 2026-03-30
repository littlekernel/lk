/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/unittest.h>
#include <lib/heap.h>

#include <stdint.h>
#include <string.h>

#if !defined(MEMSIZE) || MEMSIZE >= (128 * 1024)
#define HEAP_TEST_LARGE_ALLOCATIONS 1
#define HEAP_TEST_MEMALIGN_MAX_ALIGN 4096
#define HEAP_TEST_MEMALIGN_SIZE 64
#define HEAP_TEST_STRESS_SLOTS 16
#define HEAP_TEST_STRESS_ALIGN_BITS 8
#define HEAP_TEST_STRESS_MAX_BYTES 8192
#define HEAP_TEST_STRESS_ITERS_PER_SLOT 128
#else
#define HEAP_TEST_LARGE_ALLOCATIONS 0
#define HEAP_TEST_MEMALIGN_MAX_ALIGN 256
#define HEAP_TEST_MEMALIGN_SIZE 32
#define HEAP_TEST_STRESS_SLOTS 8
#define HEAP_TEST_STRESS_ALIGN_BITS 7
#define HEAP_TEST_STRESS_MAX_BYTES 1024
#define HEAP_TEST_STRESS_ITERS_PER_SLOT 64
#endif

/* basic malloc/free for a range of sizes */
static bool test_malloc_basic(void) {
    BEGIN_TEST;

    static const size_t sizes[] = { 1, 2, 4, 7, 8, 15, 16, 31, 32, 64, 128,
                                    256, 512, 1024, 4096,
#if HEAP_TEST_LARGE_ALLOCATIONS
                                    65536,
#endif
    };
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        void *p = malloc(sizes[i]);
        ASSERT_NONNULL(p, "malloc returned NULL");
        free(p);
    }

    END_TEST;
}

/* malloc(0) must not crash; its return value is implementation-defined */
static bool test_malloc_zero(void) {
    BEGIN_TEST;

    void *p = malloc(0);
    free(p);  /* safe whether p is NULL or a valid pointer */

    END_TEST;
}

/* free(NULL) is required to be a no-op by the C standard */
static bool test_free_null(void) {
    BEGIN_TEST;

    free(NULL);

    END_TEST;
}

/* calloc must return zeroed memory */
static bool test_calloc_zeroed(void) {
    BEGIN_TEST;

    const size_t n = 128;
    unsigned char *p = calloc(n, 1);
    ASSERT_NONNULL(p, "calloc(n, 1) failed");
    for (size_t i = 0; i < n; i++) {
        EXPECT_EQ(0, (int)p[i], "calloc byte not zeroed");
    }
    free(p);

    /* multi-element form */
    unsigned char *q = calloc(16, 16);
    ASSERT_NONNULL(q, "calloc(16, 16) failed");
    for (size_t i = 0; i < 16 * 16; i++) {
        EXPECT_EQ(0, (int)q[i], "calloc byte not zeroed (multi-element)");
    }
    free(q);

    END_TEST;
}

/* realloc to a larger size must preserve existing bytes */
static bool test_realloc_grow(void) {
    BEGIN_TEST;

    const size_t before = 32;
    const size_t after  = 256;

    unsigned char *p = malloc(before);
    ASSERT_NONNULL(p, "malloc failed");
    for (size_t i = 0; i < before; i++) {
        p[i] = (unsigned char)(i ^ 0x5a);
    }

    unsigned char *p2 = realloc(p, after);
    ASSERT_NONNULL(p2, "realloc grow failed");
    for (size_t i = 0; i < before; i++) {
        EXPECT_EQ((int)(unsigned char)(i ^ 0x5a), (int)p2[i],
                  "data corrupted after realloc grow");
    }
    free(p2);

    END_TEST;
}

/* realloc to a smaller size must preserve bytes up to the new size */
static bool test_realloc_shrink(void) {
    BEGIN_TEST;

    const size_t before = 256;
    const size_t after  = 16;

    unsigned char *p = malloc(before);
    ASSERT_NONNULL(p, "malloc failed");
    for (size_t i = 0; i < before; i++) {
        p[i] = (unsigned char)(i & 0xff);
    }

    unsigned char *p2 = realloc(p, after);
    ASSERT_NONNULL(p2, "realloc shrink failed");
    for (size_t i = 0; i < after; i++) {
        EXPECT_EQ((int)(unsigned char)(i & 0xff), (int)p2[i],
                  "data corrupted after realloc shrink");
    }
    free(p2);

    END_TEST;
}

/* realloc(NULL, size) must behave exactly like malloc(size) */
static bool test_realloc_from_null(void) {
    BEGIN_TEST;

    void *p = realloc(NULL, 64);
    ASSERT_NONNULL(p, "realloc(NULL, 64) failed");
    free(p);

    END_TEST;
}

/* LK expects realloc(ptr, 0) to return NULL and free ptr. */
static bool test_realloc_zero_size(void) {
    BEGIN_TEST;

    void *p = malloc(128);
    ASSERT_NONNULL(p, "malloc failed");

    void *q = realloc(p, 0);
    ASSERT_NULL(q, "realloc(ptr, 0) should return NULL");

    END_TEST;
}

/* memalign must return pointers with the requested power-of-2 alignment */
static bool test_memalign(void) {
    BEGIN_TEST;

    for (size_t align = 1; align <= HEAP_TEST_MEMALIGN_MAX_ALIGN; align <<= 1) {
        void *p = memalign(align, HEAP_TEST_MEMALIGN_SIZE);
        ASSERT_NONNULL(p, "memalign failed");
        EXPECT_EQ(0u, (uintptr_t)p % align, "bad alignment");
        free(p);
    }

    END_TEST;
}

/*
 * Keep multiple allocations of mixed sizes alive simultaneously, then free
 * them in reverse order.  Exercises coalescing and freelist management.
 */
static bool test_simultaneous_allocs(void) {
    BEGIN_TEST;

    static const size_t sizes[] = {
        8, 16, 32, 64, 128, 256, 512, 1024, 1, 3, 7, 15, 100, 200,
#if HEAP_TEST_LARGE_ALLOCATIONS
        98713,
#else
        4096,
#endif
        17
    };
    const int N = (int)(sizeof(sizes) / sizeof(sizes[0]));
    void *ptrs[sizeof(sizes) / sizeof(sizes[0])];

    for (int i = 0; i < N; i++) {
        ptrs[i] = malloc(sizes[i]);
        EXPECT_NONNULL(ptrs[i], "malloc failed in simultaneous_allocs");
    }
    for (int i = N - 1; i >= 0; i--) {
        free(ptrs[i]);
    }

    /* allocate again to confirm the heap is coherent after all the frees */
    for (int i = 0; i < N; i++) {
        ptrs[i] = malloc(sizes[i]);
        EXPECT_NONNULL(ptrs[i], "malloc failed after free in simultaneous_allocs");
    }
    for (int i = 0; i < N; i++) {
        free(ptrs[i]);
    }

    END_TEST;
}

/*
 * Write distinct byte patterns into several live allocations and verify they
 * are undisturbed — catches allocator bugs that hand out overlapping regions.
 */
static bool test_no_overlap(void) {
    BEGIN_TEST;

    const int N = 8;
    const size_t SIZE = 128;
    unsigned char *ptrs[8];

    for (int i = 0; i < N; i++) {
        ptrs[i] = malloc(SIZE);
        ASSERT_NONNULL(ptrs[i], "malloc failed");
        memset(ptrs[i], (int)(0xaa ^ (unsigned int)i), SIZE);
    }

    for (int i = 0; i < N; i++) {
        unsigned char expected = (unsigned char)(0xaa ^ (unsigned int)i);
        for (size_t j = 0; j < SIZE; j++) {
            ASSERT_EQ((int)expected, (int)ptrs[i][j], "heap overlap detected");
        }
    }

    for (int i = 0; i < N; i++) {
        free(ptrs[i]);
    }

    END_TEST;
}

/*
 * Stress: repeatedly alloc with random-ish (but deterministic) aligned sizes,
 * write, and free in a pattern that exercises varied freelist state.
 */
static bool test_memalign_stress(void) {
    BEGIN_TEST;

    const int slots = HEAP_TEST_STRESS_SLOTS;
    void *ptrs[HEAP_TEST_STRESS_SLOTS] = { 0 };

    for (int i = 0; i < slots * HEAP_TEST_STRESS_ITERS_PER_SLOT; i++) {
        int slot = i % slots;

        free(ptrs[slot]);

        size_t align = (size_t)1 << ((unsigned)i % HEAP_TEST_STRESS_ALIGN_BITS);
        size_t nbytes =
            (size_t)((unsigned)i * 31 + 7) % HEAP_TEST_STRESS_MAX_BYTES + 1;

        ptrs[slot] = memalign(align, nbytes);
        ASSERT_NONNULL(ptrs[slot], "memalign failed in stress test");
        EXPECT_EQ(0u, (uintptr_t)ptrs[slot] % align, "bad alignment in stress");
    }

    for (int i = 0; i < slots; i++) {
        free(ptrs[i]);
    }

    END_TEST;
}

BEGIN_TEST_CASE(heap_tests)
    RUN_TEST(test_malloc_basic)
    RUN_TEST(test_malloc_zero)
    RUN_TEST(test_free_null)
    RUN_TEST(test_calloc_zeroed)
    RUN_TEST(test_realloc_grow)
    RUN_TEST(test_realloc_shrink)
    RUN_TEST(test_realloc_from_null)
    RUN_TEST(test_realloc_zero_size)
    RUN_TEST(test_memalign)
    RUN_TEST(test_simultaneous_allocs)
    RUN_TEST(test_no_overlap)
    RUN_TEST(test_memalign_stress)
END_TEST_CASE(heap_tests)
