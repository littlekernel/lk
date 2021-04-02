// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <fbl/intrusive_pointer_traits.h>
#include <fbl/type_support.h>

namespace fbl {

// DefaultKeyedObjectTraits defines a default implementation of traits used to
// manage objects stored in associative containers such as hash-tables and
// trees.
//
// At a minimum, a class or a struct which is to be used to define the
// traits of a keyed object must define the following public members.
//
// GetKey   : A static method which takes a constant reference to an object (the
//            type of which is infered from PtrType) and returns a KeyType
//            instance corresponding to the key for an object.
// LessThan : A static method which takes two keys (key1 and key2) and returns
//            true if-and-only-if key1 is considered to be less than key2 for
//            sorting purposes.
// EqualTo  : A static method which takes two keys (key1 and key2) and returns
//            true if-and-only-if key1 is considered to be equal to key2.
//
// Rules for keys:
// ++ The type of key returned by GetKey must be compatible with the key which
//    was specified for the container.
// ++ The key for an object must remain constant for as long as the object is
//    contained within a container.
// ++ When comparing keys, comparisons must obey basic transative and
//    commutative properties.  That is to say...
//    LessThan(A, B) and LessThan(B, C) implies LessThan(A, C)
//    EqualTo(A, B) and EqualTo(B, C) implies EqualTo(A, C)
//    EqualTo(A, B) if-and-only-if EqualTo(B, A)
//    LessThan(A, B) if-and-only-if EqualTo(B, A) or (not LessThan(B, A))
//
// DefaultKeyedObjectTraits is a helper class which allows an object to be
// treated as a keyed-object by implementing a const GetKey method which returns
// a key of the appropriate type.  The key type must be compatible with the
// container key type, and must have definitions of the < and == operators for
// the purpose of generating implementation of LessThan and EqualTo.
template <typename KeyType, typename ObjType>
struct DefaultKeyedObjectTraits {
    static KeyType GetKey(const ObjType& obj)                       { return obj.GetKey(); }
    static bool LessThan(const KeyType& key1, const KeyType& key2)  { return key1 <  key2; }
    static bool EqualTo (const KeyType& key1, const KeyType& key2)  { return key1 == key2; }
};

}  // namespace fbl

