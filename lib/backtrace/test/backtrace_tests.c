/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/backtrace.h>
#include <lib/unittest.h>

#include <lk/compiler.h>
#include <string.h>

#if WITH_LIB_SYMTAB
#include <lib/symtab.h>
#endif

#if BACKTRACE_SUPPORTED

#define MAX_CAPTURE 32

static uintptr_t captured[MAX_CAPTURE];
static size_t captured_count;

/* Three frames of known nesting for the walk to find. They must not be
 * inlined or tail called into each other, or there is nothing to walk.
 */
__NO_INLINE static void bt_test_inner(void) {
    captured_count = backtrace_capture(0, (uintptr_t)__GET_FRAME(), captured, MAX_CAPTURE);
    __asm__ __volatile__("");
}

__NO_INLINE static void bt_test_middle(void) {
    bt_test_inner();
    __asm__ __volatile__("");
}

__NO_INLINE static void bt_test_outer(void) {
    bt_test_middle();
    __asm__ __volatile__("");
}

/* the walk finds the frames that are really on the stack */
static bool test_capture_depth(void) {
    BEGIN_TEST;

    captured_count = 0;
    bt_test_outer();

    /* inner's caller, its caller, and its caller: middle, outer, and whatever
     * called outer.
     */
    EXPECT_LT(2u, captured_count, "backtrace did not find the nested frames");

    END_TEST;
}

/* the captured addresses are code addresses, and each frame is distinct */
static bool test_capture_distinct(void) {
    BEGIN_TEST;

    captured_count = 0;
    bt_test_outer();
    ASSERT_LT(2u, captured_count, "backtrace did not find the nested frames");

    for (size_t i = 0; i < captured_count; i++) {
        EXPECT_NE(0u, (unsigned int)captured[i], "captured a null return address");
        for (size_t j = i + 1; j < captured_count; j++) {
            EXPECT_NE(captured[i], captured[j], "captured the same frame twice");
        }
    }

    END_TEST;
}

#if WITH_LIB_SYMTAB
/* the frames resolve to the functions that are really on the stack, which is
 * the end to end check that the walk and the symbol table agree
 */
static bool test_capture_symbolizes(void) {
    BEGIN_TEST;

    captured_count = 0;
    bt_test_outer();
    ASSERT_LT(2u, captured_count, "backtrace did not find the nested frames");

    static const char *const expected[] = {
        "bt_test_middle",
        "bt_test_outer",
    };

    for (size_t i = 0; i < countof(expected); i++) {
        /* every entry is a return address, so name the call rather than what
         * follows it, the same way backtrace_print() does
         */
        const char *name = symtab_lookup(captured[i] - 1, NULL);
        ASSERT_NONNULL(name, "frame did not resolve to a symbol");
        EXPECT_EQ(0, strcmp(name, expected[i]), "frame resolved to the wrong function");
    }

    END_TEST;
}
#endif

/* a walk from a frame pointer that is not a stack address stops immediately
 * rather than wandering off
 */
static bool test_capture_bogus_fp(void) {
    BEGIN_TEST;

    uintptr_t pcs[MAX_CAPTURE];

    /* a null frame pointer yields only the pc it was given */
    size_t count = backtrace_capture(0x1234, 0, pcs, countof(pcs));
    EXPECT_EQ(1u, count, "a null frame pointer produced frames");

    /* a misaligned one is rejected before it is dereferenced */
    count = backtrace_capture(0, 0x3, pcs, countof(pcs));
    EXPECT_EQ(0u, count, "a misaligned frame pointer produced frames");

    END_TEST;
}

BEGIN_TEST_CASE(backtrace_tests)
RUN_TEST(test_capture_depth)
RUN_TEST(test_capture_distinct)
#if WITH_LIB_SYMTAB
RUN_TEST(test_capture_symbolizes)
#endif
RUN_TEST(test_capture_bogus_fp)
END_TEST_CASE(backtrace_tests)

#else /* !BACKTRACE_SUPPORTED */

/* the stub still has to be callable */
static bool test_stub_is_empty(void) {
    BEGIN_TEST;

    uintptr_t pcs[4];
    EXPECT_EQ(0u, backtrace_capture(0x1234, 0x5678, pcs, countof(pcs)),
              "the stub produced frames");

    END_TEST;
}

BEGIN_TEST_CASE(backtrace_tests)
RUN_TEST(test_stub_is_empty)
END_TEST_CASE(backtrace_tests)

#endif
