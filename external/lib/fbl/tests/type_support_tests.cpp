// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/type_support.h>

struct s {};
union u {};

using fp = void();

// is_void tests:
static_assert(fbl::is_void<void>::value, "");
static_assert(fbl::is_void<const void>::value, "");
static_assert(fbl::is_void<volatile void>::value, "");
static_assert(fbl::is_void<const volatile void>::value, "");

static_assert(!fbl::is_void<int>::value, "");
static_assert(!fbl::is_void<void*>::value, "");

// is_null_pointer tests:
static_assert(fbl::is_null_pointer<decltype(nullptr)>::value, "");
static_assert(fbl::is_null_pointer<const decltype(nullptr)>::value, "");
static_assert(fbl::is_null_pointer<volatile decltype(nullptr)>::value, "");
static_assert(fbl::is_null_pointer<const volatile decltype(nullptr)>::value, "");

static_assert(!fbl::is_null_pointer<int>::value, "");
static_assert(!fbl::is_null_pointer<int*>::value, "");
static_assert(!fbl::is_null_pointer<s*>::value, "");
static_assert(!fbl::is_null_pointer<fp>::value, "");

// is_pointer tests:
static_assert(fbl::is_pointer<int*>::value, "");
static_assert(fbl::is_pointer<float*>::value, "");
static_assert(fbl::is_pointer<s*>::value, "");
static_assert(fbl::is_pointer<u*>::value, "");
static_assert(fbl::is_pointer<s*>::value, "");
static_assert(fbl::is_pointer<fp*>::value, "");

static_assert(fbl::is_pointer<const int*>::value, "");
static_assert(fbl::is_pointer<const float*>::value, "");
static_assert(fbl::is_pointer<const s*>::value, "");
static_assert(fbl::is_pointer<const u*>::value, "");
static_assert(fbl::is_pointer<const s*>::value, "");

static_assert(fbl::is_pointer<volatile int*>::value, "");
static_assert(fbl::is_pointer<volatile float*>::value, "");
static_assert(fbl::is_pointer<volatile s*>::value, "");
static_assert(fbl::is_pointer<volatile u*>::value, "");
static_assert(fbl::is_pointer<volatile s*>::value, "");

static_assert(fbl::is_pointer<const volatile int*>::value, "");
static_assert(fbl::is_pointer<const volatile float*>::value, "");
static_assert(fbl::is_pointer<const volatile s*>::value, "");
static_assert(fbl::is_pointer<const volatile u*>::value, "");
static_assert(fbl::is_pointer<const volatile s*>::value, "");

static_assert(!fbl::is_pointer<int>::value, "");
static_assert(!fbl::is_pointer<float>::value, "");
static_assert(!fbl::is_pointer<s>::value, "");
static_assert(!fbl::is_pointer<u>::value, "");
static_assert(!fbl::is_pointer<s>::value, "");
static_assert(!fbl::is_pointer<fp>::value, "");

static_assert(!fbl::is_pointer<const int>::value, "");
static_assert(!fbl::is_pointer<const float>::value, "");
static_assert(!fbl::is_pointer<const s>::value, "");
static_assert(!fbl::is_pointer<const u>::value, "");
static_assert(!fbl::is_pointer<const s>::value, "");

static_assert(!fbl::is_pointer<volatile int>::value, "");
static_assert(!fbl::is_pointer<volatile float>::value, "");
static_assert(!fbl::is_pointer<volatile s>::value, "");
static_assert(!fbl::is_pointer<volatile u>::value, "");
static_assert(!fbl::is_pointer<volatile s>::value, "");

static_assert(!fbl::is_pointer<const volatile int>::value, "");
static_assert(!fbl::is_pointer<const volatile float>::value, "");
static_assert(!fbl::is_pointer<const volatile s>::value, "");
static_assert(!fbl::is_pointer<const volatile u>::value, "");
static_assert(!fbl::is_pointer<const volatile s>::value, "");

// is_reference tests:
static_assert(fbl::is_reference<int&>::value, "");
static_assert(fbl::is_reference<int&&>::value, "");
static_assert(fbl::is_reference<float&>::value, "");
static_assert(fbl::is_reference<float&&>::value, "");
static_assert(fbl::is_reference<s&>::value, "");
static_assert(fbl::is_reference<s&&>::value, "");
static_assert(fbl::is_reference<u&>::value, "");
static_assert(fbl::is_reference<u&&>::value, "");
static_assert(fbl::is_reference<s&>::value, "");
static_assert(fbl::is_reference<s&&>::value, "");
static_assert(fbl::is_reference<fp&>::value, "");
static_assert(fbl::is_reference<fp&&>::value, "");

