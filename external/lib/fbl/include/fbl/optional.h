// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <assert.h>

#include <fbl/new.h>
#include <fbl/type_support.h>

namespace fbl {

namespace {

template <typename T>
void swap(T& t1, T& t2) {
    T temp(fbl::move(t1));
    t1 = fbl::move(t2);
    t2 = fbl::move(temp);
}

} // namespace

// A sentinel value for |fbl::optional<T>| indicating that it contains
// no value.
struct nullopt_t {
    explicit constexpr nullopt_t(int) {}
};
static constexpr nullopt_t nullopt(0);

// A minimal implementation of an |std::optional<T>| work-alike for C++ 14.
template <typename T>
class optional final {
public:
    constexpr optional()
        : has_value_(false) {}
    constexpr optional(nullopt_t)
        : has_value_(false) {}

    explicit constexpr optional(T value)
        : has_value_(true), value_(fbl::move(value)) {}

    optional(const optional& other)
        : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) T(other.value_);
        }
    }

    optional(optional&& other)
        : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) T(fbl::move(other.value_));
            other.value_.~T();
            other.has_value_ = false;
        }
    }

    // TODO(US-90): Presence of this destructor makes the type non-literal.
    // We should specialize this type to handle the case where T is literal
    // explicitly so that expressions these types can be constexpr.
    ~optional() {
        if (has_value_) {
            value_.~T();
        }
    }

    constexpr T& value() {
        assert(has_value_);
        return value_;
    }

    constexpr const T& value() const {
        assert(has_value_);
        return value_;
    }

    template <typename U = T>
    constexpr T value_or(U&& default_value) const {
        return has_value_ ? value_ : static_cast<T>(fbl::forward<U>(default_value));
    }

    constexpr T* operator->() { return &value_; }
    constexpr const T* operator->() const { return &value_; }
    constexpr T& operator*() { return value_; }
    constexpr const T& operator*() const { return value_; }

    bool has_value() const { return has_value_; }
    explicit operator bool() const { return has_value(); }

    optional& operator=(const optional& other) {
        if (&other == this)
            return *this;
        if (has_value_) {
            if (other.has_value_) {
                value_ = other.value_;
            } else {
                reset();
            }
        } else if (other.has_value_) {
            new (&value_) T(other.value_);
            has_value_ = true;
        }
        return *this;
    }

    optional& operator=(optional&& other) {
        if (&other == this)
            return *this;
        if (has_value_) {
            if (other.has_value_) {
                value_ = fbl::move(other.value_);
                other.value_.~T();
                other.has_value_ = false;
            } else {
                reset();
            }
        } else if (other.has_value_) {
            new (&value_) T(fbl::move(other.value_));
            has_value_ = true;
            other.value_.~T();
            other.has_value_ = false;
        }
        return *this;
    }

    optional& operator=(nullopt_t) {
        reset();
        return *this;
    }

    optional& operator=(T value) {
        if (has_value_) {
            value_ = fbl::move(value);
        } else {
            new (&value_) T(fbl::move(value));
            has_value_ = true;
        }
        return *this;
    }

    void reset() {
        if (has_value_) {
            value_.~T();
            has_value_ = false;
        }
    }

    void swap(optional& other) {
        if (&other == this)
            return;
        if (has_value_) {
            if (other.has_value_) {
              fbl::swap(value_, other.value_);
            } else {
                new (&other.value_) T(fbl::move(value_));
                other.has_value_ = true;
                value_.~T();
                has_value_ = false;
            }
        } else if (other.has_value_) {
            new (&value_) T(fbl::move(other.value_));
            has_value_ = true;
            other.value_.~T();
            other.has_value_ = false;
        }
    }

private:
    bool has_value_;
    union {
        T value_;
    };
};

template <typename T>
void swap(optional<T>& a, optional<T>& b) {
    a.swap(b);
}

template <typename T>
constexpr bool operator==(const optional<T>& lhs, nullopt_t) {
    return !lhs.has_value();
}
template <typename T>
constexpr bool operator!=(const optional<T>& lhs, nullopt_t) {
    return lhs.has_value();
}

template <typename T>
constexpr bool operator==(nullopt_t, const optional<T>& rhs) {
    return !rhs.has_value();
}
template <typename T>
constexpr bool operator!=(nullopt_t, const optional<T>& rhs) {
    return rhs.has_value();
}

template <typename T, typename U>
constexpr bool operator==(const optional<T>& lhs, const optional<U>& rhs) {
    return (lhs.has_value() == rhs.has_value()) && (!lhs.has_value() || *lhs == *rhs);
}
template <typename T, typename U>
constexpr bool operator!=(const optional<T>& lhs, const optional<U>& rhs) {
    return (lhs.has_value() != rhs.has_value()) || (lhs.has_value() && *lhs != *rhs);
}

template <typename T, typename U>
constexpr bool operator==(const optional<T>& lhs, const U& rhs) {
    return lhs.has_value() && *lhs == rhs;
}
template <typename T, typename U>
constexpr bool operator!=(const optional<T>& lhs, const U& rhs) {
    return !lhs.has_value() || *lhs != rhs;
}

template <typename T, typename U>
constexpr bool operator==(const T& lhs, const optional<U>& rhs) {
    return rhs.has_value() && lhs == *rhs;
}
template <typename T, typename U>
constexpr bool operator!=(const T& lhs, const optional<U>& rhs) {
    return !rhs.has_value() || lhs != *rhs;
}

} // namespace fbl
