//
// Copyright (c) 2021 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#pragma once

#include <type_traits>

// Helper routines used in C++ code in LK

// Macro used to simplify the task of deleting all of the default copy
// constructors and assignment operators.
#define DISALLOW_COPY_ASSIGN_AND_MOVE(_class_name)       \
    _class_name(const _class_name&) = delete;            \
    _class_name(_class_name&&) = delete;                 \
    _class_name& operator=(const _class_name&) = delete; \
    _class_name& operator=(_class_name&&) = delete

// Macro used to simplify the task of deleting the non rvalue reference copy
// constructors and assignment operators.  (IOW - forcing move semantics)
#define DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(_class_name) \
    _class_name(const _class_name&) = delete;            \
    _class_name& operator=(const _class_name&) = delete

// Macro used to simplify the task of deleting the new and new[]
// operators. (IOW - disallow heap allocations)
#define DISALLOW_NEW                       \
    static void* operator new(size_t) = delete;   \
    static void* operator new[](size_t) = delete

// TODO: find a better place for this
namespace lk {

template <typename T>
class auto_call {
public:
    constexpr explicit auto_call(T c) : call_(std::move(c)) {}
    ~auto_call() { call(); }

    auto_call(auto_call && c) : call_(std::move(c.call_)), armed_(c.armed_) {
        c.cancel();
    }
    auto_call& operator=(auto_call&& c) {
        call();
        call_ = std::move(c.call_);
        armed_ = c.armed_;
        c.cancel();
        return *this;
    }

    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(auto_call);

    void call() {
        bool armed = armed_;
        cancel();
        if (armed) {
            call_();
        }
    }
    void cancel() {
        armed_ = false;
    }

private:
    T call_;
    bool armed_ = true;
};

template <typename T>
inline auto_call<T> make_auto_call(T c) {
    return auto_call<T>(std::move(c));
}

} // namespace lk