static_assert(fbl::is_reference<const int&>::value, "");
static_assert(fbl::is_reference<const int&&>::value, "");
static_assert(fbl::is_reference<const float&>::value, "");
static_assert(fbl::is_reference<const float&&>::value, "");
static_assert(fbl::is_reference<const s&>::value, "");
static_assert(fbl::is_reference<const s&&>::value, "");
static_assert(fbl::is_reference<const u&>::value, "");
static_assert(fbl::is_reference<const u&&>::value, "");
static_assert(fbl::is_reference<const s&>::value, "");
static_assert(fbl::is_reference<const s&&>::value, "");

static_assert(fbl::is_reference<volatile int&>::value, "");
static_assert(fbl::is_reference<volatile int&&>::value, "");
static_assert(fbl::is_reference<volatile float&>::value, "");
static_assert(fbl::is_reference<volatile float&&>::value, "");
static_assert(fbl::is_reference<volatile s&>::value, "");
static_assert(fbl::is_reference<volatile s&&>::value, "");
static_assert(fbl::is_reference<volatile u&>::value, "");
static_assert(fbl::is_reference<volatile u&&>::value, "");
static_assert(fbl::is_reference<volatile s&>::value, "");
static_assert(fbl::is_reference<volatile s&&>::value, "");

static_assert(fbl::is_reference<const volatile int&>::value, "");
static_assert(fbl::is_reference<const volatile int&&>::value, "");
static_assert(fbl::is_reference<const volatile float&>::value, "");
static_assert(fbl::is_reference<const volatile float&&>::value, "");
static_assert(fbl::is_reference<const volatile s&>::value, "");
static_assert(fbl::is_reference<const volatile s&&>::value, "");
static_assert(fbl::is_reference<const volatile u&>::value, "");
static_assert(fbl::is_reference<const volatile u&&>::value, "");
static_assert(fbl::is_reference<const volatile s&>::value, "");
static_assert(fbl::is_reference<const volatile s&&>::value, "");

static_assert(!fbl::is_reference<int>::value, "");
static_assert(!fbl::is_reference<float>::value, "");
static_assert(!fbl::is_reference<s>::value, "");
static_assert(!fbl::is_reference<u>::value, "");
static_assert(!fbl::is_reference<s>::value, "");
static_assert(!fbl::is_reference<fp>::value, "");

static_assert(!fbl::is_reference<const int>::value, "");
static_assert(!fbl::is_reference<const float>::value, "");
static_assert(!fbl::is_reference<const s>::value, "");
static_assert(!fbl::is_reference<const u>::value, "");
static_assert(!fbl::is_reference<const s>::value, "");

static_assert(!fbl::is_reference<volatile int>::value, "");
static_assert(!fbl::is_reference<volatile float>::value, "");
static_assert(!fbl::is_reference<volatile s>::value, "");
static_assert(!fbl::is_reference<volatile u>::value, "");
static_assert(!fbl::is_reference<volatile s>::value, "");

static_assert(!fbl::is_reference<const volatile int>::value, "");
static_assert(!fbl::is_reference<const volatile float>::value, "");
static_assert(!fbl::is_reference<const volatile s>::value, "");
static_assert(!fbl::is_reference<const volatile u>::value, "");
static_assert(!fbl::is_reference<const volatile s>::value, "");

// is_union tests:
static_assert(fbl::is_union<u>::value, "");

static_assert(!fbl::is_union<u*>::value, "");
static_assert(!fbl::is_union<u&>::value, "");
static_assert(!fbl::is_union<u&&>::value, "");

static_assert(!fbl::is_union<int>::value, "");
static_assert(!fbl::is_union<float>::value, "");
static_assert(!fbl::is_union<s>::value, "");
static_assert(!fbl::is_union<s>::value, "");
static_assert(!fbl::is_union<fp>::value, "");

// is_const tests:
static_assert(!fbl::is_const<int>::value, "");
static_assert(fbl::is_const<const int>::value, "");
static_assert(!fbl::is_const<s>::value, "");
static_assert(fbl::is_const<const s>::value, "");
static_assert(!fbl::is_const<u>::value, "");
static_assert(fbl::is_const<const u>::value, "");

static_assert(!fbl::is_const<int*>::value, "");
static_assert(fbl::is_const<int* const>::value, "");
static_assert(!fbl::is_const<const int*>::value, "");
static_assert(fbl::is_const<const int* const>::value, "");

// underlying_type:

enum int_enum : int {};
enum char_enum : char {};
static_assert(fbl::is_same<fbl::underlying_type<int_enum>::type, int>::value, "expected int");
static_assert(fbl::is_same<fbl::underlying_type<char_enum>::type, char>::value, "expected char");

// match_cv tests:
static_assert(fbl::is_same<fbl::match_cv<int, void>::type, void>::value, "wrong type");
static_assert(fbl::is_same<fbl::match_cv<const int, void>::type, const void>::value,
              "wrong type");
