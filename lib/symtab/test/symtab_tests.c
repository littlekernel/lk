/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/symtab.h>
#include <lib/unittest.h>

#include <lk/compiler.h>
#include <string.h>

/* A function whose address and name the test can compare against, rather than
 * relying on any particular kernel symbol being present in every build. The
 * nops give it a body of more than a couple of bytes, so that an address just
 * inside it is still inside it: an empty function compiles down to a single
 * two byte instruction where compressed encodings are available.
 */
__NO_INLINE static void symtab_test_target(void) {
    __asm__ __volatile__("nop; nop; nop; nop");
}

/* The address the linker recorded for a function.
 *
 * On ARM a pointer to a Thumb function has bit 0 set to select the
 * instruction set, but the symbol itself sits at the even address, and no pc
 * ever has that bit set. Mask it off before comparing against the table.
 */
static inline uintptr_t func_addr(const void *f) {
#if ARCH_ARM
    return (uintptr_t)f & ~(uintptr_t)1;
#else
    return (uintptr_t)f;
#endif
}

#define SYMTAB_TEST_TARGET (func_addr(&symtab_test_target))
#define SYMTAB_TEST_TARGET_INTERIOR (SYMTAB_TEST_TARGET + 2)

/* the address a function symbol starts at resolves to that function */
static bool test_lookup_exact(void) {
    BEGIN_TEST;

    uintptr_t base = 0;
    const char *name = symtab_lookup(SYMTAB_TEST_TARGET, &base);

    ASSERT_NONNULL(name, "no symbol for a known function");
    EXPECT_EQ(0, strcmp(name, "symtab_test_target"), "wrong symbol name");
    EXPECT_EQ(SYMTAB_TEST_TARGET, base, "wrong symbol base");

    END_TEST;
}

/* an address inside a function still resolves to it, with an offset */
static bool test_lookup_interior(void) {
    BEGIN_TEST;

    uintptr_t base = 0;
    const char *name = symtab_lookup(SYMTAB_TEST_TARGET_INTERIOR, &base);

    ASSERT_NONNULL(name, "no symbol for an address inside a function");
    EXPECT_EQ(0, strcmp(name, "symtab_test_target"), "wrong symbol name");
    EXPECT_EQ(SYMTAB_TEST_TARGET, base, "wrong symbol base");

    END_TEST;
}

/* the base pointer is optional */
static bool test_lookup_no_base(void) {
    BEGIN_TEST;

    const char *name = symtab_lookup(SYMTAB_TEST_TARGET, NULL);
    ASSERT_NONNULL(name, "no symbol for a known function");

    END_TEST;
}

/* addresses outside the kernel's text resolve to nothing rather than to the
 * nearest symbol
 */
static bool test_lookup_out_of_range(void) {
    BEGIN_TEST;

    EXPECT_NULL(symtab_lookup(0, NULL), "symbol found for a null address");
    EXPECT_NULL(symtab_lookup(UINTPTR_MAX, NULL), "symbol found above the kernel");

    END_TEST;
}

/* every symbol in the table is reachable and ordered, which is what the binary
 * search in symtab_lookup() depends on
 */
static bool test_table_ordered(void) {
    BEGIN_TEST;

    uintptr_t base = 0;
    ASSERT_NONNULL(symtab_lookup(SYMTAB_TEST_TARGET, &base), "no symbol");

    /* walking backwards from a known symbol must land on symbols at strictly
     * decreasing addresses
     */
    uintptr_t prev = base;
    for (int i = 0; i < 32 && prev > 0; i++) {
        uintptr_t next_base = 0;
        const char *name = symtab_lookup(prev - 1, &next_base);
        if (!name) {
            break; /* walked off the front of the table */
        }
        EXPECT_LT(next_base, prev, "symbol addresses are not ordered");
        prev = next_base;
    }

    END_TEST;
}

BEGIN_TEST_CASE(symtab_tests)
RUN_TEST(test_lookup_exact)
RUN_TEST(test_lookup_interior)
RUN_TEST(test_lookup_no_base)
RUN_TEST(test_lookup_out_of_range)
RUN_TEST(test_table_ordered)
END_TEST_CASE(symtab_tests)
