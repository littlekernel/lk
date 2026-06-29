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

/* malloc() must return memory aligned to at least 2*sizeof(void*) for any size.
 * This is the "two-word" minimum that covers all fundamental scalar types
 * (uint64_t, double, etc.) on every LK architecture. */
static bool test_malloc_alignment(void) {
    BEGIN_TEST;

    const size_t min_align = 2 * sizeof(void *);

    // Verify alignment across a wide range of sizes, including awkward ones
    // that are not multiples of any power of two.
    static const size_t sizes[] = {
        1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65,
        127, 128, 255, 256, 1023, 1024,
    };
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        void *p = malloc(sizes[i]);
        ASSERT_NONNULL(p, "malloc returned NULL");
        EXPECT_EQ(0u, (uintptr_t)p % min_align, "malloc alignment violated");
        free(p);
    }

    // Punch a hole in the freelist and re-fill it, exercising the split path.
    void *a = malloc(1);
    void *b = malloc(1);
    void *c = malloc(1);
    ASSERT_NONNULL(a, "malloc a");
    ASSERT_NONNULL(b, "malloc b");
    ASSERT_NONNULL(c, "malloc c");
    EXPECT_EQ(0u, (uintptr_t)a % min_align, "a alignment");
    EXPECT_EQ(0u, (uintptr_t)b % min_align, "b alignment");
    EXPECT_EQ(0u, (uintptr_t)c % min_align, "c alignment");
    free(b);
    void *d = malloc(1);
    ASSERT_NONNULL(d, "malloc d after hole");
    EXPECT_EQ(0u, (uintptr_t)d % min_align, "d alignment after hole");
    free(a);
    free(c);
    free(d);

    END_TEST;
}

static bool test_heap_worst_case(void) {
    BEGIN_TEST;

    const int N = 64;
    const size_t alloc_size = 32;
    void *ptrs[64];

    // 1. Allocate all blocks sequentially (should be back-to-back in memory)
    for (int i = 0; i < N; i++) {
        ptrs[i] = malloc(alloc_size);
        ASSERT_NONNULL(ptrs[i], "malloc failed in worst-case test setup");
        // write to it to ensure memory is backing it
        memset(ptrs[i], 0xcc, alloc_size);
    }

    // 2. Free every other one (odd indices) to fragment the freelist
    for (int i = 1; i < N; i += 2) {
        free(ptrs[i]);
        ptrs[i] = NULL;
    }

    // 3. Try to allocate a block larger than any individual hole.
    // It should skip all the small holes and find space at the end of the heap.
    void *large_ptr = malloc(alloc_size * 3);
    ASSERT_NONNULL(large_ptr, "malloc for larger block failed under fragmentation");
    memset(large_ptr, 0xdd, alloc_size * 3);
    free(large_ptr);

    // 4. Try allocating some small blocks that should fit in the holes.
    void *temp_ptrs[5];
    for (int i = 0; i < 5; i++) {
        temp_ptrs[i] = malloc(alloc_size);
        ASSERT_NONNULL(temp_ptrs[i], "malloc to fill holes failed");
        memset(temp_ptrs[i], 0xee, alloc_size);
    }
    for (int i = 0; i < 5; i++) {
        free(temp_ptrs[i]);
    }

    // 5. Free the remaining even-indexed blocks (this forces coalescing with neighbors)
    for (int i = 0; i < N; i += 2) {
        free(ptrs[i]);
        ptrs[i] = NULL;
    }

    // 6. Ensure we can allocate the entire combined size in one go now
    void *huge_ptr = malloc(alloc_size * N);
    ASSERT_NONNULL(huge_ptr, "failed to allocate large coalesced block");
    memset(huge_ptr, 0xff, alloc_size * N);
    free(huge_ptr);

    END_TEST;
}

static bool test_realloc_in_place_shrink(void) {
    BEGIN_TEST;

    const size_t initial_size = 256;
    const size_t new_size = 128;

    uint8_t *p = malloc(initial_size);
    ASSERT_NONNULL(p, "malloc failed");

    for (size_t i = 0; i < initial_size; i++) {
        p[i] = (uint8_t)(i ^ 0x3c);
    }

    uint8_t *p2 = realloc(p, new_size);
    ASSERT_NONNULL(p2, "realloc shrink failed");

    // Data verification
    for (size_t i = 0; i < new_size; i++) {
        EXPECT_EQ((uint8_t)(i ^ 0x3c), p2[i], "data corrupted during realloc shrink");
    }

    free(p2);
    END_TEST;
}

