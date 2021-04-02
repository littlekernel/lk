// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/assert.h>
#include <zircon/compiler.h>
#include <fbl/atomic.h>
#include <fbl/canary.h>

namespace fbl {
namespace internal {

// Adoption validation will help to catch:
// - Double-adoptions
// - AddRef/Release without adopting first
// - Re-wrapping raw pointers to destroyed objects
//
// It also provides some limited defense against
// - Wrapping bad pointers
template <bool EnableAdoptionValidator>
class RefCountedBase {
protected:
    constexpr RefCountedBase()
        : ref_count_(kPreAdoptSentinel) {}

    ~RefCountedBase() {
        if (EnableAdoptionValidator) {
            // Reset the ref-count back to the pre-adopt sentinel value so that we
            // have the best chance of catching a use-after-free situation, even if
            // we have a messed up mix of debug/release translation units being
            // linked together.
            ref_count_.store(kPreAdoptSentinel, memory_order_release);
        }
    }

    void AddRef() const {
        const int32_t rc = ref_count_.fetch_add(1, memory_order_relaxed);

        // This assertion will fire if either of the following occur.
        //
        // 1) someone calls AddRef() before the object has been properly
        // Adopted.
        //
        // 2) someone calls AddRef() on a ref-counted object that has
        // reached ref_count_ == 0 but has not been destroyed yet. This
        // could happen by manually calling AddRef(), or re-wrapping such a
        // pointer with WrapRefPtr() or RefPtr<T>(T*) (both of which call
        // AddRef()).
        //
        // Note: leave the ASSERT on in all builds.  The constant
        // EnableAdoptionValidator check above should cause this code path to be
        // pruned in release builds, but leaving this as an always on ASSERT
        // will mean that the tests continue to function even when built as
        // release.
        if (EnableAdoptionValidator) {
            ZX_ASSERT_MSG(rc >= 1, "count %d(0x%08x) < 1\n", rc, static_cast<uint32_t>(rc));
        }
    }

    // Returns true if the object should self-delete.
    bool Release() const __WARN_UNUSED_RESULT {
        const int32_t rc = ref_count_.fetch_sub(1, memory_order_release);

        // This assertion will fire if someone manually calls Release()
        // on a ref-counted object too many times, or if Release is called
        // before an object has been Adopted.
        //
        // Note: leave the ASSERT on in all builds.  The constant
        // EnableAdoptionValidator check above should cause this code path to be
        // pruned in release builds, but leaving this as an always on ASSERT
        // will mean that the tests continue to function even when built as
        // release.
        if (EnableAdoptionValidator) {
            ZX_ASSERT_MSG(rc >= 1, "count %d(0x%08x) < 1\n", rc, static_cast<uint32_t>(rc));
        }

        if (rc == 1) {
            atomic_thread_fence(memory_order_acquire);
            return true;
        }

        return false;
    }

    void Adopt() const {
        // TODO(johngro): turn this into an if-constexpr when we have moved up
        // to C++17
        if (EnableAdoptionValidator) {
            int32_t expected = kPreAdoptSentinel;
            bool res = ref_count_.compare_exchange_strong(&expected, 1,
                                                          memory_order_acq_rel,
                                                          memory_order_acquire);
            // Note: leave the ASSERT on in all builds.  The constant
            // EnableAdoptionValidator check above should cause this code path
            // to be pruned in release builds, but leaving this as an always on
            // ASSERT will mean that the tests continue to function even when
            // built as release.
            ZX_ASSERT_MSG(res,
                          "count(0x%08x) != sentinel(0x%08x)\n",
                          static_cast<uint32_t>(expected),
                          static_cast<uint32_t>(kPreAdoptSentinel));
        } else {
            ref_count_.store(1, memory_order_release);
        }
    }

    // Current ref count. Only to be used for debugging purposes.
    int ref_count_debug() const {
        return ref_count_.load(memory_order_relaxed);
    }

    // Note:
    //
    // The PreAdoptSentinel value is chosen specifically to be negative when
    // stored as an int32_t, and as far away from becoming positiive (via either
    // addition or subtraction) as possible.  These properties allow us to
    // combine the debug-build adopt sanity checks and the lifecycle sanity
    // checks into a single debug assert.
    //
    // If a user creates an object, but never adopts it, they would need to
    // perform 0x4000000 (about 1 billion) unchecked AddRef or Release
    // operations before making the internal ref_count become positive again.
    // At this point, even a checked AddRef or Release operation would fail to
    // detect the bad state of the system fails to detect the problem.
    //
    static constexpr int32_t kPreAdoptSentinel = static_cast<int32_t>(0xC0000000);
    mutable fbl::atomic_int32_t ref_count_;
};

} // namespace internal
} // namespace fbl
