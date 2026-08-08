//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <type_traits>. Almost everything here is
// compile-time: the static_asserts are the test, and they run on every
// compiler/arch combination in CI. The runtime test exists so the test case
// shows up in the unittest framework output.

#include <lib/unittest.h>
#include <type_traits>

namespace {

struct Empty {};
struct Virt {
    virtual ~Virt() {}
};
struct Agg {
    int a;
    char b;
};
struct MoveOnly {
    MoveOnly(MoveOnly &&) = default;
    MoveOnly &operator=(MoveOnly &&) = default;
    MoveOnly(const MoveOnly &) = delete;
    MoveOnly &operator=(const MoveOnly &) = delete;
};
enum class E : short { a };
union U {
    int a;
    float b;
};

using std::is_same_v;

// modification traits
static_assert(is_same_v<std::remove_cvref_t<const int &>, int>);
static_assert(is_same_v<std::remove_cv_t<const volatile int>, int>);
static_assert(is_same_v<std::add_const_t<int>, const int>);
static_assert(is_same_v<std::decay_t<int[4]>, int *>);
static_assert(is_same_v<std::decay_t<int(int)>, int (*)(int)>);
static_assert(is_same_v<std::decay_t<const int &>, int>);
static_assert(is_same_v<std::add_pointer_t<void>, void *>);
static_assert(is_same_v<std::add_pointer_t<int &>, int *>);
static_assert(is_same_v<std::add_lvalue_reference_t<void>, void>);
static_assert(is_same_v<std::add_lvalue_reference_t<int>, int &>);
static_assert(is_same_v<std::add_rvalue_reference_t<int>, int &&>);
static_assert(is_same_v<std::remove_all_extents_t<int[2][3]>, int>);
static_assert(is_same_v<std::remove_pointer_t<int *const>, int>);
static_assert(is_same_v<std::make_signed_t<unsigned long>, long>);
static_assert(is_same_v<std::make_signed_t<E>, short>);
static_assert(is_same_v<std::make_unsigned_t<const int>, const unsigned int>);
static_assert(is_same_v<std::underlying_type_t<E>, short>);
static_assert(is_same_v<std::common_type_t<int, long long>, long long>);
static_assert(is_same_v<std::common_type_t<char, short, int>, int>);
static_assert(is_same_v<std::conditional_t<true, int, long>, int>);
static_assert(is_same_v<std::conditional_t<false, int, long>, long>);

// primary and composite categories
static_assert(std::is_void_v<const void>);
static_assert(std::is_null_pointer_v<decltype(nullptr)>);
static_assert(std::is_array_v<int[3]> && std::is_array_v<int[]>);
static_assert(std::is_enum_v<E> && std::is_union_v<U> && std::is_class_v<Empty>);
static_assert(std::is_function_v<int(int)> && !std::is_function_v<int (*)(int)>);
static_assert(std::is_pointer_v<int *const> && !std::is_pointer_v<int Agg::*>);
static_assert(std::is_member_object_pointer_v<int Agg::*>);
static_assert(std::is_member_function_pointer_v<void (Virt::*)()>);
static_assert(!std::is_member_function_pointer_v<int Agg::*>);
static_assert(std::is_scalar_v<int *> && std::is_scalar_v<E>);
static_assert(std::is_fundamental_v<int> && !std::is_fundamental_v<Agg>);
static_assert(std::is_object_v<Agg> && !std::is_object_v<int &>);
static_assert(std::is_compound_v<Agg> && !std::is_compound_v<char>);
static_assert(std::is_arithmetic_v<char> && !std::is_arithmetic_v<E>);

// properties
static_assert(std::is_empty_v<Empty> && !std::is_empty_v<Agg>);
static_assert(std::is_polymorphic_v<Virt> && std::has_virtual_destructor_v<Virt>);
static_assert(std::is_aggregate_v<Agg>);
static_assert(std::is_trivially_copyable_v<Agg> && std::is_trivial_v<Agg>);
static_assert(std::is_standard_layout_v<Agg>);
static_assert(std::is_const_v<const int> && std::is_volatile_v<volatile int>);
static_assert(std::is_signed_v<int> && std::is_unsigned_v<unsigned>);
static_assert(std::is_signed_v<float> && !std::is_signed_v<E>);
static_assert(std::rank_v<int[2][3]> == 2);
static_assert(std::extent_v<int[2][3]> == 2);
static_assert(std::extent_v<int[2][3], 1> == 3);
static_assert(std::alignment_of_v<int> == alignof(int));

// construction/assignment/destruction
static_assert(std::is_constructible_v<Agg>);
static_assert(std::is_default_constructible_v<Empty>);
static_assert(!std::is_default_constructible_v<MoveOnly>);
static_assert(std::is_copy_constructible_v<Agg>);
static_assert(!std::is_copy_constructible_v<MoveOnly>);
static_assert(std::is_move_constructible_v<MoveOnly>);
static_assert(std::is_nothrow_move_constructible_v<Agg>);
static_assert(std::is_nothrow_default_constructible_v<int>);
static_assert(std::is_assignable_v<int &, int>);
static_assert(!std::is_assignable_v<int, int>);
static_assert(std::is_nothrow_copy_assignable_v<Agg>);
static_assert(std::is_move_assignable_v<MoveOnly> && !std::is_copy_assignable_v<MoveOnly>);
static_assert(std::is_destructible_v<Agg>);
static_assert(std::is_destructible_v<int &>);
static_assert(!std::is_destructible_v<void>);
static_assert(!std::is_destructible_v<int[]>);
static_assert(std::is_destructible_v<int[3]>);
static_assert(std::is_trivially_destructible_v<Agg>);
static_assert(!std::is_trivially_destructible_v<Virt>);

// relationships
struct Derived : Empty {};
struct Sealed final {};
static_assert(std::is_base_of_v<Empty, Derived>);
static_assert(std::is_final_v<Sealed> && !std::is_final_v<Empty>);
static_assert(std::is_convertible_v<int, long>);
static_assert(std::is_convertible_v<Derived *, Empty *>);
static_assert(!std::is_convertible_v<Empty *, Derived *>);
static_assert(std::is_convertible_v<void, void>);
static_assert(!std::is_convertible_v<int, void>);
static_assert(!std::is_convertible_v<MoveOnly, MoveOnly &>);

// logical operators
static_assert(std::conjunction_v<std::true_type, std::true_type>);
static_assert(!std::conjunction_v<std::true_type, std::false_type>);
static_assert(std::conjunction_v<>);
static_assert(std::disjunction_v<std::false_type, std::true_type>);
static_assert(!std::disjunction_v<>);
static_assert(std::negation_v<std::false_type>);

// integral_constant is callable/convertible
static_assert(std::true_type{});
static_assert(std::integral_constant<int, 3>{}() == 3);

// remaining alias/variable forms
static_assert(is_same_v<std::add_cv_t<int>, const volatile int>);
static_assert(is_same_v<std::add_volatile_t<int>, volatile int>);
static_assert(is_same_v<std::type_identity_t<int &>, int &>);
static_assert(is_same_v<std::void_t<int, Empty>, void>);
static_assert(is_same_v<std::common_type_t<int>, int>);
static_assert(is_same_v<std::enable_if_t<true, char>, char>);
static_assert(std::is_nothrow_assignable_v<int &, int>);
static_assert(std::is_trivially_assignable_v<int &, int>);
static_assert(std::is_trivially_constructible_v<Agg, const Agg &>);
static_assert(std::extent_v<int[7]> == 7, "default dimension parameter");
static_assert(std::is_unsigned_v<std::make_unsigned_t<wchar_t>>);
static_assert(sizeof(std::make_unsigned_t<wchar_t>) == sizeof(wchar_t));
static_assert(std::is_signed_v<std::make_signed_t<char16_t>>);
static_assert(sizeof(std::make_signed_t<char16_t>) == sizeof(char16_t));

// the has-member-fn detection macros
struct WithBar {
    void Bar(int) {}
};
struct WithoutBar {};
DECLARE_HAS_MEMBER_FN(has_bar, Bar);
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_bar_sig, Bar, void (C::*)(int));
static_assert(has_bar<WithBar>::value);
static_assert(!has_bar<WithoutBar>::value);
static_assert(has_bar_sig<WithBar>::value);
static_assert(!has_bar_sig<WithoutBar>::value);

// non-standard extensions preserved
static_assert(std::is_signed_integer<int>::value);
static_assert(!std::is_signed_integer<float>::value);
static_assert(!std::is_unsigned_integer<int>::value);
static_assert(is_same_v<std::match_cv<const int, long>::type, const long>);
static_assert(std::is_convertible_pointer<Derived *, const Empty *>::value);
static_assert(std::is_pod<Agg>::value);

bool type_traits_smoke() {
    BEGIN_TEST;

    // A token runtime check so the case registers and reports.
    EXPECT_TRUE((std::is_same_v<std::decay_t<decltype("x")>, const char *>), "");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_type_traits_tests)
RUN_TEST(type_traits_smoke)
END_TEST_CASE(libcpp_type_traits_tests)

} // namespace
