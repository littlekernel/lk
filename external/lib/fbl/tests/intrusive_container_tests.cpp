// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/intrusive_container_utils.h>
#include <fbl/tests/intrusive_containers/objects.h>
#include <unittest/unittest.h>

namespace fbl {
namespace tests {
namespace intrusive_containers {

size_t TestObjBase::live_obj_count_ = 0;

template <typename T>
bool swap_test(const T initial_a, const T initial_b, bool &all_ok) {
    // Starting A and B need to be different in order for us to know that swap
    // worked.
    EXPECT_NE(::memcmp(&initial_a, &initial_b, sizeof(T)), 0);

    T a = initial_a;
    T b = initial_b;
    ::fbl::internal::Swap(a, b);

    EXPECT_EQ(::memcmp(&a, &initial_b, sizeof(T)), 0);
    EXPECT_EQ(::memcmp(&b, &initial_a, sizeof(T)), 0);

    END_TEST;
}

bool swap_test() {
    BEGIN_TEST;

    struct SimpleSmallStruct { uint8_t a, b; };
    struct SimpleBigStruct   { uint32_t a, b; };
    struct SimpleHugeStruct  { uint64_t a, b; };

    EXPECT_TRUE(swap_test<char>('a', 'b', all_ok));
    EXPECT_TRUE(swap_test<int8_t>(-5, 10, all_ok));
    EXPECT_TRUE(swap_test<uint8_t>(5, 10, all_ok));
    EXPECT_TRUE(swap_test<int16_t>(-12345, 12345, all_ok));
    EXPECT_TRUE(swap_test<uint16_t>(12345, 54321, all_ok));
    EXPECT_TRUE(swap_test<int32_t>(-1234567890, 123456789, all_ok));
    EXPECT_TRUE(swap_test<uint32_t>(1234567890, 987654321, all_ok));
    EXPECT_TRUE(swap_test<int64_t>(-12345678901234567, 12345678901234567, all_ok));
    EXPECT_TRUE(swap_test<uint64_t>(12345678901234567, 98765432109876543, all_ok));
    EXPECT_TRUE(swap_test<float>(-0.1234567f, 0.7654321f, all_ok));
    EXPECT_TRUE(swap_test<double>(-0.12345678901234567890, 0.98765432109876543210, all_ok));
    EXPECT_TRUE(swap_test<SimpleSmallStruct>({ 5, 4 }, { 2, 9 }, all_ok));
    EXPECT_TRUE(swap_test<SimpleBigStruct>({ 5, 4 }, { 2, 9 }, all_ok));
#if TEST_WILL_NOT_COMPILE || 0
    EXPECT_TRUE(swap_test<SimpleHugeStruct>({ 5, 4 }, { 2, 9 }, all_ok));
#endif

    SimpleBigStruct a = {};
    SimpleBigStruct b = {};
    EXPECT_TRUE(swap_test<void*>(&a, &b, all_ok));
    EXPECT_TRUE(swap_test<SimpleBigStruct*>(&a, &b, all_ok));

    END_TEST;
}


BEGIN_TEST_CASE(container_utils_tests)
RUN_NAMED_TEST("swap test", swap_test)
END_TEST_CASE(container_utils_tests)

}  // namespace intrusive_containers
}  // namespace tests
}  // namespace fbl