static bool test_realloc_in_place_grow(void) {
    BEGIN_TEST;

    const size_t initial_size = 128;
    const size_t new_size = 256;

    uint8_t *p = malloc(initial_size);
    ASSERT_NONNULL(p, "malloc failed");

    for (size_t i = 0; i < initial_size; i++) {
        p[i] = (uint8_t)(i ^ 0xa5);
    }

    // We keep the area after p free. Realloc should ideally grow in-place if supported.
    uint8_t *p2 = realloc(p, new_size);
    ASSERT_NONNULL(p2, "realloc grow failed");

    // Data verification
    for (size_t i = 0; i < initial_size; i++) {
        EXPECT_EQ((uint8_t)(i ^ 0xa5), p2[i], "data corrupted during realloc grow");
    }

    free(p2);
    END_TEST;
}

static bool test_realloc_blocked_grow(void) {
    BEGIN_TEST;

    const size_t initial_size = 128;
    const size_t new_size = 256;

    uint8_t *p = malloc(initial_size);
    ASSERT_NONNULL(p, "malloc failed");

    // Allocate a block right after p to block in-place growth
    uint8_t *blocker = malloc(64);
    ASSERT_NONNULL(blocker, "malloc blocker failed");

    for (size_t i = 0; i < initial_size; i++) {
        p[i] = (uint8_t)(i ^ 0x55);
    }

    // Grow p. It must allocate a new block and copy data.
    uint8_t *p2 = realloc(p, new_size);
    ASSERT_NONNULL(p2, "realloc blocked grow failed");
    EXPECT_NE(p2, blocker, "realloc returned blocker pointer!");

    // Data verification
    for (size_t i = 0; i < initial_size; i++) {
        EXPECT_EQ((uint8_t)(i ^ 0x55), p2[i], "data corrupted during blocked realloc grow");
    }

    free(p2);
    free(blocker);
    END_TEST;
}

static bool test_realloc_multiple_steps(void) {
    BEGIN_TEST;

    size_t sizes[] = { 32, 128, 64, 512, 1024, 256, 16, 2048 };
    uint8_t *p = NULL;

    for (size_t step = 0; step < sizeof(sizes) / sizeof(sizes[0]); step++) {
        size_t new_size = sizes[step];
        size_t prev_size = (step == 0) ? 0 : sizes[step - 1];
        size_t min_size = (new_size < prev_size) ? new_size : prev_size;

        p = realloc(p, new_size);
        ASSERT_NONNULL(p, "realloc step failed");

        // Verify data from previous step is intact
        for (size_t i = 0; i < min_size; i++) {
            EXPECT_EQ((uint8_t)(i ^ (step - 1)), p[i], "data corrupted across steps");
        }

        // Fill with new step-specific pattern
        for (size_t i = 0; i < new_size; i++) {
            p[i] = (uint8_t)(i ^ step);
        }
    }

    free(p);
    END_TEST;
}


BEGIN_TEST_CASE(heap_tests)
    RUN_TEST(test_malloc_basic)
    RUN_TEST(test_malloc_zero)
    RUN_TEST(test_malloc_alignment)
    RUN_TEST(test_free_null)
    RUN_TEST(test_calloc_zeroed)
    RUN_TEST(test_realloc_grow)
    RUN_TEST(test_realloc_shrink)
    RUN_TEST(test_realloc_from_null)
    RUN_TEST(test_realloc_zero_size)
    RUN_TEST(test_realloc_in_place_shrink)
    RUN_TEST(test_realloc_in_place_grow)
    RUN_TEST(test_realloc_blocked_grow)
    RUN_TEST(test_realloc_multiple_steps)
    RUN_TEST(test_memalign)
    RUN_TEST(test_simultaneous_allocs)
    RUN_TEST(test_no_overlap)
    RUN_TEST(test_memalign_stress)
    RUN_TEST(test_heap_worst_case)
END_TEST_CASE(heap_tests)
