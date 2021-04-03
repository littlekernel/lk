// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/initializer_list.h>
#include <unittest/unittest.h>

namespace {

bool ExpectListContents(size_t expected_size, fbl::initializer_list<int> list, bool &all_ok) {
    EXPECT_EQ(expected_size, list.size());

    size_t index = 0;
    for (const int *it = list.begin(); it != list.end(); ++it, ++index) {
        EXPECT_EQ(static_cast<int>(index), *it);
    }
    EXPECT_EQ(expected_size, index);

    return all_ok;
}

bool empty_test() {
    BEGIN_TEST;

    ExpectListContents(0u, {}, all_ok);

    END_TEST;
}

bool non_empty_test() {
    BEGIN_TEST;

    ExpectListContents(6u, {0, 1, 2, 3, 4, 5}, all_ok);

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(initializer_list_tests)
RUN_TEST(empty_test)
RUN_TEST(non_empty_test)
END_TEST_CASE(initializer_list_tests)
