// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/atomic.h>

#include <fbl/limits.h>
#include <unittest/unittest.h>

namespace {

// A struct with an interesting size for pointer arithmetic tests.
struct S {
    char bytes[48];
};

using function_pointer = void(*)();

bool atomic_explicit_declarations_test() {
    BEGIN_TEST;

    [[gnu::unused]] fbl::atomic<char> zero_char(0);
    [[gnu::unused]] fbl::atomic<signed char> zero_schar(0);
    [[gnu::unused]] fbl::atomic<unsigned char> zero_uchar(0);
    [[gnu::unused]] fbl::atomic<short> zero_short(0);
    [[gnu::unused]] fbl::atomic<unsigned short> zero_ushort(0);
    [[gnu::unused]] fbl::atomic<int> zero_int(0);
    [[gnu::unused]] fbl::atomic<unsigned int> zero_uint(0);
    [[gnu::unused]] fbl::atomic<long> zero_long(0);
    [[gnu::unused]] fbl::atomic<unsigned long> zero_ulong(0);
    [[gnu::unused]] fbl::atomic<long long> zero_llong(0);
    [[gnu::unused]] fbl::atomic<unsigned long long> zero_ullong(0);

    [[gnu::unused]] fbl::atomic<intptr_t> zero_intptr_t(0);
    [[gnu::unused]] fbl::atomic<uintptr_t> zero_uintptr_t(0);
    [[gnu::unused]] fbl::atomic<size_t> zero_size_t(0);
    [[gnu::unused]] fbl::atomic<ptrdiff_t> zero_ptrdiff_t(0);
    [[gnu::unused]] fbl::atomic<intmax_t> zero_intmax_t(0);
    [[gnu::unused]] fbl::atomic<uintmax_t> zero_uintmax_t(0);

    [[gnu::unused]] fbl::atomic<int8_t> zero_int8_t(0);
    [[gnu::unused]] fbl::atomic<uint8_t> zero_uint8_t(0);
    [[gnu::unused]] fbl::atomic<int16_t> zero_int16_t(0);
    [[gnu::unused]] fbl::atomic<uint16_t> zero_uint16_t(0);
    [[gnu::unused]] fbl::atomic<int32_t> zero_int32_t(0);
    [[gnu::unused]] fbl::atomic<uint32_t> zero_uint32_t(0);
    [[gnu::unused]] fbl::atomic<int64_t> zero_int64_t(0);
    [[gnu::unused]] fbl::atomic<uint64_t> zero_uint64_t(0);

    [[gnu::unused]] fbl::atomic<int_least8_t> zero_int_least8_t(0);
    [[gnu::unused]] fbl::atomic<uint_least8_t> zero_uint_least8_t(0);
    [[gnu::unused]] fbl::atomic<int_least16_t> zero_int_least16_t(0);
    [[gnu::unused]] fbl::atomic<uint_least16_t> zero_uint_least16_t(0);
    [[gnu::unused]] fbl::atomic<int_least32_t> zero_int_least32_t(0);
    [[gnu::unused]] fbl::atomic<uint_least32_t> zero_uint_least32_t(0);
    [[gnu::unused]] fbl::atomic<int_least64_t> zero_int_least64_t(0);
    [[gnu::unused]] fbl::atomic<uint_least64_t> zero_uint_least64_t(0);
    [[gnu::unused]] fbl::atomic<int_fast8_t> zero_int_fast8_t(0);
    [[gnu::unused]] fbl::atomic<uint_fast8_t> zero_uint_fast8_t(0);
    [[gnu::unused]] fbl::atomic<int_fast16_t> zero_int_fast16_t(0);
    [[gnu::unused]] fbl::atomic<uint_fast16_t> zero_uint_fast16_t(0);
    [[gnu::unused]] fbl::atomic<int_fast32_t> zero_int_fast32_t(0);
    [[gnu::unused]] fbl::atomic<uint_fast32_t> zero_uint_fast32_t(0);
    [[gnu::unused]] fbl::atomic<int_fast64_t> zero_int_fast64_t(0);
    [[gnu::unused]] fbl::atomic<uint_fast64_t> zero_uint_fast64_t(0);

    [[gnu::unused]] fbl::atomic<bool> zero_bool(false);

    [[gnu::unused]] fbl::atomic<void*> zero_void_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<const void*> zero_const_void_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<volatile void*> zero_volatile_void_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<const volatile void*> zero_const_volatile_void_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<int*> zero_int_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<const int*> zero_const_int_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<volatile int*> zero_volatile_int_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<const volatile int*> zero_const_volatile_int_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<S*> zero_S_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<const S*> zero_const_S_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<volatile S*> zero_volatile_S_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<const volatile S*> zero_const_volatile_S_pointer(nullptr);
    [[gnu::unused]] fbl::atomic<function_pointer> zero_function_pointer(nullptr);

    END_TEST;
}

bool atomic_using_declarations_test() {
    BEGIN_TEST;

    [[gnu::unused]] fbl::atomic_char zero_char(0);
    [[gnu::unused]] fbl::atomic_schar zero_schar(0);
    [[gnu::unused]] fbl::atomic_uchar zero_uchar(0);
    [[gnu::unused]] fbl::atomic_short zero_short(0);
    [[gnu::unused]] fbl::atomic_ushort zero_ushort(0);
    [[gnu::unused]] fbl::atomic_int zero_int(0);
    [[gnu::unused]] fbl::atomic_uint zero_uint(0);
    [[gnu::unused]] fbl::atomic_long zero_long(0);
    [[gnu::unused]] fbl::atomic_ulong zero_ulong(0);
    [[gnu::unused]] fbl::atomic_llong zero_llong(0);
    [[gnu::unused]] fbl::atomic_ullong zero_ullong(0);

    [[gnu::unused]] fbl::atomic_intptr_t zero_intptr_t(0);
    [[gnu::unused]] fbl::atomic_uintptr_t zero_uintptr_t(0);
    [[gnu::unused]] fbl::atomic_size_t zero_size_t(0);
    [[gnu::unused]] fbl::atomic_ptrdiff_t zero_ptrdiff_t(0);
    [[gnu::unused]] fbl::atomic_intmax_t zero_intmax_t(0);
    [[gnu::unused]] fbl::atomic_uintmax_t zero_uintmax_t(0);

    [[gnu::unused]] fbl::atomic_int8_t zero_int8_t(0);
    [[gnu::unused]] fbl::atomic_uint8_t zero_uint8_t(0);
    [[gnu::unused]] fbl::atomic_int16_t zero_int16_t(0);
    [[gnu::unused]] fbl::atomic_uint16_t zero_uint16_t(0);
    [[gnu::unused]] fbl::atomic_int32_t zero_int32_t(0);
    [[gnu::unused]] fbl::atomic_uint32_t zero_uint32_t(0);
    [[gnu::unused]] fbl::atomic_int64_t zero_int64_t(0);
    [[gnu::unused]] fbl::atomic_uint64_t zero_uint64_t(0);

    [[gnu::unused]] fbl::atomic_int_least8_t zero_int_least8_t(0);
    [[gnu::unused]] fbl::atomic_uint_least8_t zero_uint_least8_t(0);
    [[gnu::unused]] fbl::atomic_int_least16_t zero_int_least16_t(0);
    [[gnu::unused]] fbl::atomic_uint_least16_t zero_uint_least16_t(0);
    [[gnu::unused]] fbl::atomic_int_least32_t zero_int_least32_t(0);
    [[gnu::unused]] fbl::atomic_uint_least32_t zero_uint_least32_t(0);
    [[gnu::unused]] fbl::atomic_int_least64_t zero_int_least64_t(0);
    [[gnu::unused]] fbl::atomic_uint_least64_t zero_uint_least64_t(0);
    [[gnu::unused]] fbl::atomic_int_fast8_t zero_int_fast8_t(0);
    [[gnu::unused]] fbl::atomic_uint_fast8_t zero_uint_fast8_t(0);
    [[gnu::unused]] fbl::atomic_int_fast16_t zero_int_fast16_t(0);
    [[gnu::unused]] fbl::atomic_uint_fast16_t zero_uint_fast16_t(0);
    [[gnu::unused]] fbl::atomic_int_fast32_t zero_int_fast32_t(0);
    [[gnu::unused]] fbl::atomic_uint_fast32_t zero_uint_fast32_t(0);
    [[gnu::unused]] fbl::atomic_int_fast64_t zero_int_fast64_t(0);
    [[gnu::unused]] fbl::atomic_uint_fast64_t zero_uint_fast64_t(0);

    [[gnu::unused]] fbl::atomic_bool zero_bool(false);

    END_TEST;
}

// To increase test readability after this point, static_assert that
// most of these are the same as fbl::atomic<some other type>. That
// way no one has to read a million lines of test code about
// fbl::atomic_uint_least32_t.

template <typename T>
constexpr bool IsSameAsSomeBuiltin() {
    return fbl::is_same<T, fbl::atomic_char>::value ||
           fbl::is_same<T, fbl::atomic_schar>::value ||
           fbl::is_same<T, fbl::atomic_uchar>::value ||
           fbl::is_same<T, fbl::atomic_short>::value ||
           fbl::is_same<T, fbl::atomic_ushort>::value ||
           fbl::is_same<T, fbl::atomic_int>::value ||
           fbl::is_same<T, fbl::atomic_uint>::value ||
           fbl::is_same<T, fbl::atomic_long>::value ||
           fbl::is_same<T, fbl::atomic_ulong>::value ||
           fbl::is_same<T, fbl::atomic_llong>::value ||
           fbl::is_same<T, fbl::atomic_ullong>::value ||
           fbl::is_same<T, fbl::atomic_bool>::value;
}

static_assert(IsSameAsSomeBuiltin<fbl::atomic_intptr_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uintptr_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_size_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_ptrdiff_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_intmax_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uintmax_t>(), "");

static_assert(IsSameAsSomeBuiltin<fbl::atomic_int8_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint8_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int16_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint16_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int32_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint32_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int64_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint64_t>(), "");

static_assert(IsSameAsSomeBuiltin<fbl::atomic_int_least8_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint_least8_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int_least16_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint_least16_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int_least32_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint_least32_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int_least64_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint_least64_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int_fast8_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint_fast8_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int_fast16_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint_fast16_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int_fast32_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint_fast32_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_int_fast64_t>(), "");
static_assert(IsSameAsSomeBuiltin<fbl::atomic_uint_fast64_t>(), "");

// We should be able to instantiate fbl::atomics of enum type as well.

enum unspecified_enum {
    kUnspecifiedValue = 23,
};
__UNUSED fbl::atomic<unspecified_enum> atomic_unspecified_enum;

enum specified_enum_char : char {
    kSpecifiedValue_char = 23,
};
__UNUSED fbl::atomic<specified_enum_char> atomic_specified_enum_char;

enum specified_enum_signed_char : signed char {
    kSpecifiedValue_signed_char = 23,
};
__UNUSED fbl::atomic<specified_enum_signed_char> atomic_specified_enum_signed_char;

enum specified_enum_unsigned_char : unsigned char {
    kSpecifiedValue_unsigned_char = 23,
};
__UNUSED fbl::atomic<specified_enum_unsigned_char> atomic_specified_enum_unsigned_char;

enum specified_enum_short : short {
    kSpecifiedValue_short = 23,
};
__UNUSED fbl::atomic<specified_enum_short> atomic_specified_enum_short;

enum specified_enum_unsigned_short : unsigned short {
    kSpecifiedValue_unsigned_short = 23,
};
__UNUSED fbl::atomic<specified_enum_unsigned_short> atomic_specified_enum_unsigned_short;

enum specified_enum_int : int {
    kSpecifiedValue_int = 23,
};
__UNUSED fbl::atomic<specified_enum_int> atomic_specified_enum_int;

enum specified_enum_unsigned_int : unsigned int {
    kSpecifiedValue_unsigned_int = 23,
};
__UNUSED fbl::atomic<specified_enum_unsigned_int> atomic_specified_enum_unsigned_int;

enum specified_enum_long : long {
    kSpecifiedValue_long = 23,
};
__UNUSED fbl::atomic<specified_enum_long> atomic_specified_enum_long;

enum specified_enum_unsigned_long : unsigned long{
    kSpecifiedValue_unsigned_long = 23,
};
__UNUSED fbl::atomic<specified_enum_unsigned_long> atomic_specified_enum_unsigned_long;

enum specified_enum_long_long : long long {
    kSpecifiedValue_long_long = 23,
};
__UNUSED fbl::atomic<specified_enum_long_long> atomic_specified_enum_long_long;

enum specified_enum_unsigned_long_long : unsigned long long {
    kSpecifiedValue_unsigned_long_long = 23,
};
__UNUSED fbl::atomic<specified_enum_unsigned_long_long> atomic_specified_enum_unsigned_long_long;

enum specified_enum_bool : bool {
    kSpecifiedValue_bool = true,
};
__UNUSED fbl::atomic<specified_enum_bool> atomic_specified_enum_bool;

enum struct unspecified_struct_enum {
    kUnspecifiedValueStruct = 23,
};
__UNUSED fbl::atomic<unspecified_struct_enum> atomic_unspecified_struct_enum;

enum struct specified_struct_enum_char : char {
    kSpecifiedStructValue_char = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_char> atomic_specified_struct_enum_char;

enum struct specified_struct_enum_signed_char : signed char {
    kSpecifiedStructValue_signed_char = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_signed_char> atomic_specified_struct_enum_signed_char;

enum struct specified_struct_enum_unsigned_char : unsigned char {
    kSpecifiedStructValue_unsigned_char = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_unsigned_char> atomic_specified_struct_enum_unsigned_char;

enum struct specified_struct_enum_short : short {
    kSpecifiedStructValue_short = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_short> atomic_specified_struct_enum_short;

enum struct specified_struct_enum_unsigned_short : unsigned short {
    kSpecifiedStructValue_unsigned_short = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_unsigned_short> atomic_specified_struct_enum_unsigned_short;

enum struct specified_struct_enum_int : int {
    kSpecifiedStructValue_int = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_int> atomic_specified_struct_enum_int;

enum struct specified_struct_enum_unsigned_int : unsigned int {
    kSpecifiedStructValue_unsigned_int = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_unsigned_int> atomic_specified_struct_enum_unsigned_int;

enum struct specified_struct_enum_long : long {
    kSpecifiedStructValue_long = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_long> atomic_specified_struct_enum_long;

enum struct specified_struct_enum_unsigned_long : unsigned long{
    kSpecifiedStructValue_unsigned_long = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_unsigned_long> atomic_specified_struct_enum_unsigned_long;

enum struct specified_struct_enum_long_long : long long {
    kSpecifiedStructValue_long_long = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_long_long> atomic_specified_struct_enum_long_long;

enum struct specified_struct_enum_unsigned_long_long : unsigned long long {
    kSpecifiedStructValue_unsigned_long_long = 23,
};
__UNUSED fbl::atomic<specified_struct_enum_unsigned_long_long> atomic_specified_struct_enum_unsigned_long_long;

enum struct specified_struct_enum_bool : bool {
    kSpecifiedStructValue_bool = true,
};
__UNUSED fbl::atomic<specified_struct_enum_bool> atomic_specified_struct_enum_bool;

bool atomic_wont_compile_test() {
    BEGIN_TEST;

    // fbl::atomic only supports integer, enum, and pointer types.

#if TEST_WILL_NOT_COMPILE || 0
    struct not_integral {};
    fbl::atomic<not_integral> not_integral;
#endif

#if TEST_WILL_NOT_COMPILE || 0
    fbl::atomic<float> not_integral;
#endif

#if TEST_WILL_NOT_COMPILE || 0
    fbl::atomic<double> not_integral;
#endif

    END_TEST;
}

fbl::memory_order orders[] = {
    fbl::memory_order_relaxed,
    fbl::memory_order_acquire,
    fbl::memory_order_release,
    fbl::memory_order_acq_rel,
    fbl::memory_order_seq_cst,
};

// Bunch of machinery for arithmetic tests.
template <typename T>
using ordinary_op = T (*)(T*, T);

template <typename T>
using atomic_op = T (*)(fbl::atomic<T>*, T, fbl::memory_order);

template <typename T>
using volatile_op = T (*)(volatile fbl::atomic<T>*, T, fbl::memory_order);

template <typename T>
struct TestCase {
    ordinary_op<T> ordinary;
    atomic_op<T> nonmember_atomic;
    atomic_op<T> member_atomic;
    volatile_op<T> nonmember_volatile;
    volatile_op<T> member_volatile;
};

template <typename T>
T test_values[] = {
    0,
    1,
    23,
    fbl::numeric_limits<T>::min() / 4,
    fbl::numeric_limits<T>::max() / 4,
};

template <>
bool test_values<bool>[] = {
    false,
    true,
    true,
    false,
    true,
};

template <>
void* test_values<void*>[] = {
    &test_values<int>[0],
    &test_values<int>[1],
    &test_values<int>[2],
    &test_values<int>[3],
    &test_values<int>[4],
};

template <>
const void* test_values<const void*>[] = {
    &test_values<int>[0],
    &test_values<int>[1],
    &test_values<int>[2],
    &test_values<int>[3],
    &test_values<int>[4],
};

template <>
volatile void* test_values<volatile void*>[] = {
    &test_values<int>[0],
    &test_values<int>[1],
    &test_values<int>[2],
    &test_values<int>[3],
    &test_values<int>[4],
};

template <>
const volatile void* test_values<const volatile void*>[] = {
    &test_values<int>[0],
    &test_values<int>[1],
    &test_values<int>[2],
    &test_values<int>[3],
    &test_values<int>[4],
};

template <>
int* test_values<int*>[] = {
    &test_values<int>[0],
    &test_values<int>[1],
    &test_values<int>[2],
    &test_values<int>[3],
    &test_values<int>[4],
};

template <>
const int* test_values<const int*>[] = {
    &test_values<int>[0],
    &test_values<int>[1],
    &test_values<int>[2],
    &test_values<int>[3],
    &test_values<int>[4],
};

template <>
volatile int* test_values<volatile int*>[] = {
    &test_values<int>[0],
    &test_values<int>[1],
    &test_values<int>[2],
    &test_values<int>[3],
    &test_values<int>[4],
};

template <>
const volatile int* test_values<const volatile int*>[] = {
    &test_values<int>[0],
    &test_values<int>[1],
    &test_values<int>[2],
    &test_values<int>[3],
    &test_values<int>[4],
};

S test_values_of_S[] = { {}, {}, {}, {} };

template <>
S* test_values<S*>[] = {
    &test_values_of_S[0],
    &test_values_of_S[1],
    &test_values_of_S[2],
    &test_values_of_S[3],
    nullptr,
};

template <>
const S* test_values<const S*>[] = {
    &test_values_of_S[0],
    &test_values_of_S[1],
    &test_values_of_S[2],
    &test_values_of_S[3],
    nullptr,
};

template <>
volatile S* test_values<volatile S*>[] = {
    &test_values_of_S[0],
    &test_values_of_S[1],
    &test_values_of_S[2],
    &test_values_of_S[3],
    nullptr,
};

template <>
const volatile S* test_values<const volatile S*>[] = {
    &test_values_of_S[0],
    &test_values_of_S[1],
    &test_values_of_S[2],
    &test_values_of_S[3],
    nullptr,
};

// Try to force each of these to be different so that the test
// continues working under ICF. The CAS tests compare function pointer
// values, so it's important that these have different addresses.
static volatile int volatile_0;
void nothing_0() {
    volatile_0 = 0;
}
static volatile int volatile_1;
void nothing_1() {
    volatile_1 = 1;
}
static volatile int volatile_2;
void nothing_2() {
    volatile_2 = 2;
}
static volatile int volatile_3;
void nothing_3() {
    volatile_3 = 3;
}

template <>
function_pointer test_values<function_pointer>[] = {
    &nothing_0,
    &nothing_1,
    &nothing_2,
    &nothing_3,
    nullptr,
};

template <typename T>
TestCase<T> test_cases[] = {
    {
        [](T* ptr_to_a, T b) -> T {
            T a = *ptr_to_a;
            *ptr_to_a = static_cast<T>(a + b);
            return a;
        },
        fbl::atomic_fetch_add<T>,
        [](fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
            return ptr_to_atomic_a->fetch_add(b, order);
        },
        fbl::atomic_fetch_add<T>,
        [](volatile fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
            return ptr_to_atomic_a->fetch_add(b, order);
        },
    },
    {
        [](T* ptr_to_a, T b) -> T {
            T a = *ptr_to_a;
            *ptr_to_a = static_cast<T>(a & b);
            return a;
        },
        fbl::atomic_fetch_and<T>,
        [](fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
            return ptr_to_atomic_a->fetch_and(b, order);
        },
        fbl::atomic_fetch_and<T>,
        [](volatile fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
            return ptr_to_atomic_a->fetch_and(b, order);
        },
    },
    {
        [](T* ptr_to_a, T b) -> T {
            T a = *ptr_to_a;
            *ptr_to_a = static_cast<T>(a | b);
            return a;
        },
        fbl::atomic_fetch_or<T>,
        [](fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
            return ptr_to_atomic_a->fetch_or(b, order);
        },
        fbl::atomic_fetch_or<T>,
        [](volatile fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
            return ptr_to_atomic_a->fetch_or(b, order);
        },
    },
    {
        [](T* ptr_to_a, T b) -> T {
            T a = *ptr_to_a;
            *ptr_to_a = static_cast<T>(a ^ b);
            return a;
        },
        fbl::atomic_fetch_xor<T>,
        [](fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
            return ptr_to_atomic_a->fetch_xor(b, order);
        },
        fbl::atomic_fetch_xor<T>,
        [](volatile fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
            return ptr_to_atomic_a->fetch_xor(b, order);
        },
    },
};

template <typename T>
TestCase<T> subtraction_test_case = {
    [](T* ptr_to_a, T b) -> T {
        T a = *ptr_to_a;
        *ptr_to_a = static_cast<T>(a - b);
        return a;
    },
    fbl::atomic_fetch_sub,
    [](fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
        return ptr_to_atomic_a->fetch_sub(b, order);
    },
    fbl::atomic_fetch_sub,
    [](volatile fbl::atomic<T>* ptr_to_atomic_a, T b, fbl::memory_order order) -> T {
        return ptr_to_atomic_a->fetch_sub(b, order);
    },
};

template <typename T>
bool math_test(bool &all_ok) {
    for (const T original_left : test_values<T>) {
        for (T right : test_values<T>) {
            for (const auto& order : orders) {
                for (auto test_case : test_cases<T>) {
                    {
                        fbl::atomic<T> atomic_left(original_left);
                        T left = original_left;
                        ASSERT_EQ(test_case.ordinary(&left, right),
                                  test_case.member_atomic(&atomic_left, right, order),
                                  "Atomic and ordinary math differ");
                        ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
                    }
                    {
                        fbl::atomic<T> atomic_left(original_left);
                        T left = original_left;
                        ASSERT_EQ(test_case.ordinary(&left, right),
                                  test_case.nonmember_atomic(&atomic_left, right, order),
                                  "Atomic and ordinary math differ");
                        ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
                    }
                    {
                        volatile fbl::atomic<T> atomic_left(original_left);
                        T left = original_left;
                        ASSERT_EQ(test_case.ordinary(&left, right),
                                  test_case.member_volatile(&atomic_left, right, order),
                                  "Atomic and ordinary math differ");
                        ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
                    }
                    {
                        volatile fbl::atomic<T> atomic_left(original_left);
                        T left = original_left;
                        ASSERT_EQ(test_case.ordinary(&left, right),
                                  test_case.nonmember_volatile(&atomic_left, right, order),
                                  "Atomic and ordinary math differ");
                        ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
                    }
                }
                // Let's not worry about signed subtraction UB.
                if (fbl::is_unsigned<T>::value) {
                    {
                        fbl::atomic<T> atomic_left(original_left);
                        T left = original_left;
                        ASSERT_EQ(subtraction_test_case<T>.ordinary(&left, right),
                                  subtraction_test_case<T>.member_atomic(&atomic_left, right, order),
                                  "Atomic and ordinary math differ");
                        ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
                    }
                    {
                        fbl::atomic<T> atomic_left(original_left);
                        T left = original_left;
                        ASSERT_EQ(subtraction_test_case<T>.ordinary(&left, right),
                                  subtraction_test_case<T>.nonmember_atomic(&atomic_left, right, order),
                                  "Atomic and ordinary math differ");
                        ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
                    }
                    {
                        volatile fbl::atomic<T> atomic_left(original_left);
                        T left = original_left;
                        ASSERT_EQ(subtraction_test_case<T>.ordinary(&left, right),
                                  subtraction_test_case<T>.member_volatile(&atomic_left, right, order),
                                  "Atomic and ordinary math differ");
                        ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
                    }
                    {
                        volatile fbl::atomic<T> atomic_left(original_left);
                        T left = original_left;
                        ASSERT_EQ(subtraction_test_case<T>.ordinary(&left, right),
                                  subtraction_test_case<T>.nonmember_volatile(&atomic_left, right, order),
                                  "Atomic and ordinary math differ");
                        ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
                    }
                }
            }
        }
    }

    return true;
}

template <typename T>
using ordinary_pointer_op = T (*)(T*, ptrdiff_t);

template <typename T>
using atomic_pointer_op = T (*)(fbl::atomic<T>*, ptrdiff_t, fbl::memory_order);

template <typename T>
using volatile_pointer_op = T (*)(volatile fbl::atomic<T>*, ptrdiff_t, fbl::memory_order);

template <typename T>
struct PointerTestCase {
    ordinary_pointer_op<T> ordinary;
    atomic_pointer_op<T> nonmember_atomic;
    atomic_pointer_op<T> member_atomic;
    volatile_pointer_op<T> nonmember_volatile;
    volatile_pointer_op<T> member_volatile;
};

template <typename T>
PointerTestCase<T> pointer_add_test_case = {
    [](T* ptr_to_a, ptrdiff_t d) -> T {
        T a = *ptr_to_a;
        *ptr_to_a = a + d;
        return a;
    },
    [](fbl::atomic<T>* ptr_to_atomic_a, ptrdiff_t d, fbl::memory_order order) -> T {
        return fbl::atomic_fetch_add(ptr_to_atomic_a, d, order);
    },
    [](fbl::atomic<T>* ptr_to_atomic_a, ptrdiff_t d, fbl::memory_order order) -> T {
        return ptr_to_atomic_a->fetch_add(d, order);
    },
    [](volatile fbl::atomic<T>* ptr_to_atomic_a, ptrdiff_t d, fbl::memory_order order) -> T {
        return fbl::atomic_fetch_add(ptr_to_atomic_a, d, order);
    },
    [](volatile fbl::atomic<T>* ptr_to_atomic_a, ptrdiff_t d, fbl::memory_order order) -> T {
        return ptr_to_atomic_a->fetch_add(d, order);
    },
};

template <typename T>
PointerTestCase<T> pointer_sub_test_case = {
    [](T* ptr_to_a, ptrdiff_t d) -> T {
        T a = *ptr_to_a;
        *ptr_to_a = a - d;
        return a;
    },
    [](fbl::atomic<T>* ptr_to_atomic_a, ptrdiff_t d, fbl::memory_order order) -> T {
        return fbl::atomic_fetch_sub(ptr_to_atomic_a, d, order);
    },
    [](fbl::atomic<T>* ptr_to_atomic_a, ptrdiff_t d, fbl::memory_order order) -> T {
        return ptr_to_atomic_a->fetch_sub(d, order);
    },
    [](volatile fbl::atomic<T>* ptr_to_atomic_a, ptrdiff_t d, fbl::memory_order order) -> T {
        return fbl::atomic_fetch_sub(ptr_to_atomic_a, d, order);
    },
    [](volatile fbl::atomic<T>* ptr_to_atomic_a, ptrdiff_t d, fbl::memory_order order) -> T {
        return ptr_to_atomic_a->fetch_sub(d, order);
    },
};

template <typename T>
bool pointer_add_test(bool &all_ok) {
    static_assert(fbl::is_pointer<T>::value, "");
    ptrdiff_t right = 2;
    const auto& test_case = pointer_add_test_case<T>;
    for (const T original_left : test_values<T>) {
        for (const auto& order : orders) {
            {
                fbl::atomic<T> atomic_left(original_left);
                T left = original_left;
                ASSERT_EQ(test_case.ordinary(&left, right),
                          test_case.member_atomic(&atomic_left, right, order),
                          "Atomic and ordinary math differ");
                ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
            }
            {
                fbl::atomic<T> atomic_left(original_left);
                T left = original_left;
                ASSERT_EQ(test_case.ordinary(&left, right),
                          test_case.nonmember_atomic(&atomic_left, right, order),
                          "Atomic and ordinary math differ");
                ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
            }
            {
                volatile fbl::atomic<T> atomic_left(original_left);
                T left = original_left;
                ASSERT_EQ(test_case.ordinary(&left, right),
                          test_case.member_volatile(&atomic_left, right, order),
                          "Atomic and ordinary math differ");
                ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
            }
            {
                volatile fbl::atomic<T> atomic_left(original_left);
                T left = original_left;
                ASSERT_EQ(test_case.ordinary(&left, right),
                          test_case.nonmember_volatile(&atomic_left, right, order),
                          "Atomic and ordinary math differ");
                ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
            }
        }

        right -= 1;
    }

    return true;
}

template <typename T>
bool pointer_sub_test(bool &all_ok) {
    static_assert(fbl::is_pointer<T>::value, "");
    ptrdiff_t right = -2;
    const auto& test_case = pointer_sub_test_case<T>;
    for (const T original_left : test_values<T>) {
        for (const auto& order : orders) {
            {
                fbl::atomic<T> atomic_left(original_left);
                T left = original_left;
                ASSERT_EQ(test_case.ordinary(&left, right),
                          test_case.member_atomic(&atomic_left, right, order),
                          "Atomic and ordinary math differ");
                ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
            }
            {
                fbl::atomic<T> atomic_left(original_left);
                T left = original_left;
                ASSERT_EQ(test_case.ordinary(&left, right),
                          test_case.nonmember_atomic(&atomic_left, right, order),
                          "Atomic and ordinary math differ");
                ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
            }
            {
                volatile fbl::atomic<T> atomic_left(original_left);
                T left = original_left;
                ASSERT_EQ(test_case.ordinary(&left, right),
                          test_case.member_volatile(&atomic_left, right, order),
                          "Atomic and ordinary math differ");
                ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
            }
            {
                volatile fbl::atomic<T> atomic_left(original_left);
                T left = original_left;
                ASSERT_EQ(test_case.ordinary(&left, right),
                          test_case.nonmember_volatile(&atomic_left, right, order),
                          "Atomic and ordinary math differ");
                ASSERT_EQ(left, atomic_load(&atomic_left), "Atomic and ordinary math differ");
            }
        }

        right += 1;
    }

    return true;
}

template <typename T>
bool load_store_test(bool &all_ok) {
    fbl::atomic<T> atomic_value;

    for (T value : test_values<T>) {
        atomic_value.store(value);
        ASSERT_EQ(atomic_value.load(), value, "Member load/store busted");
    }

    for (T value : test_values<T>) {
        fbl::atomic_store(&atomic_value, value);
        ASSERT_EQ(atomic_load(&atomic_value), value, "Nonmember load/store busted");
    }

    volatile fbl::atomic<T> volatile_value;

    for (T value : test_values<T>) {
        volatile_value.store(value);
        ASSERT_EQ(volatile_value.load(), value, "Member load/store busted");
    }

    for (T value : test_values<T>) {
        fbl::atomic_store(&volatile_value, value);
        ASSERT_EQ(atomic_load(&volatile_value), value, "Nonmember load/store busted");
    }

    return true;
}

template <typename T>
bool exchange_test(bool &all_ok) {
    T last_value = test_values<T>[0];
    fbl::atomic<T> atomic_value(last_value);

    for (T value : test_values<T>) {
        ASSERT_EQ(atomic_value.load(), last_value, "Member exchange busted");
        ASSERT_EQ(atomic_value.exchange(value), last_value, "Member exchange busted");
        last_value = value;
    }

    last_value = test_values<T>[0];
    atomic_value.store(last_value);

    for (T value : test_values<T>) {
        ASSERT_EQ(fbl::atomic_load(&atomic_value), last_value, "Nonmember exchange busted");
        ASSERT_EQ(fbl::atomic_exchange(&atomic_value, value), last_value, "Nonmember exchange busted");
        last_value = value;
    }

    last_value = test_values<T>[0];
    volatile fbl::atomic<T> volatile_value(last_value);

    for (T value : test_values<T>) {
        ASSERT_EQ(volatile_value.load(), last_value, "Member exchange busted");
        ASSERT_EQ(volatile_value.exchange(value), last_value, "Member exchange busted");
        last_value = value;
    }

    last_value = test_values<T>[0];
    volatile_value.store(last_value);

    for (T value : test_values<T>) {
        ASSERT_EQ(fbl::atomic_load(&volatile_value), last_value, "Nonmember exchange busted");
        ASSERT_EQ(fbl::atomic_exchange(&volatile_value, value), last_value, "Nonmember exchange busted");
        last_value = value;
    }

    return true;
}

template <typename T>
struct cas_function {
    bool (*function)(fbl::atomic<T>* atomic_ptr, T* expected, T desired,
                     fbl::memory_order success_order, fbl::memory_order failure_order);
    bool can_spuriously_fail;
};

template <typename T>
cas_function<T> cas_functions[] = {
    {fbl::atomic_compare_exchange_weak, true},
    {fbl::atomic_compare_exchange_strong, false},
    {[](fbl::atomic<T>* atomic_ptr, T* expected, T desired,
        fbl::memory_order success_order, fbl::memory_order failure_order) {
         return atomic_ptr->compare_exchange_weak(expected, desired, success_order, failure_order);
     },
     true},
    {[](fbl::atomic<T>* atomic_ptr, T* expected, T desired,
        fbl::memory_order success_order, fbl::memory_order failure_order) {
         return atomic_ptr->compare_exchange_strong(expected, desired, success_order, failure_order);
     },
     false},
};

template <typename T>
struct volatile_cas_function {
    bool (*function)(volatile fbl::atomic<T>* atomic_ptr, T* expected, T desired,
                     fbl::memory_order success_order, fbl::memory_order failure_order);
    bool can_spuriously_fail;
};

template <typename T>
volatile_cas_function<T> volatile_cas_functions[] = {
    {fbl::atomic_compare_exchange_weak, true},
    {fbl::atomic_compare_exchange_strong, false},
    {[](volatile fbl::atomic<T>* atomic_ptr, T* expected, T desired,
        fbl::memory_order success_order, fbl::memory_order failure_order) {
         return atomic_ptr->compare_exchange_weak(expected, desired, success_order, failure_order);
     },
     true},
    {[](volatile fbl::atomic<T>* atomic_ptr, T* expected, T desired,
        fbl::memory_order success_order, fbl::memory_order failure_order) {
         return atomic_ptr->compare_exchange_strong(expected, desired, success_order, failure_order);
     },
     false}};

enum cas_slots {
    kExpected = 0,
    kActual = 1,
    kDesired = 2,
};

template <typename T>
T cas_test_values[] = {
    22,
    23,
    24,
};

template <>
bool cas_test_values<bool>[] = {
    false,
    true,
    false,
};

template <>
void* cas_test_values<void*>[] = {
    &cas_test_values<int>[0],
    &cas_test_values<int>[1],
    &cas_test_values<int>[2],
};

template <>
const void* cas_test_values<const void*>[] = {
    &cas_test_values<int>[0],
    &cas_test_values<int>[1],
    &cas_test_values<int>[2],
};

template <>
volatile void* cas_test_values<volatile void*>[] = {
    &cas_test_values<int>[0],
    &cas_test_values<int>[1],
    &cas_test_values<int>[2],
};

template <>
const volatile void* cas_test_values<const volatile void*>[] = {
    &cas_test_values<int>[0],
    &cas_test_values<int>[1],
    &cas_test_values<int>[2],
};

template <>
int* cas_test_values<int*>[] = {
    &cas_test_values<int>[0],
    &cas_test_values<int>[1],
    &cas_test_values<int>[2],
};

template <>
const int* cas_test_values<const int*>[] = {
    &cas_test_values<int>[0],
    &cas_test_values<int>[1],
    &cas_test_values<int>[2],
};

template <>
volatile int* cas_test_values<volatile int*>[] = {
    &cas_test_values<int>[0],
    &cas_test_values<int>[1],
    &cas_test_values<int>[2],
};

template <>
const volatile int* cas_test_values<const volatile int*>[] = {
    &cas_test_values<int>[0],
    &cas_test_values<int>[1],
    &cas_test_values<int>[2],
};

S cas_test_values_of_S[] = { {}, {} };

template <>
S* cas_test_values<S*>[] = {
    &cas_test_values_of_S[0],
    &cas_test_values_of_S[1],
    nullptr,
};

template <>
const S* cas_test_values<const S*>[] = {
    &cas_test_values_of_S[0],
    &cas_test_values_of_S[1],
    nullptr,
};

template <>
volatile S* cas_test_values<volatile S*>[] = {
    &cas_test_values_of_S[0],
    &cas_test_values_of_S[1],
    nullptr,
};

template <>
const volatile S* cas_test_values<const volatile S*>[] = {
    &cas_test_values_of_S[0],
    &cas_test_values_of_S[1],
    nullptr,
};

template <>
function_pointer cas_test_values<function_pointer>[] = {
    &nothing_0,
    &nothing_1,
    nullptr,
};

template <typename T>
bool compare_exchange_test(bool &all_ok) {
    for (auto cas : cas_functions<T>) {
        for (const auto& success_order : orders) {
            for (const auto& failure_order : orders) {
                {
                    // Failure case
                    T actual = cas_test_values<T>[kActual];
                    fbl::atomic<T> atomic_value(actual);
                    T expected = cas_test_values<T>[kExpected];
                    T desired = cas_test_values<T>[kDesired];
                    EXPECT_FALSE(cas.function(&atomic_value, &expected, desired,
                                              success_order, failure_order),
                                 "compare-exchange shouldn't have succeeded!");
                    EXPECT_EQ(expected, actual, "compare-exchange didn't report actual value!");
                }
                {
                    // Success case
                    T actual = cas_test_values<T>[kActual];
                    fbl::atomic<T> atomic_value(actual);
                    T expected = actual;
                    T desired = cas_test_values<T>[kDesired];
                    // Some compare-and-swap functions can spuriously fail.
                    bool succeeded = cas.function(&atomic_value, &expected, desired,
                                                  success_order, failure_order);
                    if (!cas.can_spuriously_fail) {
                        EXPECT_TRUE(succeeded, "compare-exchange should've succeeded!");
                    }
                    EXPECT_EQ(expected, actual, "compare-exchange didn't report actual value!");
                }
            }
        }
    }

    for (auto cas : volatile_cas_functions<T>) {
        for (const auto& success_order : orders) {
            for (const auto& failure_order : orders) {
                {
                    // Failure case
                    T actual = cas_test_values<T>[kActual];
                    fbl::atomic<T> atomic_value(actual);
                    T expected = cas_test_values<T>[kExpected];
                    T desired = cas_test_values<T>[kDesired];
                    EXPECT_FALSE(cas.function(&atomic_value, &expected, desired,
                                              success_order, failure_order),
                                 "compare-exchange shouldn't have succeeded!");
                    EXPECT_EQ(expected, actual, "compare-exchange didn't report actual value!");
                }
                {
                    // Success case
                    T actual = cas_test_values<T>[kActual];
                    fbl::atomic<T> atomic_value(actual);
                    T expected = actual;
                    T desired = cas_test_values<T>[kDesired];
                    // Compare-and-swap can spuriously fail.
                    // Some compare-and-swap functions can spuriously fail.
                    bool succeeded = cas.function(&atomic_value, &expected, desired,
                                                  success_order, failure_order);
                    if (!cas.can_spuriously_fail) {
                        EXPECT_TRUE(succeeded, "compare-exchange should've succeeded!");
                    }
                    EXPECT_EQ(expected, actual, "compare-exchange didn't report actual value!");
                }
            }
        }
    }

    return true;
}

// Actual test cases on operations for each builtin type.
bool atomic_math_test() {
    BEGIN_TEST;

    ASSERT_TRUE(math_test<char>(all_ok));
    ASSERT_TRUE(math_test<signed char>(all_ok));
    ASSERT_TRUE(math_test<unsigned char>(all_ok));
    ASSERT_TRUE(math_test<short>(all_ok));
    ASSERT_TRUE(math_test<unsigned short>(all_ok));
    ASSERT_TRUE(math_test<int>(all_ok));
    ASSERT_TRUE(math_test<unsigned int>(all_ok));
    ASSERT_TRUE(math_test<long>(all_ok));
    ASSERT_TRUE(math_test<unsigned long>(all_ok));
    ASSERT_TRUE(math_test<long long>(all_ok));
    ASSERT_TRUE(math_test<unsigned long long>(all_ok));

    END_TEST;
}

bool atomic_pointer_math_test() {
    BEGIN_TEST;

    ASSERT_TRUE(pointer_add_test<int*>(all_ok));
    ASSERT_TRUE(pointer_add_test<const int*>(all_ok));
    ASSERT_TRUE(pointer_add_test<volatile int*>(all_ok));
    ASSERT_TRUE(pointer_add_test<const volatile int*>(all_ok));
    ASSERT_TRUE(pointer_add_test<S*>(all_ok));
    ASSERT_TRUE(pointer_add_test<const S*>(all_ok));
    ASSERT_TRUE(pointer_add_test<volatile S*>(all_ok));
    ASSERT_TRUE(pointer_add_test<const volatile S*>(all_ok));

    ASSERT_TRUE(pointer_sub_test<int*>(all_ok));
    ASSERT_TRUE(pointer_sub_test<const int*>(all_ok));
    ASSERT_TRUE(pointer_sub_test<volatile int*>(all_ok));
    ASSERT_TRUE(pointer_sub_test<const volatile int*>(all_ok));
    ASSERT_TRUE(pointer_sub_test<S*>(all_ok));
    ASSERT_TRUE(pointer_sub_test<const S*>(all_ok));
    ASSERT_TRUE(pointer_sub_test<volatile S*>(all_ok));
    ASSERT_TRUE(pointer_sub_test<const volatile S*>(all_ok));

    // Note that there is no void pointer or function pointer math,
    // and so no tests of them.

    END_TEST;
}

bool atomic_load_store_test() {
    BEGIN_TEST;

    ASSERT_TRUE(load_store_test<char>(all_ok));
    ASSERT_TRUE(load_store_test<signed char>(all_ok));
    ASSERT_TRUE(load_store_test<unsigned char>(all_ok));
    ASSERT_TRUE(load_store_test<short>(all_ok));
    ASSERT_TRUE(load_store_test<unsigned short>(all_ok));
    ASSERT_TRUE(load_store_test<int>(all_ok));
    ASSERT_TRUE(load_store_test<unsigned int>(all_ok));
    ASSERT_TRUE(load_store_test<long>(all_ok));
    ASSERT_TRUE(load_store_test<unsigned long>(all_ok));
    ASSERT_TRUE(load_store_test<long long>(all_ok));
    ASSERT_TRUE(load_store_test<unsigned long long>(all_ok));
    ASSERT_TRUE(load_store_test<bool>(all_ok));

    ASSERT_TRUE(load_store_test<void*>(all_ok));
    ASSERT_TRUE(load_store_test<const void*>(all_ok));
    ASSERT_TRUE(load_store_test<volatile void*>(all_ok));
    ASSERT_TRUE(load_store_test<const volatile void*>(all_ok));
    ASSERT_TRUE(load_store_test<int*>(all_ok));
    ASSERT_TRUE(load_store_test<const int*>(all_ok));
    ASSERT_TRUE(load_store_test<volatile int*>(all_ok));
    ASSERT_TRUE(load_store_test<const volatile int*>(all_ok));
    ASSERT_TRUE(load_store_test<function_pointer>(all_ok));

    END_TEST;
}

bool atomic_exchange_test() {
    BEGIN_TEST;

    ASSERT_TRUE(exchange_test<char>(all_ok));
    ASSERT_TRUE(exchange_test<signed char>(all_ok));
    ASSERT_TRUE(exchange_test<unsigned char>(all_ok));
    ASSERT_TRUE(exchange_test<short>(all_ok));
    ASSERT_TRUE(exchange_test<unsigned short>(all_ok));
    ASSERT_TRUE(exchange_test<int>(all_ok));
    ASSERT_TRUE(exchange_test<unsigned int>(all_ok));
    ASSERT_TRUE(exchange_test<long>(all_ok));
    ASSERT_TRUE(exchange_test<unsigned long>(all_ok));
    ASSERT_TRUE(exchange_test<long long>(all_ok));
    ASSERT_TRUE(exchange_test<unsigned long long>(all_ok));
    ASSERT_TRUE(exchange_test<bool>(all_ok));

    ASSERT_TRUE(exchange_test<void*>(all_ok));
    ASSERT_TRUE(exchange_test<const void*>(all_ok));
    ASSERT_TRUE(exchange_test<volatile void*>(all_ok));
    ASSERT_TRUE(exchange_test<const volatile void*>(all_ok));
    ASSERT_TRUE(exchange_test<int*>(all_ok));
    ASSERT_TRUE(exchange_test<const int*>(all_ok));
    ASSERT_TRUE(exchange_test<volatile int*>(all_ok));
    ASSERT_TRUE(exchange_test<const volatile int*>(all_ok));
    ASSERT_TRUE(exchange_test<S*>(all_ok));
    ASSERT_TRUE(exchange_test<const S*>(all_ok));
    ASSERT_TRUE(exchange_test<volatile S*>(all_ok));
    ASSERT_TRUE(exchange_test<const volatile S*>(all_ok));
    ASSERT_TRUE(exchange_test<function_pointer>(all_ok));

    END_TEST;
}

bool atomic_compare_exchange_test() {
    BEGIN_TEST;

    ASSERT_TRUE(compare_exchange_test<char>(all_ok));
    ASSERT_TRUE(compare_exchange_test<signed char>(all_ok));
    ASSERT_TRUE(compare_exchange_test<unsigned char>(all_ok));
    ASSERT_TRUE(compare_exchange_test<short>(all_ok));
    ASSERT_TRUE(compare_exchange_test<unsigned short>(all_ok));
    ASSERT_TRUE(compare_exchange_test<int>(all_ok));
    ASSERT_TRUE(compare_exchange_test<unsigned int>(all_ok));
    ASSERT_TRUE(compare_exchange_test<long>(all_ok));
    ASSERT_TRUE(compare_exchange_test<unsigned long>(all_ok));
    ASSERT_TRUE(compare_exchange_test<long long>(all_ok));
    ASSERT_TRUE(compare_exchange_test<unsigned long long>(all_ok));
    ASSERT_TRUE(compare_exchange_test<bool>(all_ok));

    ASSERT_TRUE(compare_exchange_test<void*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<const void*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<volatile void*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<const volatile void*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<int*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<const int*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<volatile int*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<const volatile int*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<S*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<const S*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<volatile S*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<const volatile S*>(all_ok));
    ASSERT_TRUE(compare_exchange_test<function_pointer>(all_ok));

    END_TEST;
}

// Code wants to rely on the ABI of fbl::atomic types. This means
// matching the underlying types' size and alignment, and the class
// being standard layout.

static_assert(sizeof(fbl::atomic<char>) == sizeof(char), "");
static_assert(sizeof(fbl::atomic<signed char>) == sizeof(signed char), "");
static_assert(sizeof(fbl::atomic<unsigned char>) == sizeof(unsigned char), "");
static_assert(sizeof(fbl::atomic<short>) == sizeof(short), "");
static_assert(sizeof(fbl::atomic<unsigned short>) == sizeof(unsigned short), "");
static_assert(sizeof(fbl::atomic<int>) == sizeof(int), "");
static_assert(sizeof(fbl::atomic<unsigned int>) == sizeof(unsigned int), "");
static_assert(sizeof(fbl::atomic<long>) == sizeof(long), "");
static_assert(sizeof(fbl::atomic<unsigned long>) == sizeof(unsigned long), "");
static_assert(sizeof(fbl::atomic<long long>) == sizeof(long long), "");
static_assert(sizeof(fbl::atomic<unsigned long long>) == sizeof(unsigned long long), "");
static_assert(sizeof(fbl::atomic<bool>) == sizeof(bool), "");
static_assert(sizeof(fbl::atomic<void*>) == sizeof(void*), "");
static_assert(sizeof(fbl::atomic<const void*>) == sizeof(const void*), "");
static_assert(sizeof(fbl::atomic<volatile void*>) == sizeof(volatile void*), "");
static_assert(sizeof(fbl::atomic<const volatile void*>) == sizeof(const volatile void*), "");
static_assert(sizeof(fbl::atomic<int*>) == sizeof(int*), "");
static_assert(sizeof(fbl::atomic<const int*>) == sizeof(const int*), "");
static_assert(sizeof(fbl::atomic<volatile int*>) == sizeof(volatile int*), "");
static_assert(sizeof(fbl::atomic<const volatile int*>) == sizeof(const volatile int*), "");
static_assert(sizeof(fbl::atomic<S*>) == sizeof(S*), "");
static_assert(sizeof(fbl::atomic<const S*>) == sizeof(const S*), "");
static_assert(sizeof(fbl::atomic<volatile S*>) == sizeof(volatile S*), "");
static_assert(sizeof(fbl::atomic<const volatile S*>) == sizeof(const volatile S*), "");
static_assert(sizeof(fbl::atomic<function_pointer>) == sizeof(function_pointer), "");

static_assert(alignof(fbl::atomic<char>) == alignof(char), "");
static_assert(alignof(fbl::atomic<signed char>) == alignof(signed char), "");
static_assert(alignof(fbl::atomic<unsigned char>) == alignof(unsigned char), "");
static_assert(alignof(fbl::atomic<short>) == alignof(short), "");
static_assert(alignof(fbl::atomic<unsigned short>) == alignof(unsigned short), "");
static_assert(alignof(fbl::atomic<int>) == alignof(int), "");
static_assert(alignof(fbl::atomic<unsigned int>) == alignof(unsigned int), "");
static_assert(alignof(fbl::atomic<long>) == alignof(long), "");
static_assert(alignof(fbl::atomic<unsigned long>) == alignof(unsigned long), "");
static_assert(alignof(fbl::atomic<long long>) == alignof(long long), "");
static_assert(alignof(fbl::atomic<unsigned long long>) == alignof(unsigned long long), "");
static_assert(alignof(fbl::atomic<bool>) == alignof(bool), "");
static_assert(alignof(fbl::atomic<void*>) == alignof(void*), "");
static_assert(alignof(fbl::atomic<const void*>) == alignof(const void*), "");
static_assert(alignof(fbl::atomic<volatile void*>) == alignof(volatile void*), "");
static_assert(alignof(fbl::atomic<const volatile void*>) == alignof(const volatile void*), "");
static_assert(alignof(fbl::atomic<int*>) == alignof(int*), "");
static_assert(alignof(fbl::atomic<const int*>) == alignof(const int*), "");
static_assert(alignof(fbl::atomic<volatile int*>) == alignof(volatile int*), "");
static_assert(alignof(fbl::atomic<const volatile int*>) == alignof(const volatile int*), "");
static_assert(alignof(fbl::atomic<S*>) == alignof(S*), "");
static_assert(alignof(fbl::atomic<const S*>) == alignof(const S*), "");
static_assert(alignof(fbl::atomic<volatile S*>) == alignof(volatile S*), "");
static_assert(alignof(fbl::atomic<const volatile S*>) == alignof(const volatile S*), "");
static_assert(alignof(fbl::atomic<function_pointer>) == alignof(function_pointer), "");

static_assert(fbl::is_standard_layout<fbl::atomic<char>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<signed char>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<unsigned char>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<short>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<unsigned short>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<int>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<unsigned int>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<long>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<unsigned long>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<long long>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<unsigned long long>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<bool>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<void*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<const void*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<volatile void*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<const volatile void*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<void*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<const int*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<volatile int*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<const volatile int*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<S*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<const S*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<volatile S*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<const volatile S*>>::value, "");
static_assert(fbl::is_standard_layout<fbl::atomic<function_pointer>>::value, "");

bool atomic_fence_test() {
    BEGIN_TEST;

    for (const auto& order : orders) {
        atomic_thread_fence(order);
        atomic_signal_fence(order);
    }

    END_TEST;
}

bool atomic_init_test() {
    BEGIN_TEST;

    fbl::atomic_uint32_t atomic1;
    fbl::atomic_init(&atomic1, 1u);
    EXPECT_EQ(1u, atomic1.load());

    fbl::atomic_uint32_t atomic2;
    volatile fbl::atomic_uint32_t* vatomic2 = &atomic2;
    fbl::atomic_init(vatomic2, 2u);
    EXPECT_EQ(2u, atomic2.load());

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(atomic_tests)
RUN_NAMED_TEST("Atomic explicit declarations test", atomic_explicit_declarations_test)
RUN_NAMED_TEST("Atomic using declarations test", atomic_using_declarations_test)
RUN_NAMED_TEST("Atomic won't compile test", atomic_wont_compile_test)
RUN_NAMED_TEST("Atomic math test", atomic_math_test)
RUN_NAMED_TEST("Atomic pointer math test", atomic_pointer_math_test)
RUN_NAMED_TEST("Atomic load/store test", atomic_load_store_test)
RUN_NAMED_TEST("Atomic exchange test", atomic_exchange_test)
RUN_NAMED_TEST("Atomic compare-exchange test", atomic_compare_exchange_test)
RUN_NAMED_TEST("Atomic fence test", atomic_fence_test)
RUN_NAMED_TEST("Atomic init test", atomic_init_test)
END_TEST_CASE(atomic_tests);