static_assert(fbl::is_same<fbl::match_cv<volatile void, char>::type, volatile char>::value,
              "wrong type");
static_assert(fbl::is_same<fbl::match_cv<const int, const char>::type, const char>::value,
              "wrong type");
static_assert(fbl::is_same<fbl::match_cv<const int, volatile char>::type, const char>::value,
              "wrong type");
static_assert(fbl::is_same<fbl::match_cv<char, const volatile void>::type, void>::value,
              "wrong type");


// is_class tests
namespace is_class_tests {
static_assert(!fbl::is_class<int>::value, "'int' should not pass the is_class<> test!");

class A { };
static_assert(fbl::is_class<A>::value, "'class' should pass the is_class<> test!");

struct B { };
static_assert(fbl::is_class<B>::value, "'struct' should pass the is_class<> test!");

union C { int a; float b; };
static_assert(!fbl::is_class<C>::value, "'union' should not pass the is_class<> test!");

enum D { D_ENUM_VALUE };
static_assert(!fbl::is_class<D>::value, "'enum' should not pass the is_class<> test!");

enum class E { VALUE };
static_assert(!fbl::is_class<E>::value, "'enum class' should not pass the is_class<> test!");
}  // namespace is_class_tests

// is_base_of tests
namespace is_base_of_tests {

static_assert(!fbl::is_base_of<int, int>::value,
              "scalar types should not be bases of scalar types");

class A { };
static_assert(fbl::is_base_of<A, A>::value, "A should be a base of A!");

class B : public A { };
static_assert( fbl::is_base_of<B, B>::value, "B should be a base of B!");
static_assert( fbl::is_base_of<A, B>::value, "A should be a base of B!");
static_assert(!fbl::is_base_of<B, A>::value, "B should not be a base of A!");

class C : public B { };
static_assert( fbl::is_base_of<C, C>::value, "C should be a base of C!");
static_assert( fbl::is_base_of<B, C>::value, "B should be a base of C!");
static_assert( fbl::is_base_of<A, C>::value, "A should be a base of C!");
static_assert(!fbl::is_base_of<C, B>::value, "C should not be a base of B!");
static_assert(!fbl::is_base_of<C, A>::value, "C should not be a base of A!");

class D { };
class E : public B, public D { };
static_assert( fbl::is_base_of<D, D>::value, "D should be a base of D!");
static_assert( fbl::is_base_of<E, E>::value, "E should be a base of E!");
static_assert( fbl::is_base_of<A, E>::value, "A should be a base of E!");
static_assert( fbl::is_base_of<B, E>::value, "B should be a base of E!");
static_assert(!fbl::is_base_of<C, E>::value, "C should not be a base of E!");
static_assert( fbl::is_base_of<D, E>::value, "D should be a base of E!");
static_assert(!fbl::is_base_of<E, A>::value, "E should not be a base of A!");
static_assert(!fbl::is_base_of<E, B>::value, "E should not be a base of B!");
static_assert(!fbl::is_base_of<E, C>::value, "E should not be a base of C!");
static_assert(!fbl::is_base_of<E, D>::value, "E should not be a base of D!");

struct sA { };
static_assert(fbl::is_base_of<sA, sA>::value, "sA should be a base of sA!");

struct sB : public sA { };
static_assert( fbl::is_base_of<sB, sB>::value, "sB should be a base of sB!");
static_assert( fbl::is_base_of<sA, sB>::value, "sA should be a base of sB!");
static_assert(!fbl::is_base_of<sB, sA>::value, "sB should not be a base of sA!");

struct sC : public sB { };
static_assert( fbl::is_base_of<sC, sC>::value, "sC should be a base of sC!");
static_assert( fbl::is_base_of<sB, sC>::value, "sB should be a base of sC!");
static_assert( fbl::is_base_of<sA, sC>::value, "sA should be a base of sC!");
static_assert(!fbl::is_base_of<sC, sB>::value, "sC should not be a base of sB!");
static_assert(!fbl::is_base_of<sC, sA>::value, "sC should not be a base of sA!");

struct sD { };
struct sE : public sB, public sD { };
static_assert( fbl::is_base_of<sD, sD>::value, "sD should be a base of sD!");
static_assert( fbl::is_base_of<sE, sE>::value, "sE should be a base of sE!");
static_assert( fbl::is_base_of<sA, sE>::value, "sA should be a base of sE!");
static_assert( fbl::is_base_of<sB, sE>::value, "sB should be a base of sE!");
static_assert(!fbl::is_base_of<sC, sE>::value, "sC should not be a base of sE!");
static_assert( fbl::is_base_of<sD, sE>::value, "sD should be a base of sE!");
static_assert(!fbl::is_base_of<sE, sA>::value, "sE should not be a base of sA!");
static_assert(!fbl::is_base_of<sE, sB>::value, "sE should not be a base of sB!");
static_assert(!fbl::is_base_of<sE, sC>::value, "sE should not be a base of sC!");
static_assert(!fbl::is_base_of<sE, sD>::value, "sE should not be a base of sD!");

}  // namespace is_base_of_tests

