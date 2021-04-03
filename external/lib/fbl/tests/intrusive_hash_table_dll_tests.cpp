// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unittest/unittest.h>
#include <fbl/intrusive_double_list.h>
#include <fbl/intrusive_hash_table.h>
#include <fbl/tests/intrusive_containers/associative_container_test_environment.h>
#include <fbl/tests/intrusive_containers/intrusive_hash_table_checker.h>
#include <fbl/tests/intrusive_containers/test_thunks.h>

namespace fbl {
namespace tests {
namespace intrusive_containers {

using OtherKeyType  = uint16_t;
using OtherHashType = uint32_t;
static constexpr OtherHashType kOtherNumBuckets = 23;

template <typename PtrType>
struct OtherHashTraits {
    using ObjType = typename ::fbl::internal::ContainerPtrTraits<PtrType>::ValueType;
    using BucketStateType = DoublyLinkedListNodeState<PtrType>;

    // Linked List Traits
    static BucketStateType& node_state(ObjType& obj) {
        return obj.other_container_state_.bucket_state_;
    }

    // Keyed Object Traits
    static OtherKeyType GetKey(const ObjType& obj) {
        return obj.other_container_state_.key_;
    }

    static bool LessThan(const OtherKeyType& key1, const OtherKeyType& key2) {
        return key1 <  key2;
    }

    static bool EqualTo(const OtherKeyType& key1, const OtherKeyType& key2) {
        return key1 == key2;
    }

    // Hash Traits
    static OtherHashType GetHash(const OtherKeyType& key) {
        return static_cast<OtherHashType>((key * 0xaee58187) % kOtherNumBuckets);
    }

    // Set key is a trait which is only used by the tests, not by the containers
    // themselves.
    static void SetKey(ObjType& obj, OtherKeyType key) {
        obj.other_container_state_.key_ = key;
    }
};

template <typename PtrType>
struct OtherHashState {
private:
    friend struct OtherHashTraits<PtrType>;
    OtherKeyType key_;
    typename OtherHashTraits<PtrType>::BucketStateType bucket_state_;
};

template <typename PtrType>
class HTDLLTraits {
public:
    using ObjType = typename ::fbl::internal::ContainerPtrTraits<PtrType>::ValueType;

    using ContainerType           = HashTable<size_t, PtrType, DoublyLinkedList<PtrType>>;
    using ContainableBaseClass    = DoublyLinkedListable<PtrType>;
    using ContainerStateType      = DoublyLinkedListNodeState<PtrType>;
    using KeyType                 = typename ContainerType::KeyType;
    using HashType                = typename ContainerType::HashType;

    using OtherContainerTraits    = OtherHashTraits<PtrType>;
    using OtherContainerStateType = OtherHashState<PtrType>;
    using OtherBucketType         = DoublyLinkedList<PtrType, OtherContainerTraits>;
    using OtherContainerType      = HashTable<OtherKeyType,
                                              PtrType,
                                              OtherBucketType,
                                              OtherHashType,
                                              kOtherNumBuckets,
                                              OtherContainerTraits,
                                              OtherContainerTraits>;

