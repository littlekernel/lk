// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stddef.h>

#include <zircon/assert.h>
#include <fbl/algorithm.h>
#include <fbl/alloc_checker.h>
#include <fbl/macros.h>
#include <fbl/new.h>
#include <fbl/type_support.h>
#include <fbl/unique_ptr.h>

namespace fbl {
namespace internal {

// Checks if |T| is null. Defaults to false. |Comparison| is the type yielded by
// comparing a T value with nullptr.
template <typename T, typename Comparison = bool>
struct NullEq {
    static constexpr bool Test(const T&) { return false; }
};

// Partial specialization for |T| values comparable to nullptr.
template <typename T>
struct NullEq<T, decltype(*static_cast<T*>(nullptr) == nullptr)> {
    // This is intended for a T that's a function pointer type.  However, it
    // also matches for a T that can be implicitly coerced to a function
    // pointer type, such as a function type or a captureless lambda's closure
    // type.  In that case, the compiler might complain that the comparison is
    // always false because the address of a function can never be a null
    // pointer.  It's possible to do template selection to match function
    // types, but it's not possible to match captureless lambda closure types
    // that way.  So just suppress the warning.  The compiler will optimize
    // away the always-false comparison.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
    static constexpr bool Test(const T& v) { return v == nullptr; }
#pragma GCC diagnostic pop
};

template <typename T>
static constexpr bool IsNull(const T& v) {
    return NullEq<T>::Test(v);
}

template <typename Result, typename... Args>
class FunctionTarget {
public:
    FunctionTarget() = default;
    virtual ~FunctionTarget() = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(FunctionTarget);

    virtual bool is_null() const = 0;

    virtual Result operator()(Args... args) const = 0;

    virtual void MoveInitializeTo(void* ptr) = 0;
};

template <typename Result, typename... Args>
class NullFunctionTarget final : public FunctionTarget<Result, Args...> {
public:
    NullFunctionTarget() = default;
    ~NullFunctionTarget() final = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(NullFunctionTarget);

    bool is_null() const final { return true; }

    Result operator()(Args... args) const final {
        ZX_PANIC("Attempted to invoke fbl::Function with a null target.");
    }

    void MoveInitializeTo(void* ptr) final {
        new (ptr) NullFunctionTarget();
    }
};

template <typename Callable, typename Result, typename... Args>
class InlineFunctionTarget final : public FunctionTarget<Result, Args...> {
public:
    explicit InlineFunctionTarget(Callable target)
        : target_(fbl::move(target)) {}
    InlineFunctionTarget(Callable target, AllocChecker* ac)
        : target_(fbl::move(target)) { ac->arm(0u, true); }
    InlineFunctionTarget(InlineFunctionTarget&& other)
        : target_(fbl::move(other.target_)) {}
    ~InlineFunctionTarget() final = default;

    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(InlineFunctionTarget);

    bool is_null() const final { return false; }

    Result operator()(Args... args) const final {
        return target_(fbl::forward<Args>(args)...);
    }

    void MoveInitializeTo(void* ptr) final {
        new (ptr) InlineFunctionTarget(fbl::move(*this));
    }

private:
    mutable Callable target_;
};

template <typename Callable, typename Result, typename... Args>
class HeapFunctionTarget final : public FunctionTarget<Result, Args...> {
public:
    explicit HeapFunctionTarget(Callable target)
        : target_ptr_(fbl::make_unique<Callable>(fbl::move(target))) {}
    HeapFunctionTarget(Callable target, AllocChecker* ac)
        : target_ptr_(fbl::make_unique_checked<Callable>(ac, fbl::move(target))) {}
    HeapFunctionTarget(HeapFunctionTarget&& other)
        : target_ptr_(fbl::move(other.target_ptr_)) {}
    ~HeapFunctionTarget() final = default;

    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(HeapFunctionTarget);

    bool is_null() const final { return false; }

    Result operator()(Args... args) const final {
        return (*target_ptr_)(fbl::forward<Args>(args)...);
    }

