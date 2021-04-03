// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

namespace fbl {
namespace tests {
namespace intrusive_containers {

#define MAKE_TEST_THUNK(_test_name) \
static bool _test_name ## Test() { \
    TestEnvironmentClass env; \
    BEGIN_TEST; \
    EXPECT_TRUE(env._test_name(), ""); \
    EXPECT_TRUE(env.Reset(), ""); \
    END_TEST; \
}

// A utility class used to generate static test thunks for the various
// combinations of test environments and test object types.
template <typename TestEnvironmentClass>
struct TestThunks {
    // Generic tests
    MAKE_TEST_THUNK(Clear);
    MAKE_TEST_THUNK(ClearUnsafe);
    MAKE_TEST_THUNK(IsEmpty);
    MAKE_TEST_THUNK(Iterate);
    MAKE_TEST_THUNK(IterErase);
    MAKE_TEST_THUNK(DirectErase);
    MAKE_TEST_THUNK(MakeIterator);
    MAKE_TEST_THUNK(ReverseIterate);
    MAKE_TEST_THUNK(ReverseIterErase);
    MAKE_TEST_THUNK(Swap);
    MAKE_TEST_THUNK(RvalueOps);
    MAKE_TEST_THUNK(Scope);
    MAKE_TEST_THUNK(TwoContainer);
    MAKE_TEST_THUNK(IterCopyPointer);
    MAKE_TEST_THUNK(EraseIf);
    MAKE_TEST_THUNK(FindIf);

    // Sequence specific tests
    MAKE_TEST_THUNK(PushFront);
    MAKE_TEST_THUNK(PopFront);
    MAKE_TEST_THUNK(PushBack);
    MAKE_TEST_THUNK(PopBack);
    MAKE_TEST_THUNK(SeqIterate);
    MAKE_TEST_THUNK(SeqReverseIterate);
    MAKE_TEST_THUNK(EraseNext);
    MAKE_TEST_THUNK(InsertAfter);
    MAKE_TEST_THUNK(Insert);
    MAKE_TEST_THUNK(DirectInsert);
    MAKE_TEST_THUNK(Splice);
    MAKE_TEST_THUNK(ReplaceIfCopy);
    MAKE_TEST_THUNK(ReplaceIfMove);
    MAKE_TEST_THUNK(ReplaceCopy);
    MAKE_TEST_THUNK(ReplaceMove);

    // Associative container specific tests
    MAKE_TEST_THUNK(InsertByKey);
    MAKE_TEST_THUNK(FindByKey);
    MAKE_TEST_THUNK(EraseByKey);
    MAKE_TEST_THUNK(InsertOrFind);
    MAKE_TEST_THUNK(InsertOrReplace);

    // Ordered Associative container specific tests
    MAKE_TEST_THUNK(OrderedIter)
    MAKE_TEST_THUNK(OrderedReverseIter)
    MAKE_TEST_THUNK(UpperBound);
    MAKE_TEST_THUNK(LowerBound);
};
#undef MAKE_TEST_THUNK

// Macros used to define test object types, test environments, and test thunk structs used for
// exercising various containers and managed/unmanaged pointer types.
#define DEFINE_TEST_OBJECT(_container_type, _ptr_type, _ptr_prefix, _ptr_suffix, _base_type) \
class _ptr_type ## _container_type ## TestObj :                                              \
    public _base_type<_container_type ## Traits<                                             \
        _ptr_prefix _ptr_type ## _container_type ## TestObj _ptr_suffix>> {                  \
public:                                                                                      \
    explicit _ptr_type ## _container_type ## TestObj(size_t val)                             \
            : _base_type(val) { }                                                            \
};                                                                                           \
using _ptr_type ## _container_type ## TestTraits =                                           \
    _ptr_type ## TestTraits<_ptr_type ## _container_type ## TestObj>

#define DEFINE_TEST_OBJECTS(_container_type)                                      \
    DEFINE_TEST_OBJECT(_container_type, Unmanaged,            , *, TestObj);      \
    DEFINE_TEST_OBJECT(_container_type, UniquePtr, unique_ptr<, >, TestObj);      \
    DEFINE_TEST_OBJECT(_container_type, RefPtr,        RefPtr<, >, RefedTestObj)

#define DEFINE_TEST_THUNK(_env_type, _container_type, _ptr_type) \
    TestThunks<_env_type ## ContainerTestEnvironment<_ptr_type ## _container_type ## TestTraits>>


}  // namespace intrusive_containers
}  // namespace tests
}  // namespace fbl