namespace has_virtual_destructor_tests {

struct A            {         ~A() { } };
struct B            { virtual ~B() { } };
struct C : public A {         ~C() { } };
struct D : public B {         ~D() { } };
struct E : public A { virtual ~E() { } };

static_assert(!fbl::has_virtual_destructor<A>::value, "A should have no virtual destructor");
static_assert( fbl::has_virtual_destructor<B>::value, "B should have a virtual destructor");
static_assert(!fbl::has_virtual_destructor<C>::value, "C should have no virtual destructor");
static_assert( fbl::has_virtual_destructor<D>::value, "D should have a virtual destructor");
static_assert( fbl::has_virtual_destructor<E>::value, "E should have a virtual destructor");

}  // namespace has_virtual_destructor_tests

namespace is_floating_point {

static_assert(fbl::is_floating_point<float>::value, "float is floating point");
static_assert(fbl::is_floating_point<double>::value, "double is floating point");
static_assert(fbl::is_floating_point<long double>::value, "long double is floating point");

static_assert(!fbl::is_floating_point<int>::value, "int is not floating point");
static_assert(!fbl::is_floating_point<decltype(nullptr)>::value, "nullptr is not floating point");

struct A {};
static_assert(!fbl::is_floating_point<A>::value, "A is not floating point");
static_assert(!fbl::is_floating_point<A*>::value, "A pointer is not floating point");

} // namespace is_floating_point

namespace is_integral_tests {

static_assert(fbl::is_integral<bool>::value, "bool is integral");
static_assert(fbl::is_integral<char>::value, "char is integral");
static_assert(fbl::is_integral<char16_t>::value, "char16_t is integral");
static_assert(fbl::is_integral<char32_t>::value, "char32_t is integral");
static_assert(fbl::is_integral<wchar_t>::value, "wchar_t is integral");
static_assert(fbl::is_integral<signed char>::value, "signed char is integral");
static_assert(fbl::is_integral<unsigned char>::value, "unsigned char is integral");
static_assert(fbl::is_integral<short int>::value, "short int is integral");
static_assert(fbl::is_integral<unsigned short int>::value, "unsigned short int is integral");
static_assert(fbl::is_integral<int>::value, "int is integral");
static_assert(fbl::is_integral<unsigned int>::value, "unsigned int is integral");
static_assert(fbl::is_integral<long int>::value, "long int is integral");
static_assert(fbl::is_integral<unsigned long int>::value, "unsigned long int is integral");
static_assert(fbl::is_integral<long long int>::value, "long long int is integral");
static_assert(fbl::is_integral<unsigned long long int>::value, "unsigned long long int is integral");

static_assert(!fbl::is_integral<float>::value, "float is not integral");
static_assert(!fbl::is_integral<decltype(nullptr)>::value, "nullptr is not integral");

struct A {};
static_assert(!fbl::is_integral<A>::value, "A is not floating point");
static_assert(!fbl::is_integral<A*>::value, "A pointer is not integral");

} // namespace is_integral_tests

namespace is_arithmetic_tests {

static_assert(fbl::is_arithmetic<bool>::value, "bool is arithmetic");
static_assert(fbl::is_arithmetic<char>::value, "char is arithmetic");
static_assert(fbl::is_arithmetic<char16_t>::value, "char16_t is arithmetic");
static_assert(fbl::is_arithmetic<char32_t>::value, "char32_t is arithmetic");
static_assert(fbl::is_arithmetic<wchar_t>::value, "wchar_t is arithmetic");
static_assert(fbl::is_arithmetic<signed char>::value, "signed char is arithmetic");
static_assert(fbl::is_arithmetic<unsigned char>::value, "unsigned char is arithmetic");
static_assert(fbl::is_arithmetic<short int>::value, "short int is arithmetic");
static_assert(fbl::is_arithmetic<unsigned short int>::value, "unsigned short int is arithmetic");
static_assert(fbl::is_arithmetic<int>::value, "int is arithmetic");
static_assert(fbl::is_arithmetic<unsigned int>::value, "unsigned int is arithmetic");
static_assert(fbl::is_arithmetic<long int>::value, "long int is arithmetic");
static_assert(fbl::is_arithmetic<unsigned long int>::value, "unsigned long int is arithmetic");
static_assert(fbl::is_arithmetic<long long int>::value, "long long int is arithmetic");
static_assert(fbl::is_arithmetic<unsigned long long int>::value, "unsigned long long int is arithmetic");

static_assert(fbl::is_arithmetic<float>::value, "float is arithmetic");
static_assert(fbl::is_arithmetic<double>::value, "double is arithmetic");
static_assert(fbl::is_arithmetic<long double>::value, "long double is arithmetic");

static_assert(!fbl::is_arithmetic<decltype(nullptr)>::value, "nullptr is not arithmetic");

struct A {};
static_assert(!fbl::is_arithmetic<A>::value, "A is not arithmetic");
static_assert(!fbl::is_arithmetic<A*>::value, "A pointer is not arithmetic");

} // namespace is_arithmetic_tests

