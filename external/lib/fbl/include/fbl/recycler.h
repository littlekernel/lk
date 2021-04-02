// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/assert.h>
#include <fbl/type_support.h>

// fbl::Recyclable<T>
//
// Notes:
//
// fbl::Recyclable<T> is a mix-in class which allows users to control what
// happens to objects when they reach the end of their lifecycle, as determined
// by the fbl managed pointer classes.
//
// The general idea is as follows.  A developer might have some sort of
// factory pattern where they hand out either unique_ptr<>s or RefPtr<>s
// to objects which they have created.  When their user is done with the
// object and the managed pointers let go of it, instead of executing the
// destructor and deleting the object, the developer may want to
// "recycle" the object and use it for some internal purpose.  Examples
// include...
//
// 1) Putting the object on some sort of internal list to hand out again
//    of the object is re-usable and the cost of construction/destruction
//    is high.
// 2) Putting the object into some form of deferred destruction queue
//    because users are either too high priority to pay the cost of
//    destruction when the object is released, or because the act of
//    destruction might involve operations which are not permitted when
//    the object is released (perhaps the object is released at IRQ time,
//    but the system needs to be running in a thread in order to properly
//    clean up the object)
// 3) Re-using the object internally for something like bookkeeping
//    purposes.
//
// In order to make use of the feature, users need to do two things.
//
// 1) Derive from fbl::Recyclable<T>.
// 2) Implement a method with the signature "void fbl_recycle()"
//
// When deriving from Recyclable<T>, T should be devoid of cv-qualifiers (even if
// the managed pointers handed out by the user's code are const or volatile).
// In addition, fbl_recycle must be visible to fbl::Recyclable<T>, either
// because it is public or because the T is friends with fbl::Recyclable<T>.
//
// :: Example ::
//
// Some code hands out unique pointers to const Foo objects and wishes to
// have the chance to recycle them.  The code would look something like
// this...
//
// class Foo : public fbl::Recyclable<Foo> {
// public:
//   // public implementation here
// private:
//   friend class fbl::Recyclable<Foo>;
//   void fbl_recycle() {
//     if (should_recycle())) {
//       do_recycle_stuff();
//     } else {
//       delete this;
//     }
//   }
// };
//
// Note: the intention is to use this feature with managed pointers,
// which will automatically detect and call the recycle method if
// present.  That said, there is nothing to stop users for manually
// calling fbl_recycle, provided that it is visible to the code which
// needs to call it.

namespace fbl {

// Default implementation of fbl::Recyclable.
//
// Note: we provide a default implementation instead of just a fwd declaration
// so we can add a static_assert which will give a user a more human readable
// error in case they make the mistake of deriving from fbl::Recyclable<const Foo>
// instead of fbl::Recyclable<Foo>
template <typename T, typename = void>
class Recyclable {
    // Note: static assert must depend on T in order to trigger only when the template gets
    // expanded.  If it does not depend on any template parameters, eg static_assert(false), then it
    // will always explode, regardless of whether or not the template is ever expanded.
    static_assert(is_same<T, T>::value == false,
                  "fbl::Recyclable<T> objects must not specify cv-qualifiers for T.  "
                  "IOW - derive from fbl::Recyclable<Foo>, not fbl::Recyclable<const Foo>");
};

namespace internal {

// Test to see if an object is recyclable.  An object of type T is considered to
// be recyclable if it derives from fbl::Recyclable<T>
template <typename T>
using has_fbl_recycle = is_base_of<::fbl::Recyclable<typename remove_cv<T>::type>, T>;

// internal::recycler is the internal helper class which is permitted to call
// Recyclable<T>::fbl_recycle_thunk.  It can be removed when we move to C++17
// and have the ability to prune code paths using if-constexpr before they need
// to undergo name lookup or template expansion.
template <typename T, typename = void>
struct recycler {
    static inline void recycle(T* ptr) { ZX_ASSERT(false); }
};

template <typename T>
struct recycler<T, typename enable_if<has_fbl_recycle<T>::value == true>::type> {
    static inline void recycle(T* ptr) {
        Recyclable<typename remove_cv<T>::type>::fbl_recycle_thunk(
                const_cast<typename remove_cv<T>::type *>(ptr));
    }
};

}  // namespace internal

template <typename T>
class Recyclable<T,
                 typename enable_if<is_same<typename remove_cv<T>::type, T>::value>::type> {
private:
    // Recyclable is friends with all cv-forms of the internal::recycler.  This
    // way, managed pointer types can say internal::recycler<T>::recycle(ptr)
    // without needing to worry if T is const, volatile, or both.
    friend struct ::fbl::internal::recycler<T>;
    friend struct ::fbl::internal::recycler<const T>;
    friend struct ::fbl::internal::recycler<volatile T>;
    friend struct ::fbl::internal::recycler<const volatile T>;

    static void fbl_recycle_thunk(T* ptr) {
        static_assert(is_same<decltype(&T::fbl_recycle), void (T::*)(void)>::value,
                      "fbl_recycle() methods must be non-static member "
                      "functions with the signature 'void fbl_recycle()', and "
                      "be visible to fbl::Recyclable<T> (either because they are "
                      " public, or because of friendship).");
        ptr->fbl_recycle();
    }
};

}  // namespace fbl
