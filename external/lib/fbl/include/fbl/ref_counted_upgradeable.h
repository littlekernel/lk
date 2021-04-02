// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/assert.h>
#include <fbl/macros.h>
#include <fbl/ref_counted_internal.h>
#include <fbl/ref_ptr.h>

namespace fbl {
namespace internal {

// A variant of the standard RefCouted base class which allows an
// "UpgradeFromRaw" operation which can be used to give weak-pointer like
// behavior for very specific use cases.
//
template <bool EnableAdoptionValidator>
class RefCountedUpgradeableBase : public RefCountedBase<EnableAdoptionValidator> {
public:
    RefCountedUpgradeableBase() = default;
    ~RefCountedUpgradeableBase() = default;

    // RefCountedUpgradeableBase<> instances may not be copied, assigned or moved.
    DISALLOW_COPY_ASSIGN_AND_MOVE(RefCountedUpgradeableBase);

    // This method must only be called from MakeRefPtrUpgradeFromRaw.  See its
    // comments for details in the proper use of this method. The actual job of
    // this function is to atomically increment the refcount if the refcount is
    // greater than zero.
    //
    // This method returns false if the object was found with an invalid
    // refcount (refcount was <= 0), and true if the refcount was not zero and
    // it was incremented.
    //
    // The procedure used is the while-CAS loop with the advantage that
    // compare_exchange on failure updates |old| on failure (to exchange) so the
    // loop does not have to do a separate load.
    //
    bool AddRefMaybeInDestructor() const __WARN_UNUSED_RESULT {
        int32_t old = this->ref_count_.load(memory_order_acquire);
        do {
            if (old <= 0) {
                return false;
            }
        } while (!this->ref_count_.compare_exchange_weak(&old,
                                                         old + 1,
                                                         memory_order_acq_rel,
                                                         memory_order_acquire));
        return true;
    }
};

}  // namespace internal

// A variant of the standard RefCouted base class which allows an
// "UpgradeFromRaw" operation which can be used to give weak-pointer like
// behavior for a very specific use case in the kernel.
//
template <typename T,
          bool EnableAdoptionValidator = ZX_DEBUG_ASSERT_IMPLEMENTED>
class RefCountedUpgradeable : private internal::RefCountedUpgradeableBase<EnableAdoptionValidator> {
public:
    RefCountedUpgradeable() = default;
    ~RefCountedUpgradeable() = default;

    using internal::RefCountedBase<EnableAdoptionValidator>::AddRef;
    using internal::RefCountedBase<EnableAdoptionValidator>::Release;
    using internal::RefCountedBase<EnableAdoptionValidator>::Adopt;
    using internal::RefCountedBase<EnableAdoptionValidator>::ref_count_debug;
    using internal::RefCountedUpgradeableBase<EnableAdoptionValidator>::AddRefMaybeInDestructor;

    // RefCountedUpgradeable<> instances may not be copied, assigned or moved.
    DISALLOW_COPY_ASSIGN_AND_MOVE(RefCountedUpgradeable);
};

// Constructs a RefPtr from a raw T* which is being held alive by RefPtr
// with the caveat that the existing RefPtr might be in the process of
// destructing the T object. When the T object is in the destructor, the
// resulting RefPtr is null, otherwise the resulting RefPtr points to T*
// with the updated reference count.
//
// The only way for this to be a valid pattern is that the call is made
// while holding |lock| and that the same lock also is used to protect the
// value of T* .
//
// This pattern is needed in collaborating objects which cannot hold a
// RefPtr to each other because it would cause a reference cycle. Instead
// there is a raw pointer from one to the other and a RefPtr in the
// other direction. When needed the raw pointer can be upgraded via
// MakeRefPtrUpgradeFromRaw() and operated outside |lock|.
//
// For example:
//
//  class Holder: public RefCounted<Holder> {
//  public:
//      void add_client(Client* c) {
//          Autolock al(&lock_);
//          client_ = c;
//      }
//
//      void remove_client() {
//          Autolock al(&lock_);
//          client_ = nullptr;
//      }
//
//      void PassClient(Bar* bar) {
//          fbl::RefPtr<Client> rc_client;
//          {
//              Autolock al(&lock_);
//              if (client_) {
//                  rc_client = fbl::internal::MakeRefPtrUpgradeFromRaw(client_, lock_);
//              }
//          }
//
//          if (rc_client) {
//              bar->Client(move(rc_client));  // Bar might keep a ref to client.
//          } else {
//              bar->OnNoClient();
//          }
//      }
//
//  private:
//      fbl::Mutex lock_;
//      Client* client __TA_REQUIRES(lock_);
//  };
//
//  class Client: public RefCounted<Client> {
//  public:
//      Client(RefPtr<Holder> holder) : holder_(move(holder)) {
//          holder_->add_client(this);
//      }
//
//      ~Client() {
//          holder_->remove_client();
//      }
//  private:
//      fbl::RefPtr<Holder> holder_;
//  };
//
//

template <typename T, typename LockCapability>
inline RefPtr<T> MakeRefPtrUpgradeFromRaw(T* ptr, const LockCapability& lock) __TA_REQUIRES(lock) {
    if (!ptr->AddRefMaybeInDestructor()) {
        return nullptr;
    }

    return internal::MakeRefPtrNoAdopt(ptr);
}

}  // namespace fbl