namespace is_signed_tests {

static_assert(fbl::is_signed<int>::value, "int is signed");
static_assert(!fbl::is_signed<unsigned int>::value, "unsigned int is not signed");
static_assert(fbl::is_signed<float>::value, "float is signed");

static_assert(!fbl::is_signed<decltype(nullptr)>::value, "nullptr is not signed");

struct A {};
static_assert(!fbl::is_signed<A>::value, "A is not signed");
static_assert(!fbl::is_signed<A*>::value, "A pointer is not signed");

} // namespace is_signed_tests

namespace is_unsigned_tests {

static_assert(!fbl::is_unsigned<int>::value, "int is not unsigned");
static_assert(fbl::is_unsigned<unsigned int>::value, "unsigned int is unsigned");
static_assert(!fbl::is_unsigned<float>::value, "float is not unsigned");

static_assert(!fbl::is_unsigned<decltype(nullptr)>::value, "nullptr is not unsigned");

struct A {};
static_assert(!fbl::is_unsigned<A>::value, "A is not unsigned");
static_assert(!fbl::is_unsigned<A*>::value, "A pointer is not unsigned");

} // namespace is_unsigned_tests

namespace is_signed_integer_tests {

static_assert(fbl::is_signed_integer<int>::value, "int is signed integer");
static_assert(!fbl::is_signed_integer<unsigned int>::value, "unsigned int is not signed integer");
static_assert(!fbl::is_signed_integer<float>::value, "float is not signed integer");

static_assert(!fbl::is_signed_integer<decltype(nullptr)>::value, "nullptr is not signed integer");

struct A {};
static_assert(!fbl::is_signed_integer<A>::value, "A is not signed integer");
static_assert(!fbl::is_signed_integer<A*>::value, "A pointer is not signed integer");

} // namespace is_signed_integer_tests

namespace is_unsigned_integer_tests {

static_assert(!fbl::is_unsigned_integer<int>::value, "int is not unsigned integer");
static_assert(fbl::is_unsigned_integer<unsigned int>::value, "unsigned int is unsigned integer");
static_assert(!fbl::is_unsigned_integer<float>::value, "float is not unsigned integer");

static_assert(!fbl::is_unsigned_integer<decltype(nullptr)>::value, "nullptr is not unsigned integer");

struct A {};
static_assert(!fbl::is_unsigned_integer<A>::value, "A is not unsigned integer");
static_assert(!fbl::is_unsigned_integer<A*>::value, "A pointer is not unsigned integer");

} // namespace is_unsigned_integer_tests

namespace is_enum_tests {

enum E {};
struct S {};
static_assert(fbl::is_enum<E>::value, "enum is an enum");
static_assert(!fbl::is_enum<S>::value, "struct is not an enum");
static_assert(!fbl::is_enum<int>::value, "int is not an enum");
static_assert(!fbl::is_enum<void>::value, "void is not an enum");

} // namespace is_enum_tests

namespace is_pointer_tests {

struct StructType   {
    void MemberFunction();
    int member_variable;

