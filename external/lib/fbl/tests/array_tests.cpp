// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/array.h>

#include <fbl/alloc_checker.h>
#include <fbl/algorithm.h>
#include <unittest/unittest.h>

namespace {

class DestructorSignaler {
public:
    DestructorSignaler() : array(nullptr), result(nullptr) {}
    ~DestructorSignaler() {
      if (array && result)
        *result = array->get();
    }

    fbl::Array<DestructorSignaler>* array;
    DestructorSignaler** result;
};

bool destructor_test() {
    BEGIN_TEST;

    DestructorSignaler bogus;
    DestructorSignaler* result = &bogus;

    fbl::AllocChecker ac;
    DestructorSignaler* signalers = new (&ac) DestructorSignaler[2];
    EXPECT_TRUE(ac.check());

    {
        fbl::Array<DestructorSignaler> array(signalers, 2);
        array[0].array = &array;
        array[0].result = &result;
    }

    EXPECT_FALSE(result == &bogus);
    EXPECT_TRUE(result == nullptr);

    END_TEST;
}

}  // namespace

BEGIN_TEST_CASE(array_tests)
RUN_NAMED_TEST("destructor test", destructor_test)
END_TEST_CASE(array_tests);
