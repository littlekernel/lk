/*
 * Copyright (c) 2006-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <new>

#include <__lk/abort.h>
#include <lib/heap.h>
#include <lk/compiler.h>

// The throwing forms may not return nullptr; with -fno-exceptions the only
// conforming way to report failure is to abort. malloc(0) is allowed to
// return nullptr, but operator new(0) must return a unique pointer, so the
// size is bumped first.

void *operator new(size_t s) {
    if (s == 0) {
        s = 1;
    }
    void *ptr = malloc(s);
    if (unlikely(!ptr)) {
        __LK_CXX_PANIC("operator new: allocation failed");
    }
    return ptr;
}

void *operator new[](size_t s) {
    if (s == 0) {
        s = 1;
    }
    void *ptr = malloc(s);
    if (unlikely(!ptr)) {
        __LK_CXX_PANIC("operator new[]: allocation failed");
    }
    return ptr;
}

void *operator new(size_t s, std::align_val_t align) {
    if (s == 0) {
        s = 1;
    }
    void *ptr = memalign(static_cast<size_t>(align), s);
    if (unlikely(!ptr)) {
        __LK_CXX_PANIC("operator new: aligned allocation failed");
    }
    return ptr;
}

void *operator new[](size_t s, std::align_val_t align) {
    if (s == 0) {
        s = 1;
    }
    void *ptr = memalign(static_cast<size_t>(align), s);
    if (unlikely(!ptr)) {
        __LK_CXX_PANIC("operator new[]: aligned allocation failed");
    }
    return ptr;
}

void *operator new(size_t s, const std::nothrow_t &) noexcept {
    if (s == 0) {
        s = 1;
    }
    return malloc(s);
}

void *operator new[](size_t s, const std::nothrow_t &) noexcept {
    if (s == 0) {
        s = 1;
    }
    return malloc(s);
}

void *operator new(size_t s, std::align_val_t align, const std::nothrow_t &) noexcept {
    if (s == 0) {
        s = 1;
    }
    return memalign(static_cast<size_t>(align), s);
}

void *operator new[](size_t s, std::align_val_t align, const std::nothrow_t &) noexcept {
    if (s == 0) {
        s = 1;
    }
    return memalign(static_cast<size_t>(align), s);
}

void operator delete(void *p) noexcept {
    free(p);
}

void operator delete[](void *p) noexcept {
    free(p);
}

void operator delete(void *p, size_t) noexcept {
    free(p);
}

void operator delete[](void *p, size_t) noexcept {
    free(p);
}

void operator delete(void *p, std::align_val_t) noexcept {
    free(p);
}

void operator delete[](void *p, std::align_val_t) noexcept {
    free(p);
}

void operator delete(void *p, size_t, std::align_val_t) noexcept {
    free(p);
}

void operator delete[](void *p, size_t, std::align_val_t) noexcept {
    free(p);
}

void operator delete(void *p, const std::nothrow_t &) noexcept {
    free(p);
}

void operator delete[](void *p, const std::nothrow_t &) noexcept {
    free(p);
}
