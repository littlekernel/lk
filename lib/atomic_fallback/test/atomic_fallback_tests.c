/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/unittest.h>

#include <stdbool.h>
#include <stdint.h>

/*
 * The routines under test are the compiler's atomic library interface, so they
 * are not declared in any header. Declare them here, and call them through
 * volatile function pointers so the compiler cannot recognize the names and
 * quietly expand something inline instead of calling ours.
 */

#if !defined(__clang__)
/* see the note in atomic_fallback.c about gcc's builtin declaration */
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif

#define DECLARE_WIDTH(width, type) \
    type __atomic_load_##width(const volatile void *ptr, int memorder); \
    void __atomic_store_##width(volatile void *ptr, type val, int memorder); \
    bool __atomic_compare_exchange_##width(volatile void *ptr, void *expected, \
                                           type desired, int success, int failure); \
    type __atomic_exchange_##width(volatile void *ptr, type val, int memorder); \
    type __atomic_fetch_add_##width(volatile void *ptr, type val, int memorder); \
    type __atomic_fetch_sub_##width(volatile void *ptr, type val, int memorder); \
    type __atomic_fetch_and_##width(volatile void *ptr, type val, int memorder); \
    type __atomic_fetch_or_##width(volatile void *ptr, type val, int memorder); \
    type __atomic_fetch_xor_##width(volatile void *ptr, type val, int memorder); \
    type __atomic_fetch_nand_##width(volatile void *ptr, type val, int memorder);

DECLARE_WIDTH(1, uint8_t)
DECLARE_WIDTH(2, uint16_t)
DECLARE_WIDTH(4, uint32_t)

#undef DECLARE_WIDTH

/*
 * One test body per width. The values are chosen to stay inside a byte so the
 * same expectations hold for all three.
 */
#define DEFINE_WIDTH_TEST(width, type) \
    static bool atomic_fallback_##width(void) { \
        BEGIN_TEST; \
        \
        type (*volatile load)(const volatile void *, int) = __atomic_load_##width; \
        void (*volatile store)(volatile void *, type, int) = __atomic_store_##width; \
        bool (*volatile cas)(volatile void *, void *, type, int, int) = \
            __atomic_compare_exchange_##width; \
        type (*volatile exchange)(volatile void *, type, int) = __atomic_exchange_##width; \
        type (*volatile fetch_add)(volatile void *, type, int) = __atomic_fetch_add_##width; \
        type (*volatile fetch_sub)(volatile void *, type, int) = __atomic_fetch_sub_##width; \
        type (*volatile fetch_and)(volatile void *, type, int) = __atomic_fetch_and_##width; \
        type (*volatile fetch_or)(volatile void *, type, int) = __atomic_fetch_or_##width; \
        type (*volatile fetch_xor)(volatile void *, type, int) = __atomic_fetch_xor_##width; \
        type (*volatile fetch_nand)(volatile void *, type, int) = __atomic_fetch_nand_##width; \
        \
        type val = 0; \
        \
        /* load and store, relaxed and ordered */ \
        store(&val, 0x12, __ATOMIC_RELAXED); \
        EXPECT_EQ(0x12U, (unsigned)load(&val, __ATOMIC_RELAXED), ""); \
        store(&val, 0x34, __ATOMIC_SEQ_CST); \
        EXPECT_EQ(0x34U, (unsigned)load(&val, __ATOMIC_ACQUIRE), ""); \
        \
        /* exchange returns the old value */ \
        EXPECT_EQ(0x34U, (unsigned)exchange(&val, 0x56, __ATOMIC_RELAXED), ""); \
        EXPECT_EQ(0x56U, (unsigned)val, ""); \
        \
        /* the fetch operations all return the old value and store the new */ \
        val = 0x10; \
        EXPECT_EQ(0x10U, (unsigned)fetch_add(&val, 5, __ATOMIC_RELAXED), ""); \
        EXPECT_EQ(0x15U, (unsigned)val, ""); \
        EXPECT_EQ(0x15U, (unsigned)fetch_sub(&val, 4, __ATOMIC_RELAXED), ""); \
        EXPECT_EQ(0x11U, (unsigned)val, ""); \
        \
        val = 0x3c; \
        EXPECT_EQ(0x3cU, (unsigned)fetch_and(&val, 0x0f, __ATOMIC_RELAXED), ""); \
        EXPECT_EQ(0x0cU, (unsigned)val, ""); \
        EXPECT_EQ(0x0cU, (unsigned)fetch_or(&val, 0x30, __ATOMIC_RELAXED), ""); \
        EXPECT_EQ(0x3cU, (unsigned)val, ""); \
        EXPECT_EQ(0x3cU, (unsigned)fetch_xor(&val, 0x0f, __ATOMIC_RELAXED), ""); \
        EXPECT_EQ(0x33U, (unsigned)val, ""); \
        \
        /* nand is ~(old & val), truncated to the width */ \
        val = 0x3c; \
        EXPECT_EQ(0x3cU, (unsigned)fetch_nand(&val, 0x0f, __ATOMIC_RELAXED), ""); \
        EXPECT_EQ((unsigned)(type)~(type)0x0c, (unsigned)val, ""); \
        \
        /* compare exchange: match swaps and leaves the expected value alone */ \
        val = 0x42; \
        type expected = 0x42; \
        EXPECT_TRUE(cas(&val, &expected, 0x99, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST), ""); \
        EXPECT_EQ(0x99U, (unsigned)val, ""); \
        EXPECT_EQ(0x42U, (unsigned)expected, ""); \
        \
        /* mismatch leaves memory alone and writes the current value back out, \
         * which is what a caller looping on cas depends on */ \
        expected = 0x11; \
        EXPECT_FALSE(cas(&val, &expected, 0x77, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST), ""); \
        EXPECT_EQ(0x99U, (unsigned)val, ""); \
        EXPECT_EQ(0x99U, (unsigned)expected, ""); \
        \
        END_TEST; \
    }

