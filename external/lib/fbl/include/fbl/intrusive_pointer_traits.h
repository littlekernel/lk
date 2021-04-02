// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdint.h>
#include <zircon/compiler.h>
#include <fbl/ref_ptr.h>
#include <fbl/unique_ptr.h>
#include <fbl/type_support.h>

namespace fbl {
namespace internal {

// Declaration of the base type which will be used to control what type of
// pointers are permitted to be in containers.
template <typename T> struct ContainerPtrTraits;

// Traits for managing raw pointers.
template <typename T>
struct ContainerPtrTraits<T*> {
    using ValueType       = T;
    using RefType         = T&;
    using ConstRefType    = const T&;
    using PtrType         = T*;
    using ConstPtrType    = const T*;
    using RawPtrType      = T*;
    using ConstRawPtrType = const T*;

    static constexpr bool IsManaged = false;
    static constexpr bool CanCopy = true;

    static inline T* GetRaw(const PtrType& ptr) { return ptr; }
    static inline T* Copy(const RawPtrType& ptr) { return ptr; }

    static inline RawPtrType Leak(PtrType& ptr) __WARN_UNUSED_RESULT {
        return ptr;
    }

    static inline PtrType Reclaim(RawPtrType ptr) {
        return ptr;
    }
};

// Traits for managing unique pointers.
template <typename T>
struct ContainerPtrTraits<::fbl::unique_ptr<T>> {
    using ValueType       = T;
    using RefType         = T&;
    using ConstRefType    = const T&;
    using PtrType         = ::fbl::unique_ptr<T>;
    using ConstPtrType    = ::fbl::unique_ptr<const T>;
    using RawPtrType      = T*;
    using ConstRawPtrType = const T*;

    static constexpr bool IsManaged = true;
    static constexpr bool CanCopy = false;

    static inline T* GetRaw(const PtrType& ptr) { return ptr.get(); }

    static inline RawPtrType Leak(PtrType& ptr) __WARN_UNUSED_RESULT {
        return ptr.release();
    }

    static inline PtrType Reclaim(RawPtrType ptr) {
        return PtrType(ptr);
    }
};

// Traits for managing ref_counted pointers.
template <typename T>
struct ContainerPtrTraits<::fbl::RefPtr<T>> {
    using ValueType       = T;
    using RefType         = T&;
    using ConstRefType    = const T&;
    using PtrType         = ::fbl::RefPtr<T>;
    using ConstPtrType    = ::fbl::RefPtr<const T>;
    using RawPtrType      = T*;
    using ConstRawPtrType = const T*;

    static constexpr bool IsManaged = true;
    static constexpr bool CanCopy = true;

    static inline T*       GetRaw(const PtrType& ptr)      { return ptr.get(); }
    static inline PtrType  Copy(const RawPtrType& ptr)     { return PtrType(ptr); }

    static inline RawPtrType Leak(PtrType& ptr) __WARN_UNUSED_RESULT {
        return ptr.leak_ref();
    }

    static inline PtrType Reclaim(RawPtrType ptr) {
        return ::fbl::internal::MakeRefPtrNoAdopt(ptr);
    }
};

}  // namespace internal
}  // namespace fbl