    static void StaticMemberFunction();
    static int static_member_variable;
};

static void SomeStaticFunc() { }
void SomeGlobalFunc() {
    // Force an artifical use of SomeStaticFunc to avoid warnings.
    SomeStaticFunc();
}

class  ClassType    { };
enum   EnumType     { One, Two };
union  UnionType    { int a; double b; };

static_assert(!fbl::is_pointer<StructType>::value,         "StructType is not a pointer!");
static_assert( fbl::is_pointer<StructType*>::value,        "StructType* is a pointer!");
static_assert( fbl::is_pointer<StructType**>::value,       "StructType** is a pointer!");
static_assert(!fbl::is_pointer<ClassType>::value,          "ClassType is not a pointer!");
static_assert( fbl::is_pointer<ClassType*>::value,         "ClassType* is a pointer!");
static_assert( fbl::is_pointer<ClassType**>::value,        "ClassType** is a pointer!");
static_assert(!fbl::is_pointer<EnumType>::value,           "EnumType is not a pointer!");
static_assert( fbl::is_pointer<EnumType*>::value,          "EnumType* is a pointer!");
static_assert( fbl::is_pointer<EnumType**>::value,         "EnumType** is a pointer!");
static_assert(!fbl::is_pointer<UnionType>::value,          "UnionType is not a pointer!");
static_assert( fbl::is_pointer<UnionType*>::value,         "UnionType* is a pointer!");
static_assert( fbl::is_pointer<UnionType**>::value,        "UnionType** is a pointer!");
static_assert(!fbl::is_pointer<int>::value,                "int is not a pointer!");
static_assert(!fbl::is_pointer<int[]>::value,              "int[] is not a pointer!");
static_assert( fbl::is_pointer<int*>::value,               "int* is a pointer!");
static_assert( fbl::is_pointer<int**>::value,              "int** is a pointer!");

static_assert(!fbl::is_pointer<const int>::value,          "const int is not a pointer!");
static_assert(!fbl::is_pointer<volatile int>::value,       "volatile int is not a pointer!");
static_assert(!fbl::is_pointer<const volatile int>::value, "const volatile int is not a pointer!");

static_assert( fbl::is_pointer<const int*>::value,          "const int* is a pointer!");
static_assert( fbl::is_pointer<volatile int*>::value,       "volatile int* is a pointer!");
static_assert( fbl::is_pointer<const volatile int*>::value, "const volatile int* is a pointer!");

static_assert( fbl::is_pointer<int* const >::value,          "int* const is a pointer!");
static_assert( fbl::is_pointer<int* volatile >::value,       "int* volatile is a pointer!");
static_assert( fbl::is_pointer<int* const volatile >::value, "int* const volatile is a pointer!");

static_assert( fbl::is_pointer<const int* const >::value,    "const int* const is a pointer!");
static_assert( fbl::is_pointer<const int* volatile >::value, "const int* volatile is a pointer!");
static_assert( fbl::is_pointer<const int* const volatile>::value,
        "const int* const volatile is a pointer!");

static_assert( fbl::is_pointer<volatile int* const >::value, "volatile int* const is a pointer!");
static_assert( fbl::is_pointer<volatile int* volatile >::value,
        "volatile int* volatile is a pointer!");
static_assert( fbl::is_pointer<volatile int* const volatile>::value,
        "volatile int* const volatile is a pointer!");

static_assert( fbl::is_pointer<const volatile int* const >::value,
        "const volatile int* const is a pointer!");
static_assert( fbl::is_pointer<const volatile int* volatile >::value,
        "const volatile int* volatile is a pointer!");
static_assert( fbl::is_pointer<const volatile int* const volatile>::value,
        "const volatile int* const volatile is a pointer!");

static_assert(!fbl::is_pointer<decltype(&StructType::MemberFunction)>::value,
              "pointer to StructType::MemberFunction is not a pointer!");
static_assert(!fbl::is_pointer<decltype(&StructType::member_variable)>::value,
              "pointer to StructType::member_variable is not a pointer!");

static_assert( fbl::is_pointer<decltype(&StructType::StaticMemberFunction)>::value,
              "pointer to StructType::MemberFunction is a pointer!");
static_assert( fbl::is_pointer<decltype(&StructType::static_member_variable)>::value,
              "pointer to StructType::static_member_variable is a pointer!");

static_assert( fbl::is_pointer<decltype(&SomeGlobalFunc)>::value,
              "pointer to SomeGlobalFunc is a pointer!");
static_assert( fbl::is_pointer<decltype(&SomeStaticFunc)>::value,
              "pointer to SomeStaticFunc is a pointer!");
static_assert(!fbl::is_pointer<decltype(nullptr)>::value,
              "decltype(nullptr) (aka nullptr_t) is not a pointer (because C++)");
}  // namespace is_pointer_tests;

namespace is_convertible_tests {

class A { };
class B : public A { };
class C { };

template <typename From, typename To>
using icp = fbl::is_convertible_pointer<From, To>;

static_assert( icp<B*, A*>::value, "Should convert B* --> A*");
static_assert(!icp<A*, B*>::value, "Should not convert A* --> B*");
static_assert(!icp<A,  B*>::value, "Should not convert A --> B*");
static_assert(!icp<A*, B>::value,  "Should not convert A* --> B");
static_assert(!icp<A,  B>::value,  "Should not convert A --> B");
static_assert(!icp<A*, C*>::value, "Should not convert A* --> C*");

static_assert( icp<int*, void*>::value,         "Should convert int* --> void*");
static_assert( icp<int*, const int*>::value,    "Should convert int* --> const int*");
static_assert( icp<int*, volatile int*>::value, "Should convert int* --> volatile int*");
static_assert(!icp<const int*, int*>::value,    "Should not convert const int* --> int*");
static_assert(!icp<volatile int*, int*>::value, "Should not convert volatile int* --> int*");
static_assert(!icp<unsigned int*, int*>::value, "Should not convert unsigned int* --> int*");
static_assert(!icp<int*, unsigned int*>::value, "Should not convert int* --> unsigned int*");
static_assert(!icp<float*, double*>::value,     "Should not convert float* --> double*");

}  // namespace is_convertible_tests