DEFINE_WIDTH_TEST(1, uint8_t)
DEFINE_WIDTH_TEST(2, uint16_t)
DEFINE_WIDTH_TEST(4, uint32_t)

#undef DEFINE_WIDTH_TEST

/* the routines must not disturb whatever is next to them in memory */
static bool atomic_fallback_neighbors(void) {
    BEGIN_TEST;

    struct {
        uint8_t before;
        uint8_t target;
        uint8_t after;
    } bytes = { 0xaa, 0x00, 0xbb };

    uint8_t (*volatile fetch_add_1)(volatile void *, uint8_t, int) = __atomic_fetch_add_1;
    fetch_add_1(&bytes.target, 0xff, __ATOMIC_RELAXED);
    EXPECT_EQ(0xaaU, bytes.before, "");
    EXPECT_EQ(0xffU, bytes.target, "");
    EXPECT_EQ(0xbbU, bytes.after, "");

    struct {
        uint16_t before;
        uint16_t target;
        uint16_t after;
    } halves = { 0xaaaa, 0x0000, 0xbbbb };

    uint16_t (*volatile fetch_add_2)(volatile void *, uint16_t, int) = __atomic_fetch_add_2;
    fetch_add_2(&halves.target, 0xffff, __ATOMIC_RELAXED);
    EXPECT_EQ(0xaaaaU, halves.before, "");
    EXPECT_EQ(0xffffU, halves.target, "");
    EXPECT_EQ(0xbbbbU, halves.after, "");

    END_TEST;
}

BEGIN_TEST_CASE(atomic_fallback_tests)
RUN_TEST(atomic_fallback_1)
RUN_TEST(atomic_fallback_2)
RUN_TEST(atomic_fallback_4)
RUN_TEST(atomic_fallback_neighbors)
END_TEST_CASE(atomic_fallback_tests)
