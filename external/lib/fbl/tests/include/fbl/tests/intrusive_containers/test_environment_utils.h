// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <fbl/type_support.h>

namespace fbl {
namespace tests {
namespace intrusive_containers {

// ContainerUtils
//
// A utility class used by container tests to move a pointer to an object into
// an instance of the container being tested.  For sequenced containers, the
// operation will be push_front().  For associative containers, the operation
// will be an insert() by key.
template <typename ContainerType, typename Enable = void>
struct ContainerUtils;

template <typename ContainerType>
struct ContainerUtils<ContainerType,
                      typename enable_if<ContainerType::IsSequenced, void>::type> {
    using PtrTraits = typename ContainerType::PtrTraits;
    using PtrType   = typename PtrTraits::PtrType;

    static void MoveInto(ContainerType& container, PtrType&& ptr) {
        container.push_front(fbl::move(ptr));
    }
};

template <typename ContainerType>
struct ContainerUtils<ContainerType,
                      typename enable_if<ContainerType::IsAssociative, void>::type> {
    using PtrTraits = typename ContainerType::PtrTraits;
    using PtrType   = typename PtrTraits::PtrType;

    static void MoveInto(ContainerType& container, PtrType&& ptr) {
        container.insert(fbl::move(ptr));
    }
};

template <typename ContainerType, typename Enable = void>
struct SizeUtils;

template <typename ContainerType>
struct SizeUtils<ContainerType,
                 typename enable_if<ContainerType::SupportsConstantOrderSize == true, void>::type> {
    static size_t size(const ContainerType& container) { return container.size(); }
};

template <typename ContainerType>
struct SizeUtils<ContainerType,
                 typename enable_if<ContainerType::SupportsConstantOrderSize == false,
                                    void>::type> {
    static size_t size(const ContainerType& container) { return container.size_slow(); }
};

}  // namespace intrusive_containers
}  // namespace tests
}  // namespace fbl