    void MoveInitializeTo(void* ptr) final {
        new (ptr) HeapFunctionTarget(fbl::move(*this));
    }

private:
    fbl::unique_ptr<Callable> target_ptr_;
};

// Holds a function target.
// If a callable object is small enough, it will be stored as an |InlineFunctionTarget|.
// Otherwise it will be stored as a |HeapFunctionTarget|.
template <size_t target_size, typename Result, typename... Args>
struct FunctionTargetHolder final {
    FunctionTargetHolder() = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(FunctionTargetHolder);

    void InitializeNullTarget() {
        using NullFunctionTarget = fbl::internal::NullFunctionTarget<Result, Args...>;
        static_assert(sizeof(NullFunctionTarget) <= target_size,
                      "NullFunctionTarget should fit in FunctionTargetHolder.");
        new (&bits_) NullFunctionTarget();
    }

    template <typename Callable>
    struct TargetHelper {
        using InlineFunctionTarget = fbl::internal::InlineFunctionTarget<Callable, Result, Args...>;
        using HeapFunctionTarget = fbl::internal::HeapFunctionTarget<Callable, Result, Args...>;
        static constexpr bool can_inline = (sizeof(InlineFunctionTarget) <= target_size);
        using Type = typename fbl::conditional<can_inline, InlineFunctionTarget, HeapFunctionTarget>::type;
        static_assert(sizeof(Type) <= target_size, "Target should fit in FunctionTargetHolder.");
    };

    template <typename Callable>
    void InitializeTarget(Callable target) {
        new (&bits_) typename TargetHelper<Callable>::Type(fbl::move(target));
    }

    template <typename Callable>
    void InitializeTarget(Callable target, AllocChecker* ac) {
        new (&bits_) typename TargetHelper<Callable>::Type(fbl::move(target), ac);
    }

    void MoveInitializeTargetFrom(FunctionTargetHolder& other) {
        other.target().MoveInitializeTo(&bits_);
    }

    void DestroyTarget() {
        target().~FunctionTarget();
    }

    using FunctionTarget = fbl::internal::FunctionTarget<Result, Args...>;
    FunctionTarget& target() { return *reinterpret_cast<FunctionTarget*>(&bits_); }
    const FunctionTarget& target() const { return *reinterpret_cast<const FunctionTarget*>(&bits_); }

private:
    alignas(max_align_t) union { char data[target_size]; } bits_;
};

template <size_t inline_callable_size, bool require_inline, typename Result, typename... Args>
class Function;

template <size_t inline_callable_size, bool require_inline, typename Result, typename... Args>
class Function<inline_callable_size, require_inline, Result(Args...)> {
    struct FakeCallable {
        alignas(max_align_t) char bits[fbl::round_up(inline_callable_size, sizeof(void*))];
    };
    static constexpr size_t inline_target_size =
        sizeof(InlineFunctionTarget<FakeCallable, Result, Args...>);
    static constexpr size_t heap_target_size =
        sizeof(HeapFunctionTarget<FakeCallable, Result, Args...>);
    static constexpr size_t target_size = require_inline ? inline_target_size
                                                         : fbl::max(inline_target_size, heap_target_size);
    using TargetHolder = FunctionTargetHolder<target_size, Result, Args...>;

public:
    using result_type = Result;

    Function() { holder_.InitializeNullTarget(); }

    Function(decltype(nullptr)) { holder_.InitializeNullTarget(); }

    Function(Function&& other) {
        holder_.MoveInitializeTargetFrom(other.holder_);
        other.holder_.InitializeNullTarget();
    }

    template <typename Callable>
    Function(Callable target) {
        InitializeTarget(fbl::move(target));
    }

    template <typename Callable>
    Function(Callable target, AllocChecker* ac) {
        InitializeTarget(fbl::move(target), ac);
    }

    ~Function() {
        holder_.DestroyTarget();
    }

    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(Function);

