// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#ifdef __cplusplus

#include <fbl/macros.h>
#include <fbl/mutex.h>
#include <fbl/null_lock.h>

namespace fbl {

// Introduce preprocessor definitions for the underlying mutex data type and the
// lock/unlock operations based on whether this code is being used in the kernel
// or in user-mode.
#ifdef _KERNEL
#define fbl_mutex_t mutex_t
#define fbl_mutex_acquire mutex_acquire
#define fbl_mutex_release mutex_release
#else
#define fbl_mutex_t mtx_t
#define fbl_mutex_acquire mtx_lock
#define fbl_mutex_release mtx_unlock
#endif

class __TA_SCOPED_CAPABILITY AutoLock {
public:
    explicit AutoLock(fbl_mutex_t* mutex) __TA_ACQUIRE(mutex)
        : mutex_(mutex), acquired_(true) {
        fbl_mutex_acquire(mutex_);
    }

    explicit AutoLock(Mutex* mutex) __TA_ACQUIRE(mutex)
        :   AutoLock(mutex->GetInternal()) {}

    explicit AutoLock(fbl::NullLock* mutex) __TA_ACQUIRE(mutex)
        : mutex_(nullptr), acquired_(false) {}

    ~AutoLock()  __TA_RELEASE() {
        release();
    }

    // early release the mutex before the object goes out of scope
    void release() __TA_RELEASE() {
        // In typical usage, this conditional will be optimized away so
        // that fbl_mutex_release() is called unconditionally.
        if (acquired_) {
            fbl_mutex_release(mutex_);
            mutex_ = nullptr;
            acquired_ = false;
        }
    }

    // suppress default constructors
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoLock);

private:
    fbl_mutex_t* mutex_;
    bool acquired_;
};

}  // namespace fbl

// Remove the underlying mutex preprocessor definitions.  Do not let them leak
// out into the world at large.
#undef fbl_mutex_t
#undef fbl_mutex_acquire
#undef fbl_mutex_release

#endif  // #ifdef __cplusplus