namespace fbl {
namespace internal {

// DirectEraseUtils
//
// A utility class used by HashTable to implement an O(n) or O(k) direct erase
// operation depending on whether or not the HashTable's bucket type supports
// O(k) erase.
template <typename ContainerType, typename Enable = void>
struct DirectEraseUtils;

template <typename ContainerType>
struct DirectEraseUtils<
        ContainerType,
        typename enable_if<ContainerType::SupportsConstantOrderErase == false, void>::type> {
    using PtrTraits = typename ContainerType::PtrTraits;
    using PtrType   = typename PtrTraits::PtrType;
    using ValueType = typename PtrTraits::ValueType;

    static PtrType erase(ContainerType& container, ValueType& obj) {
        return container.erase_if(
            [&obj](const ValueType& other) -> bool {
                return &obj == &other;
            });
    }
};

template <typename ContainerType>
struct DirectEraseUtils<
        ContainerType,
        typename enable_if<ContainerType::SupportsConstantOrderErase == true, void>::type> {
    using PtrTraits = typename ContainerType::PtrTraits;
    using PtrType   = typename PtrTraits::PtrType;
    using ValueType = typename PtrTraits::ValueType;

    static PtrType erase(ContainerType& container, ValueType& obj) {
        return container.erase(obj);
    }
};

// KeyEraseUtils
//
// A utility class used by HashTable to implement an O(n) or O(k) erase-by-key
// operation depending on whether or not the HashTable's bucket type is
// associative or not.
template <typename ContainerType, typename KeyTraits, typename Enable = void>
struct KeyEraseUtils;

template <typename ContainerType, typename KeyTraits>
struct KeyEraseUtils<
        ContainerType,
        KeyTraits,
        typename enable_if<ContainerType::IsAssociative == false, void>::type> {
    using PtrTraits = typename ContainerType::PtrTraits;
    using PtrType   = typename PtrTraits::PtrType;
    using ValueType = typename PtrTraits::ValueType;

    template <typename KeyType>
    static PtrType erase(ContainerType& container, const KeyType& key) {
        return container.erase_if(
            [key](const ValueType& other) -> bool {
                return KeyTraits::EqualTo(key, KeyTraits::GetKey(other));
            });
    }
};

template <typename ContainerType, typename KeyTraits>
struct KeyEraseUtils<
        ContainerType,
        KeyTraits,
        typename enable_if<ContainerType::IsAssociative == true, void>::type> {
    using PtrTraits = typename ContainerType::PtrTraits;
    using PtrType   = typename PtrTraits::PtrType;

    template <typename KeyType>
    static PtrType erase(ContainerType& container, const KeyType& key) {
        return container.erase(key);
    }
};

// Swaps two plain old data types with size no greater than 64 bits.
template <class T, class = typename enable_if<is_pod<T>::value && (sizeof(T) <= 8)>::type>
inline void Swap(T& a, T& b) noexcept {
    T tmp = a;
    a = b;
    b = tmp;
}

// Notes on container sentinels:
//
// Intrusive container implementations employ a slightly tricky pattern where
// sentinel values are used in place of nullptr in various places in the
// internal data structure in order to make some operations a bit easier.
// Generally speaking, a sentinel pointer is a pointer to a container with the
// sentinel bit set.  It is cast and stored in the container's data structure as
// a pointer to an element which the container contains, even though it is
// actually a slightly damaged pointer to the container itself.
//
// An example of where this is used is in the doubly linked list implementation.
// The final element in the list holds the container's sentinel value instead of
// nullptr or a pointer to the head of the list.  When an iterator hits the end
// of the list, it knows that it is at the end (because the sentinel bit is set)
// but can still get back to the list itself (by clearing the sentinel bit in
// the pointer) without needing to store an explicit pointer to the list itself.
//
// Care must be taken when using sentinel values.  They are *not* valid pointers
// and must never be dereferenced, recovered into an managed representation, or
// returned to a user.  In addition, it is essential that a legitimate pointer
// to a container never need to set the sentinel bit.  Currently, bit 0 is being
// used because it should never be possible to have a proper container instance
// which is odd-aligned.
constexpr uintptr_t kContainerSentinelBit = 1u;

// Create a sentinel pointer from a raw pointer, converting it to the specified
// type in the process.
template <typename T, typename U, typename = typename enable_if<is_pointer<T>::value>::type>
constexpr T make_sentinel(U* ptr) {
    return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(ptr) |
                               kContainerSentinelBit);
}

template <typename T, typename = typename enable_if<is_pointer<T>::value>::type>
constexpr T make_sentinel(decltype(nullptr)) {
    return reinterpret_cast<T>(kContainerSentinelBit);
}

// Turn a sentinel pointer back into a normal pointer, automatically
// re-interpreting its type in the process.
template <typename T, typename U, typename = typename enable_if<is_pointer<T>::value>::type>
constexpr T unmake_sentinel(U* sentinel) {
    return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(sentinel) &
                               ~kContainerSentinelBit);
}

// Test to see if a pointer is a sentinel pointer.
template <typename T>
constexpr bool is_sentinel_ptr(const T* ptr) {
    return (reinterpret_cast<uintptr_t>(ptr) & kContainerSentinelBit) != 0;
}

// Test to see if a pointer (which may be a sentinel) is valid.  Valid in this
// context means that the pointer is not null, and is not a sentinel.
template <typename T>
constexpr bool valid_sentinel_ptr(const T* ptr) {
    return ptr && !is_sentinel_ptr(ptr);
}

}  // namespace internal
}  // namespace fbl
