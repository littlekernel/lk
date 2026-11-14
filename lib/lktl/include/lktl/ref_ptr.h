/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// Intrusive reference counting: the count lives in the object, in an lk::ref_counted<T>
// base, and lk::ref_ptr<T> is a one-word owning pointer holding one reference.
//
// Patterns:
//
//   // a counted class
//   class widget : public lk::ref_counted<widget> {
//   public:
//       explicit widget(int id);
//       ...
//   };
//
//   // create: nothrow new plus the first reference; empty if the allocation failed
//   lk::ref_ptr<widget> w = lk::make_ref<widget>(42);
//   if (!w) {
//       return ERR_NO_MEMORY;
//   }
//
//   // share, hand off, drop
//   lk::ref_ptr<widget> other = w;               // +1
//   slots_[i] = std::move(w);                    // no count traffic; w is now empty
//   void keep(lk::ref_ptr<widget> w);            // a by-value parameter takes a reference
//   void look(const widget &w);                  // borrowing needs no ref_ptr at all
//   other.reset();                               // -1; the last reference destroys
//
//   // a raw pointer to an object something else keeps alive: take a reference of your own
//   lk::ref_ptr<widget> mine(raw);
//
//   // allocated some other way: adopt the first reference by hand, exactly once
//   lk::ref_ptr<widget> w = lk::adopt_ref(new (std::nothrow) widget(42));   // = make_ref
//
//   // a hierarchy without virtual functions: the counted base declares a static destroy()
//   // that deletes the concrete type, and the last release calls it instead of delete
//   struct object : lk::ref_counted<object> {
//       enum class kind : uint8_t { PIPE, TIMER } k;
//       static void destroy(object *o);
//   };
//   struct pipe : object { ... };
//   struct timer : object { ... };
//   void object::destroy(object *o) {            // after the derived classes are complete
//       switch (o->k) {
//           case kind::PIPE: delete static_cast<pipe *>(o); break;
//           case kind::TIMER: delete static_cast<timer *>(o); break;
//       }
//   }
//   lk::ref_ptr<object> o = lk::make_ref<pipe>();                 // derived converts to base
//   if (o->k == object::kind::PIPE) {
//       lk::ref_ptr<pipe> p = lk::static_ref_cast<pipe>(o);       // after checking the kind
//   }
//
//   // C code, or anything else holding a plain pointer: count by hand
//   w->add_ref();
//   w->release();                                // may destroy w
//
// Don'ts: never delete a counted object yourself; never adopt one that is not on the heap
// unless its destroy() knows not to free it; never adopt twice (debug builds assert); a cycle
// of ref_ptrs never frees. A class that derives from ref_counted and adds members is not
// standard-layout: reach derived types with static_cast and use lk::list_hook, never
// containerof.
//
// The count is atomic, so references can be taken and dropped on any cpu; a ref_ptr variable
// itself is no more thread-safe than a plain pointer. Debug builds start the count at an
// unadopted sentinel, so add_ref() or release() before adopt_ref(), or after the last
// release, asserts. ref_counted is constexpr constructible, and outside debug builds
// trivially destructible, so a global counted object is constant-initialized and registers
// nothing at boot.
//
// The last release calls `static void destroy(T *)` when T declares one, else deletes the T.
// Releasing through a ref_counted<T> base when the object is really a class derived from T
// therefore needs destroy() or a virtual destructor; adopt_ref() refuses to compile it
// otherwise. A private destructor needs `friend class lk::ref_counted<T>;`.

#include <assert.h>
#include <cstddef>
#include <new>
#include <stdint.h>
#include <type_traits>
#include <utility>

namespace lk {

template <typename T>
class ref_ptr;
template <typename T>
ref_ptr<T> adopt_ref(T *ptr);
template <typename T, typename U>
ref_ptr<T> static_ref_cast(ref_ptr<U> &&ptr);

namespace internal {

// true when T declares a static destroy(T *)
template <typename T, typename = void>
struct has_destroy : std::false_type {};
template <typename T>
struct has_destroy<T, std::void_t<decltype(T::destroy(static_cast<T *>(nullptr)))>>
    : std::true_type {};

} // namespace internal

template <typename T>
class ref_counted {
public:
    // the class whose destroy() or destructor the last release runs
    using ref_counted_type = T;

    constexpr ref_counted() = default;
    ref_counted(const ref_counted &) = delete;
    ref_counted &operator=(const ref_counted &) = delete;

    void add_ref() const {
        // a new reference is always copied from an existing one, which orders it
        [[maybe_unused]] int32_t old = __atomic_fetch_add(&ref_count_, 1, __ATOMIC_RELAXED);
        DEBUG_ASSERT_MSG(old > 0, "add_ref with count %d (unadopted or destroyed)\n", old);
    }

    // Drops one reference and destroys the object if it was the last. Returns true when it
    // destroyed the object.
    bool release() const {
        // release ordering: every write made under this reference happens before the destroy.
        // fetch_add of -1, not fetch_sub: i386 and the 68000 have no atomic instructions and
        // their arch code supplies only __atomic_fetch_add_4.
        int32_t old = __atomic_fetch_add(&ref_count_, -1, __ATOMIC_RELEASE);
        DEBUG_ASSERT_MSG(old > 0, "release with count %d (unadopted or destroyed)\n", old);
        if (old != 1) {
            return false;
        }
        // pairs with the release above in every other thread's last release
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        T *obj = const_cast<T *>(static_cast<const T *>(this));
        if constexpr (internal::has_destroy<T>::value) {
            T::destroy(obj);
        } else {
            delete obj;
        }
        return true;
    }