    explicit operator bool() const {
        return !holder_.target().is_null();
    };

    Result operator()(Args... args) const {
        return holder_.target()(fbl::forward<Args>(args)...);
    }

    Function& operator=(decltype(nullptr)) {
        holder_.DestroyTarget();
        holder_.InitializeNullTarget();
        return *this;
    }

    Function& operator=(Function&& other) {
        holder_.DestroyTarget();
        holder_.MoveInitializeTargetFrom(other.holder_);
        other.holder_.InitializeNullTarget();
        return *this;
    }

    template <typename Callable>
    Function& operator=(Callable target) {
        SetTarget(fbl::move(target));
        return *this;
    }

    template <typename Callable>
    void SetTarget(Callable target) {
        holder_.DestroyTarget();
        InitializeTarget(fbl::move(target));
    }

    template <typename Callable>
    void SetTarget(Callable target, AllocChecker* ac) {
        holder_.DestroyTarget();
        InitializeTarget(fbl::move(target), ac);
    }

    void swap(Function& other) {
        TargetHolder temp;
        temp.MoveInitializeTargetFrom(holder_);
        holder_.MoveInitializeTargetFrom(other.holder_);
        other.holder_.MoveInitializeTargetFrom(temp);
    }

private:
    template <typename Callable>
    void InitializeTarget(Callable target) {
        static_assert(!require_inline || sizeof(Callable) <= inline_callable_size,
                      "Callable too large for InlineFunction.");
        if (IsNull(target)) {
            holder_.InitializeNullTarget();
        } else {
            holder_.InitializeTarget(fbl::move(target));
        }
    }

    template <typename Callable>
    void InitializeTarget(Callable target, AllocChecker* ac) {
        static_assert(!require_inline || sizeof(Callable) <= inline_callable_size,
                      "Callable too large for InlineFunction.");
        if (IsNull(target)) {
            holder_.InitializeNullTarget();
        } else {
            holder_.InitializeTarget(fbl::move(target), ac);
        }
    }

    TargetHolder holder_;
};

// Helper used by |BindMember| to invoke a pointer to member function.
template <typename R, typename T, typename... Args>
class MemberInvoker final {
public:
    using MemFn = R (T::*)(Args...);

    MemberInvoker(T* instance, MemFn fn)
        : instance_(instance), fn_(fn) {}