namespace conditional_tests {

static_assert(fbl::is_same<fbl::conditional<true, int, bool>::type, int>::value, "wrong type");
static_assert(fbl::is_same<fbl::conditional<false, int, bool>::type, bool>::value, "wrong type");

}  // namespace conditional_tests

namespace has_member_fn_tests {

struct A {
  void simple();
  void overloaded();
  int overloaded(int);
  int overloaded(int) const;
};

struct B {
  int simple();
  int overloaded(int) const;
};

struct C {};

DECLARE_HAS_MEMBER_FN(has_simple, simple);
DECLARE_HAS_MEMBER_FN(has_overloaded, overloaded);
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_simple_void, simple, void (C::*)());
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_simple_int, simple, int (C::*)());
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_overloaded_void, overloaded, void (C::*)());
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_overloaded_int, overloaded, int (C::*)(int));
DECLARE_HAS_MEMBER_FN_WITH_SIGNATURE(has_overloaded_int_const, overloaded, int (C::*)(int) const);

static_assert(has_simple<A>::value, "");
static_assert(has_simple<B>::value, "");
static_assert(!has_simple<C>::value, "");
static_assert(!has_simple<int>::value, "");
static_assert(!has_simple<decltype(nullptr)>::value, "");

static_assert(!has_overloaded<A>::value, "fails due to ambiguity");
static_assert(has_overloaded<B>::value, "unambiguous");
static_assert(!has_overloaded<C>::value, "");
static_assert(!has_overloaded<int>::value, "");
static_assert(!has_overloaded<decltype(nullptr)>::value, "");

static_assert(has_simple_void<A>::value, "");
static_assert(!has_simple_void<B>::value, "");
static_assert(!has_simple_void<C>::value, "");
static_assert(!has_simple_void<int>::value, "");
static_assert(!has_simple_void<decltype(nullptr)>::value, "");

static_assert(!has_simple_int<A>::value, "");
static_assert(has_simple_int<B>::value, "");
static_assert(!has_simple_int<C>::value, "");
static_assert(!has_simple_int<int>::value, "");
static_assert(!has_simple_int<decltype(nullptr)>::value, "");

static_assert(has_overloaded_void<A>::value, "");
static_assert(!has_overloaded_void<B>::value, "");
static_assert(!has_overloaded_void<C>::value, "");
static_assert(!has_overloaded_void<int>::value, "");
static_assert(!has_overloaded_void<decltype(nullptr)>::value, "");

static_assert(has_overloaded_int<A>::value, "");
static_assert(!has_overloaded_int<B>::value, "");
static_assert(!has_overloaded_int<C>::value, "");
static_assert(!has_overloaded_int<int>::value, "");
static_assert(!has_overloaded_int<decltype(nullptr)>::value, "");

static_assert(has_overloaded_int_const<A>::value, "");
static_assert(has_overloaded_int_const<B>::value, "");
static_assert(!has_overloaded_int_const<C>::value, "");
static_assert(!has_overloaded_int_const<int>::value, "");
static_assert(!has_overloaded_int_const<decltype(nullptr)>::value, "");

} // namespace has_member_fn_tests

// is_function tests:

