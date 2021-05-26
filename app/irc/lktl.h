/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <kernel/thread.h>
#include <kernel/mutex.h>

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

#pragma once

namespace lktl {

// call a routine when the object goes out of scope
template <typename T>
class auto_call {
public:
    constexpr explicit auto_call(T call) : call_(call) {}
    ~auto_call() {
        if (armed_) {
            call_();
        }
    }

    // move
    auto_call(auto_call &&ac) : armed_(ac.armed_), call_(ac.call_) {
        ac.cancel();
    }

    void cancel() {
        armed_ = false;
    }

private:
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(auto_call);

    bool armed_ = true;
    T call_;
};

// create an auto caller with implicit template specialization.
//
// example:
//      auto ac = make_auto_call([]() { printf("a lambda!\n"); });
template <typename T>
inline auto_call<T> make_auto_call(T c) {
    return auto_call<T>(c);
}

// auto mutex scope guard
class auto_lock {
public:
    explicit auto_lock(mutex_t *m) : m_(m) {
        mutex_acquire(m_);
    }

    ~auto_lock() {
        mutex_release(m_);
    }

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(auto_lock);

    mutex_t *m_ = nullptr;
};

} // namespace lktl
