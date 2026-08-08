//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <new>.

#include <lib/unittest.h>
#include <new>
#include <stdint.h>

namespace {

struct alignas(64) OverAligned {
    char c;
};

struct Counted {
    static inline int live = 0;
    Counted() { live++; }
    ~Counted() { live--; }
};

bool basic_new_delete() {
    BEGIN_TEST;

    auto *p = new int(42);
    ASSERT_TRUE(p != nullptr, "");
    EXPECT_EQ(42, *p, "");
    delete p;

    auto *arr = new int[8]{};
    ASSERT_TRUE(arr != nullptr, "");
    EXPECT_EQ(0, arr[7], "array-new with {} must zero");
    delete[] arr;

    // zero-size allocations must return unique non-null pointers; call the
    // operator directly so -Walloc-size doesn't flag the new-expression
    void *z = operator new(0);
    ASSERT_TRUE(z != nullptr, "operator new(0) may not return nullptr");
    operator delete(z);

    END_TEST;
}

bool aligned_new() {
    BEGIN_TEST;

    // over-aligned types route to the align_val_t overloads automatically
    auto *oa = new OverAligned;
    ASSERT_TRUE(oa != nullptr, "");
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(oa) & 63, "alignment respected");
    delete oa;

    auto *oarr = new OverAligned[3];
    ASSERT_TRUE(oarr != nullptr, "");
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(oarr) & 63, "array alignment respected");
    delete[] oarr;

    // explicit aligned nothrow form
    void *raw = operator new(100, std::align_val_t{256}, std::nothrow);
    ASSERT_TRUE(raw != nullptr, "");
    EXPECT_EQ(0U, reinterpret_cast<uintptr_t>(raw) & 255, "");
    operator delete(raw, std::align_val_t{256});

    END_TEST;
}

bool nothrow_new() {
    BEGIN_TEST;

    auto *p = new (std::nothrow) int(7);
    ASSERT_TRUE(p != nullptr, "");
    EXPECT_EQ(7, *p, "");
    delete p;

    auto *arr = new (std::nothrow) int[4]{};
    ASSERT_TRUE(arr != nullptr, "");
    delete[] arr;

    // the nothrow delete forms are only called by the compiler during
    // exception unwind, which cannot happen here; call them directly so
    // they are exercised at all
    void *raw = operator new(8, std::nothrow);
    ASSERT_TRUE(raw != nullptr, "");
    operator delete(raw, std::nothrow);
    raw = operator new[](8, std::nothrow);
    ASSERT_TRUE(raw != nullptr, "");
    operator delete[](raw, std::nothrow);

    END_TEST;
}

bool placement_new() {
    BEGIN_TEST;

    alignas(Counted) unsigned char storage[sizeof(Counted)];
    EXPECT_EQ(0, Counted::live, "");
    auto *c = new (storage) Counted;
    EXPECT_EQ(1, Counted::live, "");
    c->~Counted();
    EXPECT_EQ(0, Counted::live, "");

    END_TEST;
}

bool launder_works() {
    BEGIN_TEST;

    int x = 5;
    int *p = std::launder(&x);
    EXPECT_EQ(5, *p, "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_new_tests)
RUN_TEST(basic_new_delete)
RUN_TEST(aligned_new)
RUN_TEST(nothrow_new)
RUN_TEST(placement_new)
RUN_TEST(launder_works)
END_TEST_CASE(libcpp_new_tests)

} // namespace
