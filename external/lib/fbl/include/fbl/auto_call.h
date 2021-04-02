// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <fbl/macros.h>
#include <fbl/type_support.h>

// RAII class to automatically call a function-like thing as it goes out of
// scope
//
// Examples:
//
//    extern int foo();
//    int a;
//
//    auto ac = fbl::MakeAutoCall([&](){ a = 1; });
//    auto ac2 = fbl::MakeAutoCall(foo);
//
//    auto func = [&](){ a = 2; };
//    fbl::AutoCall<decltype(func)> ac3(func);
//    fbl::AutoCall<decltype(&foo)> ac4(&foo);
//
//    // abort the call
//    ac2.cancel();
namespace fbl {

template <typename T>
class AutoCall {
public:
    constexpr explicit AutoCall(T c)
        : call_(fbl::move(c)) {}
    ~AutoCall() {
        call();
    }

    // move semantics
    AutoCall(AutoCall&& c)
        : call_(fbl::move(c.call_)), active_(c.active_) {
        c.cancel();
    }

    AutoCall& operator=(AutoCall&& c) {
        call();
        call_ = fbl::move(c.call_);
        active_ = c.active_;
        c.cancel();
        return *this;
    }

    // no copy
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(AutoCall);

    // cancel the eventual call
    void cancel() {
        active_ = false;
    }

    // call it immediately
    void call() {
        // Reset |active_| first to handle recursion (in the unlikely case it
        // should happen).
        bool active = active_;
        cancel();
        if (active)
            (call_)();
    }

private:
    T call_;
    bool active_ = true;
};

// helper routine to create an autocall object without needing template
// specialization
template <typename T>
inline AutoCall<T> MakeAutoCall(T c) {
    return AutoCall<T>(fbl::move(c));
}

} // namespace fbl
