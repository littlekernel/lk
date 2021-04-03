// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unittest/unittest.h>
#include <fbl/algorithm.h>
#include <fbl/tests/intrusive_containers/associative_container_test_environment.h>

namespace fbl {
namespace tests {
namespace intrusive_containers {

// OrderedAssociativeContainerTestEnvironment<>
//
// Test environment which defines and implements tests and test utilities which
// are applicable to all ordered associative containers (containers which keep
// their elements sorted by key) such as binary search trees.
template <typename TestEnvTraits>
class OrderedAssociativeContainerTestEnvironment
    : public AssociativeContainerTestEnvironment<TestEnvTraits> {
public:
    using ACTE           = AssociativeContainerTestEnvironment<TestEnvTraits>;
    using PopulateMethod = typename ACTE::PopulateMethod;
    using ContainerType  = typename ACTE::ContainerType;
    using KeyTraits      = typename ContainerType::KeyTraits;
    using KeyType        = typename ContainerType::KeyType;
    using RawPtrType     = typename ContainerType::RawPtrType;

    struct NonConstTraits {
        using ContainerType = typename ACTE::ContainerType;
        using IterType      = typename ACTE::ContainerType::iterator;
    };

    struct ConstTraits {
        using ContainerType = const typename ACTE::ContainerType;
        using IterType      = typename ACTE::ContainerType::const_iterator;
    };

    template <typename ContainerTraits>
    struct UpperBoundTraits {
        static typename ContainerTraits::IterType Search(
                typename ContainerTraits::ContainerType& container,
                const KeyType& key) {
            return container.upper_bound(key);
        }

        static bool BoundedBy(KeyType& key, const KeyType& bound_key) {
            return KeyTraits::LessThan(key, bound_key);
        }
    };

    template <typename ContainerTraits>
    struct LowerBoundTraits {
        static typename ContainerTraits::IterType Search(
                typename ContainerTraits::ContainerType& container,
                const KeyType& key) {
            return container.lower_bound(key);
        }

        static bool BoundedBy(const KeyType& key, const KeyType& bound_key) {
            return KeyTraits::LessThan(key, bound_key) || KeyTraits::EqualTo(key, bound_key);
        }
    };

    bool DoOrderedIter(PopulateMethod populate_method) {
        BEGIN_TEST;

        ASSERT_TRUE(ACTE::Populate(container(), populate_method), "");

        auto iter = container().begin();
        EXPECT_TRUE(iter.IsValid(), "");

        for (auto prev = iter++; iter.IsValid(); prev = iter++) {
            // None of the associative containers currently support storing
            // mutliple nodes with the same key, therefor the iteration ordering
            // of the keys should be strictly monotonically increasing.
            ASSERT_TRUE(prev.IsValid(), "");

            auto iter_key = KeyTraits::GetKey(*iter);
            auto prev_key = KeyTraits::GetKey(*prev);

            EXPECT_TRUE (KeyTraits::LessThan(prev_key, iter_key), "");
            EXPECT_FALSE(KeyTraits::LessThan(iter_key, prev_key), "");
            EXPECT_FALSE(KeyTraits::EqualTo(prev_key, iter_key), "");
            EXPECT_FALSE(KeyTraits::EqualTo(iter_key, prev_key), "");
        }

        ASSERT_TRUE(TestEnvironment<TestEnvTraits>::Reset(), "");
        END_TEST;
    }

    bool OrderedIter() {
        BEGIN_TEST;

        EXPECT_TRUE(DoOrderedIter(PopulateMethod::AscendingKey), "");
        EXPECT_TRUE(DoOrderedIter(PopulateMethod::DescendingKey), "");
        EXPECT_TRUE(DoOrderedIter(PopulateMethod::RandomKey), "");

        END_TEST;
    }

    bool DoOrderedReverseIter(PopulateMethod populate_method) {
        BEGIN_TEST;

        ASSERT_TRUE(ACTE::Populate(container(), populate_method), "");

        auto iter = container().end();
        EXPECT_FALSE(iter.IsValid(), "");
        --iter;
        EXPECT_TRUE(iter.IsValid(), "");

        for (auto prev = iter--; iter.IsValid(); prev = iter--) {
            // None of the associative containers currently support storing
            // mutliple nodes with the same key, therefor the reverse iteration
            // ordering of the keys should be strictly monotonically decreasing.
            ASSERT_TRUE(prev.IsValid(), "");

            auto iter_key = KeyTraits::GetKey(*iter);
            auto prev_key = KeyTraits::GetKey(*prev);

            EXPECT_TRUE (KeyTraits::LessThan(iter_key, prev_key), "");
            EXPECT_FALSE(KeyTraits::LessThan(prev_key, iter_key), "");
            EXPECT_FALSE(KeyTraits::EqualTo(prev_key, iter_key), "");
            EXPECT_FALSE(KeyTraits::EqualTo(iter_key, prev_key), "");
        }

        ASSERT_TRUE(TestEnvironment<TestEnvTraits>::Reset(), "");
        END_TEST;
    }

    bool OrderedReverseIter() {
        BEGIN_TEST;

        EXPECT_TRUE(DoOrderedReverseIter(PopulateMethod::AscendingKey), "");
        EXPECT_TRUE(DoOrderedReverseIter(PopulateMethod::DescendingKey), "");
        EXPECT_TRUE(DoOrderedReverseIter(PopulateMethod::RandomKey), "");

        END_TEST;
    }

    template <typename BoundTraits>
    bool DoBoundTest(PopulateMethod populate_method) {
        BEGIN_TEST;

        // Searching for a value while the tree is empty should always fail.
        auto found = BoundTraits::Search(container(), 0u);
        EXPECT_FALSE(found.IsValid(), "");

        // Populate the container.
        ASSERT_TRUE(ACTE::Populate(container(), populate_method), "");

        // For every object we just put into the container compute the bound of
        // obj.key, (obj.key - 1) and (obj.key + 1) using brute force as well as
        // by using (upper|lower)_bound.  Make sure that the result agree.
        for (size_t i = 0; i < ACTE::OBJ_COUNT; ++i) {
            auto ptr = ACTE::objects()[i];
            ASSERT_NONNULL(ptr, "");

            struct {
                KeyType key;
                RawPtrType bound;
            } tests[] = {
                { .key = KeyTraits::GetKey(*ptr) - 1, .bound = nullptr },  // prev (key - 1)
                { .key = KeyTraits::GetKey(*ptr),     .bound = nullptr },  // this (key)
                { .key = KeyTraits::GetKey(*ptr) + 1, .bound = nullptr },  // next (key + 1)
            };

            // Brute force search all of the objects we have populated the
            // collect with to find the objects with the smallest keys which
            // bound this/prev/next.
            for (size_t j = 0; j < ACTE::OBJ_COUNT; ++j) {
                auto tmp = ACTE::objects()[j];
                ASSERT_NONNULL(tmp, "");
                KeyType tmp_key = KeyTraits::GetKey(*tmp);

                for (size_t k = 0; k < fbl::count_of(tests); ++k) {
                    auto& test = tests[k];

                    if (BoundTraits::BoundedBy(test.key, tmp_key) &&
                       (!test.bound ||
                        KeyTraits::LessThan(tmp_key, KeyTraits::GetKey(*test.bound))))
                        test.bound = tmp;
                }
            }

            // Now perform the same searchs using upper_bound/lower_bound.
            for (size_t k = 0; k < fbl::count_of(tests); ++k) {
                auto& test = tests[k];
                auto  iter = BoundTraits::Search(container(), test.key);

                // We should successfully find a bound using (upper|lower)_bound
                // if (and only if) we successfully found a bound using brute
                // force.  If we did find a bound, it should be the same bound
                // we found using brute force.
                if (test.bound != nullptr) {
                    ASSERT_TRUE(iter.IsValid(), "");
                    EXPECT_EQ(test.bound, iter->raw_ptr(), "");
                } else {
                    EXPECT_FALSE(iter.IsValid(), "");
                }
            }
        }

        ASSERT_TRUE(TestEnvironment<TestEnvTraits>::Reset(), "");
        END_TEST;
    }

    bool UpperBound() {
        BEGIN_TEST;

        using NonConstBoundTraits = UpperBoundTraits<NonConstTraits>;
        EXPECT_TRUE(DoBoundTest<NonConstBoundTraits>(PopulateMethod::AscendingKey), "");
        EXPECT_TRUE(DoBoundTest<NonConstBoundTraits>(PopulateMethod::DescendingKey), "");
        EXPECT_TRUE(DoBoundTest<NonConstBoundTraits>(PopulateMethod::RandomKey), "");

        using ConstBoundTraits = UpperBoundTraits<ConstTraits>;
        EXPECT_TRUE(DoBoundTest<ConstBoundTraits>(PopulateMethod::AscendingKey), "");
        EXPECT_TRUE(DoBoundTest<ConstBoundTraits>(PopulateMethod::DescendingKey), "");
        EXPECT_TRUE(DoBoundTest<ConstBoundTraits>(PopulateMethod::RandomKey), "");

        END_TEST;
    }

    bool LowerBound() {
        BEGIN_TEST;

        using NonConstBoundTraits = LowerBoundTraits<NonConstTraits>;
        EXPECT_TRUE(DoBoundTest<NonConstBoundTraits>(PopulateMethod::AscendingKey), "");
        EXPECT_TRUE(DoBoundTest<NonConstBoundTraits>(PopulateMethod::DescendingKey), "");
        EXPECT_TRUE(DoBoundTest<NonConstBoundTraits>(PopulateMethod::RandomKey), "");

        using ConstBoundTraits = LowerBoundTraits<ConstTraits>;
        EXPECT_TRUE(DoBoundTest<ConstBoundTraits>(PopulateMethod::AscendingKey), "");
        EXPECT_TRUE(DoBoundTest<ConstBoundTraits>(PopulateMethod::DescendingKey), "");
        EXPECT_TRUE(DoBoundTest<ConstBoundTraits>(PopulateMethod::RandomKey), "");


        END_TEST;
    }

    // TODO(johngro) : lower bound tests

private:
    ContainerType&       container()       { return this->container_; }
    const ContainerType& const_container() { return this->container_; }
};

}  // namespace intrusive_containers
}  // namespace tests
}  // namespace fbl