    // a snapshot for tests and debug output; another cpu may change it at any time
    int32_t ref_count_debug() const { return __atomic_load_n(&ref_count_, __ATOMIC_RELAXED); }

protected:
#if DEBUG_ASSERT_IMPLEMENTED
    // Any destructor makes a global object register itself with __cxa_atexit at boot, so the
    // check only exists in debug builds. An object goes away either never adopted or
    // through its last release.
    ~ref_counted() {
        DEBUG_ASSERT_MSG(ref_count_ == 0 || ref_count_ == kUnadopted,
                         "destroyed with count %d\n", ref_count_);
    }
#else
    ~ref_counted() = default;
#endif

private:
    template <typename U>
    friend ref_ptr<U> adopt_ref(U *ptr);

    void adopt() const {
#if DEBUG_ASSERT_IMPLEMENTED
        DEBUG_ASSERT_MSG(ref_count_ == kUnadopted, "adopt with count %d\n", ref_count_);
        ref_count_ = 1;
#endif
    }

#if DEBUG_ASSERT_IMPLEMENTED
    // far from any count a live object reaches, and negative, so add_ref/release catch it
    static constexpr int32_t kUnadopted = INT32_MIN / 2;
    mutable int32_t ref_count_ = kUnadopted;
#else
    mutable int32_t ref_count_ = 1;
#endif
};

template <typename T>
class ref_ptr {
public:
    constexpr ref_ptr() = default;
    constexpr ref_ptr(std::nullptr_t) {}

    // takes a reference of its own on an object that is already referenced elsewhere
    explicit ref_ptr(T *ptr) : ptr_(ptr) {
        if (ptr_) {
            ptr_->add_ref();
        }
    }

    ref_ptr(const ref_ptr &other) : ref_ptr(other.ptr_) {}
    ref_ptr(ref_ptr &&other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    // from a ref_ptr to a derived class
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    ref_ptr(const ref_ptr<U> &other) : ref_ptr(other.ptr_) {}
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    ref_ptr(ref_ptr<U> &&other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    ~ref_ptr() {
        if (ptr_) {
            ptr_->release();
        }
    }

    // Assignment builds the new value first and releases the old one last, so it is safe
    // against self-assignment and against a destroy() that reaches back into this pointer.
    ref_ptr &operator=(const ref_ptr &other) {
        ref_ptr(other).swap(*this);
        return *this;
    }
    ref_ptr &operator=(ref_ptr &&other) noexcept {
        ref_ptr(std::move(other)).swap(*this);
        return *this;
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    ref_ptr &operator=(const ref_ptr<U> &other) {
        ref_ptr(other).swap(*this);
        return *this;
    }
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    ref_ptr &operator=(ref_ptr<U> &&other) noexcept {
        ref_ptr(std::move(other)).swap(*this);
        return *this;
    }
    ref_ptr &operator=(std::nullptr_t) {
        reset();
        return *this;
    }

    void reset() { ref_ptr().swap(*this); }

    void swap(ref_ptr &other) noexcept {
        T *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

    T *get() const { return ptr_; }
    T &operator*() const { return *ptr_; }
    T *operator->() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }

    friend bool operator==(const ref_ptr &a, std::nullptr_t) { return a.ptr_ == nullptr; }
    friend bool operator==(std::nullptr_t, const ref_ptr &a) { return a.ptr_ == nullptr; }
    friend bool operator!=(const ref_ptr &a, std::nullptr_t) { return a.ptr_ != nullptr; }
    friend bool operator!=(std::nullptr_t, const ref_ptr &a) { return a.ptr_ != nullptr; }

private:
    template <typename U>
    friend class ref_ptr;
    template <typename U>
    friend ref_ptr<U> adopt_ref(U *ptr);
    template <typename U, typename V>
    friend ref_ptr<U> static_ref_cast(ref_ptr<V> &&ptr);

    // wraps a reference the caller already owns
    struct adopt_tag {};
    ref_ptr(T *ptr, adopt_tag) : ptr_(ptr) {}

    T *ptr_ = nullptr;
};

template <typename T, typename U>
bool operator==(const ref_ptr<T> &a, const ref_ptr<U> &b) {
    return a.get() == b.get();
}
template <typename T, typename U>
bool operator!=(const ref_ptr<T> &a, const ref_ptr<U> &b) {
    return a.get() != b.get();
}

// Takes the first reference to a newly created object. nullptr gives an empty ref_ptr, so
// the result of a nothrow new can be passed straight in.
template <typename T>
ref_ptr<T> adopt_ref(T *ptr) {
    using base = typename T::ref_counted_type;
    static_assert(std::is_same_v<std::remove_cv_t<T>, base> ||
                      internal::has_destroy<base>::value || std::has_virtual_destructor_v<base>,
                  "the last release would delete this object through its ref_counted base: "
                  "give the base a static destroy() or a virtual destructor");
    if (ptr) {
        ptr->adopt();
    }
    return ref_ptr<T>(ptr, typename ref_ptr<T>::adopt_tag{});
}

// Allocates with nothrow new and adopts; empty if the allocation failed.
template <typename T, typename... Args>
ref_ptr<T> make_ref(Args &&...args) {
    return adopt_ref(new (std::nothrow) T(std::forward<Args>(args)...));
}

// A ref_ptr to a derived class from a ref_ptr to its base. Unchecked, like static_cast:
// the caller has already established the object's real type.
template <typename T, typename U>
ref_ptr<T> static_ref_cast(const ref_ptr<U> &ptr) {
    return ref_ptr<T>(static_cast<T *>(ptr.get()));
}
template <typename T, typename U>
ref_ptr<T> static_ref_cast(ref_ptr<U> &&ptr) {
    T *p = static_cast<T *>(ptr.ptr_);
    ptr.ptr_ = nullptr;
    return ref_ptr<T>(p, typename ref_ptr<T>::adopt_tag{});
}

} // namespace lk