namespace is_function_tests {

struct s {};
union u {};

struct forward_s;

// Lots of things are not functions.

static_assert(!fbl::is_function<int>::value, "");
static_assert(!fbl::is_function<int[]>::value, "");
static_assert(!fbl::is_function<int[1]>::value, "");
static_assert(!fbl::is_function<int*>::value, "");
static_assert(!fbl::is_function<int&>::value, "");
static_assert(!fbl::is_function<int&&>::value, "");

static_assert(!fbl::is_function<float>::value, "");
static_assert(!fbl::is_function<float[]>::value, "");
static_assert(!fbl::is_function<float[1]>::value, "");
static_assert(!fbl::is_function<float*>::value, "");
static_assert(!fbl::is_function<float&>::value, "");
static_assert(!fbl::is_function<float&&>::value, "");

static_assert(!fbl::is_function<s>::value, "");
static_assert(!fbl::is_function<s[]>::value, "");
static_assert(!fbl::is_function<s[1]>::value, "");
static_assert(!fbl::is_function<s*>::value, "");
static_assert(!fbl::is_function<s&>::value, "");
static_assert(!fbl::is_function<s&&>::value, "");

static_assert(!fbl::is_function<u>::value, "");
static_assert(!fbl::is_function<u[]>::value, "");
static_assert(!fbl::is_function<u[1]>::value, "");
static_assert(!fbl::is_function<u*>::value, "");
static_assert(!fbl::is_function<u&>::value, "");
static_assert(!fbl::is_function<u&&>::value, "");

static_assert(!fbl::is_function<forward_s>::value, "");
static_assert(!fbl::is_function<forward_s[]>::value, "");
static_assert(!fbl::is_function<forward_s[1]>::value, "");
static_assert(!fbl::is_function<forward_s*>::value, "");
static_assert(!fbl::is_function<forward_s&>::value, "");
static_assert(!fbl::is_function<forward_s&&>::value, "");

static_assert(!fbl::is_function<decltype(nullptr)>::value, "");
static_assert(!fbl::is_function<decltype(nullptr)[]>::value, "");
static_assert(!fbl::is_function<decltype(nullptr)[1]>::value, "");
static_assert(!fbl::is_function<decltype(nullptr)*>::value, "");
static_assert(!fbl::is_function<decltype(nullptr)&>::value, "");
static_assert(!fbl::is_function<decltype(nullptr)&&>::value, "");

// There are pointers to void, but no references to void or arrays of
// void.
static_assert(!fbl::is_function<void>::value, "");
static_assert(!fbl::is_function<void*>::value, "");

using member_function_pointer = void(s::*)();
static_assert(!fbl::is_function<member_function_pointer>::value, "");
static_assert(!fbl::is_function<member_function_pointer[]>::value, "");
static_assert(!fbl::is_function<member_function_pointer[1]>::value, "");
static_assert(!fbl::is_function<member_function_pointer*>::value, "");
static_assert(!fbl::is_function<member_function_pointer&>::value, "");
static_assert(!fbl::is_function<member_function_pointer&&>::value, "");

// Functions are functions, but pointers or references to them are
// not.

using void_to_void = void();
static_assert(fbl::is_function<void_to_void>::value, "");
static_assert(!fbl::is_function<void_to_void*>::value, "");
static_assert(!fbl::is_function<void_to_void&>::value, "");
static_assert(!fbl::is_function<void_to_void&&>::value, "");

using void_to_int = int();
static_assert(fbl::is_function<void_to_int>::value, "");
static_assert(!fbl::is_function<void_to_int*>::value, "");
static_assert(!fbl::is_function<void_to_int&>::value, "");
static_assert(!fbl::is_function<void_to_int&&>::value, "");

using int_to_void = void(int);
static_assert(fbl::is_function<int_to_void>::value, "");
static_assert(!fbl::is_function<int_to_void*>::value, "");
static_assert(!fbl::is_function<int_to_void&>::value, "");
static_assert(!fbl::is_function<int_to_void&&>::value, "");

using int_to_int = int(int);
static_assert(fbl::is_function<int_to_int>::value, "");
static_assert(!fbl::is_function<int_to_int*>::value, "");
static_assert(!fbl::is_function<int_to_int&>::value, "");
static_assert(!fbl::is_function<int_to_int&&>::value, "");

using s_to_void = void(s);
static_assert(fbl::is_function<s_to_void>::value, "");
static_assert(!fbl::is_function<s_to_void*>::value, "");
static_assert(!fbl::is_function<s_to_void&>::value, "");
static_assert(!fbl::is_function<s_to_void&&>::value, "");

using u_to_void = void(u);
static_assert(fbl::is_function<u_to_void>::value, "");
static_assert(!fbl::is_function<u_to_void*>::value, "");
static_assert(!fbl::is_function<u_to_void&>::value, "");
static_assert(!fbl::is_function<u_to_void&&>::value, "");

using va_to_void = void(...);
static_assert(fbl::is_function<va_to_void>::value, "");
static_assert(!fbl::is_function<va_to_void*>::value, "");
static_assert(!fbl::is_function<va_to_void&>::value, "");
static_assert(!fbl::is_function<va_to_void&&>::value, "");

using int_va_to_void = void(int, ...);
static_assert(fbl::is_function<int_va_to_void>::value, "");
static_assert(!fbl::is_function<int_va_to_void*>::value, "");
static_assert(!fbl::is_function<int_va_to_void&>::value, "");
static_assert(!fbl::is_function<int_va_to_void&&>::value, "");

auto lambda = [](){};
auto function_pointer_inside_lambda = static_cast<void(*)()>(lambda);
static_assert(fbl::is_pointer<decltype(function_pointer_inside_lambda)>::value, "");
using function_type = fbl::remove_pointer<decltype(function_pointer_inside_lambda)>::type;
static_assert(fbl::is_function<function_type>::value, "");

}; // namespace is_function_tests
