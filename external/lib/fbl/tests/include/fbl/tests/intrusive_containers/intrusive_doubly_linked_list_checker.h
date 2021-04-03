// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unittest/unittest.h>
#include <fbl/intrusive_hash_table.h>

namespace fbl {
namespace tests {
namespace intrusive_containers {

using ::fbl::internal::is_sentinel_ptr;

// Sanity checks for doubly linked lists are almost the same as those for singly
// linked lists.  We also check to be sure that the tail pointer is properly
// linked up (if the list is not empty) and that it is terminated with the
// sentinel value.
class DoublyLinkedListChecker {
public:
    template <typename ContainerType>
    static bool SanityCheck(const ContainerType& container) {
        using NodeTraits = typename ContainerType::NodeTraits;
        using PtrTraits  = typename ContainerType::PtrTraits;
        BEGIN_TEST;

        typename PtrTraits::RawPtrType tmp = container.head_;
        while (true) {
            ASSERT_NONNULL(tmp, "");

            if (is_sentinel_ptr(tmp)) {
                ASSERT_EQ(container.sentinel(), tmp, "");
                break;
            }

            tmp = NodeTraits::node_state(*tmp).next_;
        }

        tmp = container.tail();
        if (!is_sentinel_ptr(container.head_)) {
            ASSERT_NONNULL(tmp, "");
            tmp = NodeTraits::node_state(*tmp).next_;
        }
        ASSERT_EQ(container.sentinel(), tmp, "");

        END_TEST;
    }
};

}  // namespace intrusive_containers
}  // namespace tests
}  // namespace fbl