    R operator()(Args... args) const {
        return (instance_->*fn_)(fbl::forward<Args>(args)...);
    }

private:
    T* const instance_;
    MemFn const fn_;
};

} // namespace internal

// The default size allowance for callable objects which can be inlined within
// a function object.  This default allows for inline storage of callables
// consiting of a function pointer and an object pointer (or similar callables
// of the same size).
constexpr size_t kDefaultInlineCallableSize = sizeof(void*) * 2;

// A move-only callable object wrapper.
//
// fbl::Function<T> behaves like std::function<T> except that it is move-only
// instead of copyable.  This means it can hold mutable lambdas without requiring
// a reference-counted wrapper.
//
// Small callable objects (smaller than |kDefaultInlineCallableSize|) are stored
// inline within the function.  Larger callable objects will be copied to the heap
// if necessary.
//
// See also fbl::SizedFunction<T, size> and fbl::InlineFunction<T, size>
// for more control over allocation behavior.
//
// SYNOPSIS:
//
// template <typename Result, typename Args...>
// class Function<Result(Args...)> {
// public:
//     using result_type = Result;
//
//     Function();
//     explicit Function(decltype(nullptr));
//     Function(Function&& other);
//     template <typename Callable>
//     explicit Function(Callable target);
//     template <typename Callable>
//     explicit Function(Callable target, AllocChecker* ac);
//
//     ~Function();
//
//     explicit operator bool() const;
//
//     Result operator()(Args... args) const;
//
//     Function& operator=(decltype(nullptr));
//     Function& operator=(Function&& other);
//     template <typename Callable>
//     Function& operator=(Callable target);
//
//     template <typename Callable>
//     void SetTarget(Callable target);
//     template <typename Callable>
//     void SetTarget(Callable target, AllocChecker* ac);
//
//     void swap(Function& other);
// };
//
// EXAMPLE:
//
// using FoldFunction = fbl::Function<int(int value, int item)>;
//
// int FoldVector(const fbl::Vector<int>& in, int value, const FoldFunction& f) {
//     for (auto& item : in) {
//         value = f(value, item);
//     }
//     return value;
// }
//
// int SumItem(int value, int item) {
//     return value + item;
// }
//
// int Sum(const fbl::Vector<int>& in) {
//     // bind to a function pointer
//     FoldFunction sum(&SumItem);
//     return FoldVector(in, 0, sum);
// }
//
// int AlternatingSum(const fbl::Vector<int>& in) {
//     // bind to a lambda
//     int sign = 1;
//     FoldFunction alternating_sum([&sign](int value, int item) {
//         value += sign * item;
//         sign *= -1;
//         return value;
//     });
//     return FoldVector(in, 0, alternating_sum);
// }
//
template <typename T>
using Function = fbl::internal::Function<kDefaultInlineCallableSize, false, T>;

// A move-only callable object wrapper with a explicitly specified (non-default)
// inline callable size preference.
//
// Behaves just like fbl::Function<T> except that it guarantees that callable
// objects of up to |inline_callable_size| bytes will be stored inline instead
// of on the heap.  This may be useful when you want to optimize storage of
// functions of a known size.
//
// Note that the effective maximum inline callable size may be slightly larger
// due to object alignment and rounding.
template <typename T, size_t inline_callable_size>
using SizedFunction = fbl::internal::Function<inline_callable_size, false, T>;

// A move-only callable object wrapper which forces callables to be stored inline
// thereby preventing heap allocation.
//
// Behaves just like fbl::Function<T> except that it will refuse to store a
// callable object larger than |inline_callable_size| (will fail to compile).
template <typename T, size_t inline_callable_size>
using InlineFunction = fbl::internal::Function<inline_callable_size, true, T>;

// Comparing functions with nullptr.
template <size_t inline_callable_size, bool require_inline, typename Result, typename... Args>
bool operator==(const fbl::internal::Function<inline_callable_size, require_inline, Result, Args...>& f,
                decltype(nullptr)) {
    return !f;
}
template <size_t inline_callable_size, bool require_inline, typename Result, typename... Args>
bool operator!=(const fbl::internal::Function<inline_callable_size, require_inline, Result, Args...>& f,
                decltype(nullptr)) {
    return !!f;
}
template <size_t inline_callable_size, bool require_inline, typename Result, typename... Args>
bool operator==(decltype(nullptr),
                const fbl::internal::Function<inline_callable_size, require_inline, Result, Args...>& f) {
    return !f;
}
template <size_t inline_callable_size, bool require_inline, typename Result, typename... Args>
bool operator!=(decltype(nullptr),
                const fbl::internal::Function<inline_callable_size, require_inline, Result, Args...>& f) {
    return !!f;
}

// A function which takes no arguments and produces no result.
using Closure = fbl::Function<void()>;

// Returns a Callable object which invokes a member function of an object.
//
// EXAMPLE:
//
// class Accumulator {
// public:
//     void Add(int value) {
//          sum += value;
//     }
//
//     int sum = 0;
// };
//
// void CountToTen(fbl::Function<void(int)> function) {
//     for (int i = 1; i <= 10; i++) {
//         function(i);
//     }
// }
//
// int SumToTen() {
//     Accumulator accum;
//     CountToTen(fbl::BindMember(&accum, &Accumulator::Add));
//     return accum.sum;
// }
template <typename R, typename T, typename... Args>
auto BindMember(T* instance, R (T::*fn)(Args...)) {
    return internal::MemberInvoker<R, T, Args...>(instance, fn);
}

} // namespace fbl
