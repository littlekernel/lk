// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/alloc_checker.h>
#include <fbl/ref_ptr.h>
#include <fbl/ref_counted.h>
#include <fbl/unique_ptr.h>
#include <fbl/type_support.h>
#include <unittest/unittest.h>

namespace {

template <typename T> struct PtrTraits;

template <typename T>
struct PtrTraits<fbl::unique_ptr<T>> {
    using ObjType = typename fbl::remove_cv<T>::type;
    static fbl::unique_ptr<T> MakePointer(T* raw) { return fbl::unique_ptr<T>(raw); }
};

template <typename T>
struct PtrTraits<fbl::RefPtr<T>> {
    using ObjType = typename fbl::remove_cv<T>::type;
    static fbl::RefPtr<T> MakePointer(T* raw) { return fbl::AdoptRef<T>(raw); }
};


class TestBase {
public:
    TestBase() { recycle_was_called_ = false; }
    static bool recycle_was_called() { return recycle_was_called_; }
protected:
    static bool recycle_was_called_;
};

bool TestBase::recycle_was_called_;

class TestPublicRecycle : public TestBase,
                          public fbl::Recyclable<TestPublicRecycle> {
public:
    void fbl_recycle() {
        recycle_was_called_ = true;
        delete this;
    }
};

class RefedTestPublicRecycle : public TestBase,
                               public fbl::RefCounted<RefedTestPublicRecycle>,
                               public fbl::Recyclable<RefedTestPublicRecycle> {
public:
    void fbl_recycle() {
        recycle_was_called_ = true;
        delete this;
    }
};

class TestPrivateRecycle : public TestBase,
                           public fbl::Recyclable<TestPrivateRecycle> {
private:
    friend class fbl::Recyclable<TestPrivateRecycle>;
    void fbl_recycle() {
        recycle_was_called_ = true;
        delete this;
    }
};

class RefedTestPrivateRecycle : public TestBase,
                                public fbl::RefCounted<RefedTestPrivateRecycle>,
                                public fbl::Recyclable<RefedTestPrivateRecycle> {
private:
    friend class fbl::Recyclable<RefedTestPrivateRecycle>;
    void fbl_recycle() {
        recycle_was_called_ = true;
        delete this;
    }
};

struct FailNoMethod : public fbl::Recyclable<FailNoMethod> { };
struct FailBadRet : public fbl::Recyclable<FailBadRet> { int fbl_recycle() { return 1; } };
struct FailBadArg : public fbl::Recyclable<FailBadArg> { void fbl_recycle(int a = 1) {} };
class  FailNotVis : public fbl::Recyclable<FailNotVis> { void fbl_recycle() {} };

#if TEST_WILL_NOT_COMPILE || 0
struct FailCVBase1 : public fbl::Recyclable<const FailCVBase1> { void fbl_recycle() {} };
#endif
#if TEST_WILL_NOT_COMPILE || 0
struct FailCVBase2 : public fbl::Recyclable<volatile FailCVBase2> { void fbl_recycle() {} };
#endif
#if TEST_WILL_NOT_COMPILE || 0
struct FailCVBase3 : public fbl::Recyclable<const volatile FailCVBase3> { void fbl_recycle() {} };
#endif

template <typename T>
static bool do_test() {
    BEGIN_TEST;

    fbl::AllocChecker ac;
    auto ptr = PtrTraits<T>::MakePointer(new (&ac) typename PtrTraits<T>::ObjType);

    ASSERT_TRUE(ac.check());
    EXPECT_FALSE(TestBase::recycle_was_called());

    ptr = nullptr;
    EXPECT_TRUE(TestBase::recycle_was_called());

    END_TEST;
}

}  // anon namespace

BEGIN_TEST_CASE(fbl_recycle)
RUN_NAMED_TEST("public unique_ptr fbl_recycle()",
               do_test<fbl::unique_ptr<TestPublicRecycle>>)
RUN_NAMED_TEST("public const unique_ptr fbl_recycle()",
               do_test<fbl::unique_ptr<const TestPublicRecycle>>)
RUN_NAMED_TEST("public volatile unique_ptr fbl_recycle()",
               do_test<fbl::unique_ptr<volatile TestPublicRecycle>>)
RUN_NAMED_TEST("public const volatile unique_ptr fbl_recycle()",
               do_test<fbl::unique_ptr<const volatile TestPublicRecycle>>)
RUN_NAMED_TEST("private unique_ptr fbl_recycle()",
               do_test<fbl::unique_ptr<TestPrivateRecycle>>)
RUN_NAMED_TEST("private const unique_ptr fbl_recycle()",
               do_test<fbl::unique_ptr<const TestPrivateRecycle>>)
RUN_NAMED_TEST("private volatile unique_ptr fbl_recycle()",
               do_test<fbl::unique_ptr<volatile TestPrivateRecycle>>)
RUN_NAMED_TEST("private const volatile unique_ptr fbl_recycle()",
               do_test<fbl::unique_ptr<const volatile TestPrivateRecycle>>)
RUN_NAMED_TEST("public RefPtr fbl_recycle()",
               do_test<fbl::RefPtr<RefedTestPublicRecycle>>)
RUN_NAMED_TEST("private RefPtr fbl_recycle()",
               do_test<fbl::RefPtr<RefedTestPrivateRecycle>>)
#if TEST_WILL_NOT_COMPILE || 0
// TODO(johngro) : If we ever support RefPtr<>s to const/volatile objects,
// instantiate tests for them here.
RUN_NAMED_TEST("public const RefPtr fbl_recycle()",
               do_test<fbl::RefPtr<const RefedTestPublicRecycle>>)
RUN_NAMED_TEST("public volatile RefPtr fbl_recycle()",
               do_test<fbl::RefPtr<volatile RefedTestPublicRecycle>>)
RUN_NAMED_TEST("public const volatile RefPtr fbl_recycle()",
               do_test<fbl::RefPtr<const volatile RefedTestPublicRecycle>>)
RUN_NAMED_TEST("private const RefPtr fbl_recycle()",
               do_test<fbl::RefPtr<const RefedTestPrivateRecycle>>)
RUN_NAMED_TEST("private volatile RefPtr fbl_recycle()",
               do_test<fbl::RefPtr<volatile RefedTestPrivateRecycle>>)
RUN_NAMED_TEST("private const volatile RefPtr fbl_recycle()",
               do_test<fbl::RefPtr<const volatile RefedTestPrivateRecycle>>)
#endif
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("FailNoMethod", do_test<fbl::unique_ptr<FailNoMethod>>);
#endif
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("FailBadRet",   do_test<fbl::unique_ptr<FailBadRet>>);
#endif
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("FailBadArg",   do_test<fbl::unique_ptr<FailBadArg>>);
#endif
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("FailNotVis",   do_test<fbl::unique_ptr<FailBadArg>>);
#endif
END_TEST_CASE(fbl_recycle);