    using TestObjBaseType  = HashedTestObjBase<typename ContainerType::KeyType,
                                               typename ContainerType::HashType,
                                               ContainerType::kNumBuckets>;
};

DEFINE_TEST_OBJECTS(HTDLL);
using UMTE = DEFINE_TEST_THUNK(Associative, HTDLL, Unmanaged);
using UPTE = DEFINE_TEST_THUNK(Associative, HTDLL, UniquePtr);
using RPTE = DEFINE_TEST_THUNK(Associative, HTDLL, RefPtr);

BEGIN_TEST_CASE(hashtable_dll_tests)
//////////////////////////////////////////
// General container specific tests.
//////////////////////////////////////////
RUN_NAMED_TEST("Clear (unmanaged)",            UMTE::ClearTest)
RUN_NAMED_TEST("Clear (unique)",               UPTE::ClearTest)
RUN_NAMED_TEST("Clear (RefPtr)",               RPTE::ClearTest)

RUN_NAMED_TEST("ClearUnsafe (unmanaged)",      UMTE::ClearUnsafeTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("ClearUnsafe (unique)",         UPTE::ClearUnsafeTest)
RUN_NAMED_TEST("ClearUnsafe (RefPtr)",         RPTE::ClearUnsafeTest)
#endif

RUN_NAMED_TEST("IsEmpty (unmanaged)",          UMTE::IsEmptyTest)
RUN_NAMED_TEST("IsEmpty (unique)",             UPTE::IsEmptyTest)
RUN_NAMED_TEST("IsEmpty (RefPtr)",             RPTE::IsEmptyTest)

RUN_NAMED_TEST("Iterate (unmanaged)",          UMTE::IterateTest)
RUN_NAMED_TEST("Iterate (unique)",             UPTE::IterateTest)
RUN_NAMED_TEST("Iterate (RefPtr)",             RPTE::IterateTest)

RUN_NAMED_TEST("IterErase (unmanaged)",        UMTE::IterEraseTest)
RUN_NAMED_TEST("IterErase (unique)",           UPTE::IterEraseTest)
RUN_NAMED_TEST("IterErase (RefPtr)",           RPTE::IterEraseTest)

RUN_NAMED_TEST("DirectErase (unmanaged)",      UMTE::DirectEraseTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("DirectErase (unique)",         UPTE::DirectEraseTest)
#endif
RUN_NAMED_TEST("DirectErase (RefPtr)",         RPTE::DirectEraseTest)

RUN_NAMED_TEST("MakeIterator (unmanaged)",     UMTE::MakeIteratorTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("MakeIterator (unique)",        UPTE::MakeIteratorTest)
#endif
RUN_NAMED_TEST("MakeIterator (RefPtr)",        RPTE::MakeIteratorTest)

RUN_NAMED_TEST("ReverseIterErase (unmanaged)", UMTE::ReverseIterEraseTest)
RUN_NAMED_TEST("ReverseIterErase (unique)",    UPTE::ReverseIterEraseTest)
RUN_NAMED_TEST("ReverseIterErase (RefPtr)",    RPTE::ReverseIterEraseTest)

RUN_NAMED_TEST("ReverseIterate (unmanaged)",   UMTE::ReverseIterateTest)
RUN_NAMED_TEST("ReverseIterate (unique)",      UPTE::ReverseIterateTest)
RUN_NAMED_TEST("ReverseIterate (RefPtr)",      RPTE::ReverseIterateTest)

// Hash tables do not support swapping or Rvalue operations (Assignment or
// construction) as doing so would be an O(n) operation (With 'n' == to the
// number of buckets in the hashtable)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("Swap (unmanaged)",             UMTE::SwapTest)
RUN_NAMED_TEST("Swap (unique)",                UPTE::SwapTest)
RUN_NAMED_TEST("Swap (RefPtr)",                RPTE::SwapTest)

RUN_NAMED_TEST("Rvalue Ops (unmanaged)",       UMTE::RvalueOpsTest)
RUN_NAMED_TEST("Rvalue Ops (unique)",          UPTE::RvalueOpsTest)
RUN_NAMED_TEST("Rvalue Ops (RefPtr)",          RPTE::RvalueOpsTest)
#endif

RUN_NAMED_TEST("Scope (unique)",               UPTE::ScopeTest)
RUN_NAMED_TEST("Scope (RefPtr)",               RPTE::ScopeTest)

RUN_NAMED_TEST("TwoContainer (unmanaged)",     UMTE::TwoContainerTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("TwoContainer (unique)",        UPTE::TwoContainerTest)
#endif
RUN_NAMED_TEST("TwoContainer (RefPtr)",        RPTE::TwoContainerTest)

RUN_NAMED_TEST("IterCopyPointer (unmanaged)",  UMTE::IterCopyPointerTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("IterCopyPointer (unique)",     UPTE::IterCopyPointerTest)
#endif
RUN_NAMED_TEST("IterCopyPointer (RefPtr)",     RPTE::IterCopyPointerTest)

RUN_NAMED_TEST("EraseIf (unmanaged)",          UMTE::EraseIfTest)
RUN_NAMED_TEST("EraseIf (unique)",             UPTE::EraseIfTest)
RUN_NAMED_TEST("EraseIf (RefPtr)",             RPTE::EraseIfTest)

RUN_NAMED_TEST("FindIf (unmanaged)",           UMTE::FindIfTest)
RUN_NAMED_TEST("FindIf (unique)",              UPTE::FindIfTest)
RUN_NAMED_TEST("FindIf (RefPtr)",              RPTE::FindIfTest)

//////////////////////////////////////////
// Associative container specific tests.
//////////////////////////////////////////
RUN_NAMED_TEST("InsertByKey (unmanaged)",      UMTE::InsertByKeyTest)
RUN_NAMED_TEST("InsertByKey (unique)",         UPTE::InsertByKeyTest)
RUN_NAMED_TEST("InsertByKey (RefPtr)",         RPTE::InsertByKeyTest)

RUN_NAMED_TEST("FindByKey (unmanaged)",        UMTE::FindByKeyTest)
RUN_NAMED_TEST("FindByKey (unique)",           UPTE::FindByKeyTest)
RUN_NAMED_TEST("FindByKey (RefPtr)",           RPTE::FindByKeyTest)

RUN_NAMED_TEST("EraseByKey (unmanaged)",       UMTE::EraseByKeyTest)
RUN_NAMED_TEST("EraseByKey (unique)",          UPTE::EraseByKeyTest)
RUN_NAMED_TEST("EraseByKey (RefPtr)",          RPTE::EraseByKeyTest)

RUN_NAMED_TEST("InsertOrFind (unmanaged)",     UMTE::InsertOrFindTest)
RUN_NAMED_TEST("InsertOrFind (unique)",        UPTE::InsertOrFindTest)
RUN_NAMED_TEST("InsertOrFind (RefPtr)",        RPTE::InsertOrFindTest)

RUN_NAMED_TEST("InsertOrReplace (unmanaged)",  UMTE::InsertOrReplaceTest)
RUN_NAMED_TEST("InsertOrReplace (unique)",     UPTE::InsertOrReplaceTest)
RUN_NAMED_TEST("InsertOrReplace (RefPtr)",     RPTE::InsertOrReplaceTest)
END_TEST_CASE(hashtable_dll_tests);

}  // namespace intrusive_containers
}  // namespace tests
}  // namespace fbl
