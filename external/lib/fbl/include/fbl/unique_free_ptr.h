// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdlib.h>
#include <fbl/macros.h>
#include <fbl/type_support.h>

namespace fbl {

// This is exactly like unique_ptr except that it deallocates using free() instead of delete.
// This should only be used on types without destructors, like 'char' or C structs.
template <typename T>
class unique_free_ptr {
public:
    static_assert(has_trivial_destructor<T>::value, "unique_free_ptr can only be used on types with trivial destructors");

    constexpr unique_free_ptr() : ptr_(nullptr) {}
    constexpr unique_free_ptr(decltype(nullptr)) : unique_free_ptr() {}

    explicit unique_free_ptr(T* t) : ptr_(t) { }

    ~unique_free_ptr() {
        free(ptr_);
    }

    unique_free_ptr(unique_free_ptr&& o) : ptr_(o.release()) {}
    unique_free_ptr& operator=(unique_free_ptr&& o) {
        reset(o.release());
        return *this;
    }

    unique_free_ptr& operator=(decltype(nullptr)) {
        reset();
        return *this;
    }

    // Comparison against nullptr operators (of the form, myptr == nullptr).
    bool operator==(decltype(nullptr)) const { return (ptr_ == nullptr); }
    bool operator!=(decltype(nullptr)) const { return (ptr_ != nullptr); }

    // Comparison against other unique_free_ptr<>'s.
    bool operator==(const unique_free_ptr& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const unique_free_ptr& o) const { return ptr_ != o.ptr_; }
    bool operator< (const unique_free_ptr& o) const { return ptr_ <  o.ptr_; }
    bool operator<=(const unique_free_ptr& o) const { return ptr_ <= o.ptr_; }
    bool operator> (const unique_free_ptr& o) const { return ptr_ >  o.ptr_; }
    bool operator>=(const unique_free_ptr& o) const { return ptr_ >= o.ptr_; }

    // move semantics only
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(unique_free_ptr);

    T* release() {
        T* t = ptr_;
        ptr_ = nullptr;
        return t;
    }
    void reset(T* t = nullptr) {
        free(ptr_);
        ptr_ = t;
    }
    void swap(unique_free_ptr& other) {
        T* t = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = t;
    }

    T* get() const {
        return ptr_;
    }

    explicit operator bool() const {
        return static_cast<bool>(ptr_);
    }

    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }

private:
    T* ptr_;
};

// Comparison against nullptrs (of the form, nullptr == myptr)
template <typename T>
static inline bool operator==(decltype(nullptr), const unique_free_ptr<T>& ptr) {
    return (ptr.get() == nullptr);
}

template <typename T>
static inline bool operator!=(decltype(nullptr), const unique_free_ptr<T>& ptr) {
    return (ptr.get() != nullptr);
}

}  // namespace fbl
