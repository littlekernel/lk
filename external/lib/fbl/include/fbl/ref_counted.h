// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/assert.h>
#include <fbl/macros.h>
#include <fbl/ref_counted_internal.h>

namespace fbl {

// Base class for a reference counted type.  Use as
//
// class Handle : public RefCounted<Handle> {
// };
//
// This supports intrusive atomic reference counting with adoption. This means
// that a new object starts life at a reference count of 1 and has to be adopted
// by a type (such as a fbl::RefPtr) that begins manipulation of the reference
// count. If the reference count ever reaches zero, the object's lifetime is
// over and it should be destroyed (Release() returns true if this is the case).
//
// You might be wondering about the unused |T| template parameter. It exists so
// that the base class of the derived object is a unique type. Without being a
// template then you can have many unrelated classes which can be implicitly
// upcasted to RefCounted* which is an undesirable situation for a mixin class
// like this one. Specially because the destructor is not virtual. The same is
// not possible via internal::RefCountedBase since the ctor and dtor are
// protected.
//
template <typename T,
          bool EnableAdoptionValidator = ZX_DEBUG_ASSERT_IMPLEMENTED>
class RefCounted : private internal::RefCountedBase<EnableAdoptionValidator> {
public:
    RefCounted() {}
    ~RefCounted() {}

    using internal::RefCountedBase<EnableAdoptionValidator>::AddRef;
    using internal::RefCountedBase<EnableAdoptionValidator>::Release;
    using internal::RefCountedBase<EnableAdoptionValidator>::Adopt;
    using internal::RefCountedBase<EnableAdoptionValidator>::ref_count_debug;

    // RefCounted<> instances may not be copied, assigned or moved.
    DISALLOW_COPY_ASSIGN_AND_MOVE(RefCounted);
};

}  // namespace fbl
