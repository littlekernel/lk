// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unittest/unittest.h>
#include <fbl/intrusive_single_list.h>
#include <fbl/tests/intrusive_containers/intrusive_singly_linked_list_checker.h>
#include <fbl/tests/intrusive_containers/sequence_container_test_environment.h>
#include <fbl/tests/intrusive_containers/test_thunks.h>

namespace fbl {
namespace tests {
namespace intrusive_containers {

template <typename ContainerStateType>
struct OtherListTraits {
    using PtrTraits = typename ContainerStateType::PtrTraits;
    static ContainerStateType& node_state(typename PtrTraits::RefType obj) {
        return obj.other_container_state_;
    }
};

template <typename PtrType>
class SLLTraits {
public:
    using TestObjBaseType         = TestObjBase;

    using ContainerType           = SinglyLinkedList<PtrType>;
    using ContainableBaseClass    = SinglyLinkedListable<PtrType>;
    using ContainerStateType      = SinglyLinkedListNodeState<PtrType>;

    using OtherContainerStateType = ContainerStateType;
    using OtherContainerTraits    = OtherListTraits<OtherContainerStateType>;
    using OtherContainerType      = SinglyLinkedList<PtrType, OtherContainerTraits>;
};

DEFINE_TEST_OBJECTS(SLL);
using UMTE = DEFINE_TEST_THUNK(Sequence, SLL, Unmanaged);
using UPTE = DEFINE_TEST_THUNK(Sequence, SLL, UniquePtr);
using RPTE = DEFINE_TEST_THUNK(Sequence, SLL, RefPtr);

BEGIN_TEST_CASE(single_linked_list_tests)
//////////////////////////////////////////
// General container specific tests.
//////////////////////////////////////////
RUN_NAMED_TEST("Clear (unmanaged)",             UMTE::ClearTest)
RUN_NAMED_TEST("Clear (unique)",                UPTE::ClearTest)
RUN_NAMED_TEST("Clear (RefPtr)",                RPTE::ClearTest)

RUN_NAMED_TEST("ClearUnsafe (unmanaged)",       UMTE::ClearUnsafeTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("ClearUnsafe (unique)",          UPTE::ClearUnsafeTest)
RUN_NAMED_TEST("ClearUnsafe (RefPtr)",          RPTE::ClearUnsafeTest)
#endif

RUN_NAMED_TEST("IsEmpty (unmanaged)",           UMTE::IsEmptyTest)
RUN_NAMED_TEST("IsEmpty (unique)",              UPTE::IsEmptyTest)
RUN_NAMED_TEST("IsEmpty (RefPtr)",              RPTE::IsEmptyTest)

RUN_NAMED_TEST("Iterate (unmanaged)",           UMTE::IterateTest)
RUN_NAMED_TEST("Iterate (unique)",              UPTE::IterateTest)
RUN_NAMED_TEST("Iterate (RefPtr)",              RPTE::IterateTest)

// SinglyLinkedLists cannot perform direct erase operations, nor can they erase
// using an iterator.
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("IterErase (unmanaged)",         UMTE::IterEraseTest)
RUN_NAMED_TEST("IterErase (unique)",            UPTE::IterEraseTest)
RUN_NAMED_TEST("IterErase (RefPtr)",            RPTE::IterEraseTest)

RUN_NAMED_TEST("DirectErase (unmanaged)",       UMTE::DirectEraseTest)
RUN_NAMED_TEST("DirectErase (unique)",          UPTE::DirectEraseTest)
RUN_NAMED_TEST("DirectErase (RefPtr)",          RPTE::DirectEraseTest)
#endif

RUN_NAMED_TEST("MakeIterator (unmanaged)",      UMTE::MakeIteratorTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("MakeIterator (unique)",         UPTE::MakeIteratorTest)
#endif
RUN_NAMED_TEST("MakeIterator (RefPtr)",         RPTE::MakeIteratorTest)

// SinglyLinkedLists cannot iterate backwards.
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("ReverseIterErase (unmanaged)",  UMTE::ReverseIterEraseTest)
RUN_NAMED_TEST("ReverseIterErase (unique)",     UPTE::ReverseIterEraseTest)
RUN_NAMED_TEST("ReverseIterErase (RefPtr)",     RPTE::ReverseIterEraseTest)

RUN_NAMED_TEST("ReverseIterate (unmanaged)",    UMTE::ReverseIterateTest)
RUN_NAMED_TEST("ReverseIterate (unique)",       UPTE::ReverseIterateTest)
RUN_NAMED_TEST("ReverseIterate (RefPtr)",       RPTE::ReverseIterateTest)
#endif

RUN_NAMED_TEST("Swap (unmanaged)",              UMTE::SwapTest)
RUN_NAMED_TEST("Swap (unique)",                 UPTE::SwapTest)
RUN_NAMED_TEST("Swap (RefPtr)",                 RPTE::SwapTest)

RUN_NAMED_TEST("Rvalue Ops (unmanaged)",        UMTE::RvalueOpsTest)
RUN_NAMED_TEST("Rvalue Ops (unique)",           UPTE::RvalueOpsTest)
RUN_NAMED_TEST("Rvalue Ops (RefPtr)",           RPTE::RvalueOpsTest)

RUN_NAMED_TEST("Scope (unique)",                UPTE::ScopeTest)
RUN_NAMED_TEST("Scope (RefPtr)",                RPTE::ScopeTest)

RUN_NAMED_TEST("TwoContainer (unmanaged)",      UMTE::TwoContainerTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("TwoContainer (unique)",         UPTE::TwoContainerTest)
#endif
RUN_NAMED_TEST("TwoContainer (RefPtr)",         RPTE::TwoContainerTest)

RUN_NAMED_TEST("IterCopyPointer (unmanaged)",   UMTE::IterCopyPointerTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("IterCopyPointer (unique)",      UPTE::IterCopyPointerTest)
#endif
RUN_NAMED_TEST("IterCopyPointer (RefPtr)",      RPTE::IterCopyPointerTest)

RUN_NAMED_TEST("EraseIf (unmanaged)",           UMTE::EraseIfTest)
RUN_NAMED_TEST("EraseIf (unique)",              UPTE::EraseIfTest)
RUN_NAMED_TEST("EraseIf (RefPtr)",              RPTE::EraseIfTest)

RUN_NAMED_TEST("FindIf (unmanaged)",            UMTE::FindIfTest)
RUN_NAMED_TEST("FindIf (unique)",               UPTE::FindIfTest)
RUN_NAMED_TEST("FindIf (RefPtr)",               RPTE::FindIfTest)

//////////////////////////////////////////
// Sequence container specific tests.
//////////////////////////////////////////
RUN_NAMED_TEST("PushFront (unmanaged)",         UMTE::PushFrontTest)
RUN_NAMED_TEST("PushFront (unique)",            UPTE::PushFrontTest)
RUN_NAMED_TEST("PushFront (RefPtr)",            RPTE::PushFrontTest)

RUN_NAMED_TEST("PopFront (unmanaged)",          UMTE::PopFrontTest)
RUN_NAMED_TEST("PopFront (unique)",             UPTE::PopFrontTest)
RUN_NAMED_TEST("PopFront (RefPtr)",             RPTE::PopFrontTest)

// Singly linked lists cannot push/pop to/from the back
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("PushBack (unmanaged)",          UMTE::PushBackTest)
RUN_NAMED_TEST("PushBack (unique)",             UPTE::PushBackTest)
RUN_NAMED_TEST("PushBack (RefPtr)",             RPTE::PushBackTest)

RUN_NAMED_TEST("PopBack (unmanaged)",           UMTE::PopBackTest)
RUN_NAMED_TEST("PopBack (unique)",              UPTE::PopBackTest)
RUN_NAMED_TEST("PopBack (RefPtr)",              RPTE::PopBackTest)
#endif

RUN_NAMED_TEST("SeqIterate (unmanaged)",        UMTE::SeqIterateTest)
RUN_NAMED_TEST("SeqIterate (unique)",           UPTE::SeqIterateTest)
RUN_NAMED_TEST("SeqIterate (RefPtr)",           RPTE::SeqIterateTest)

// SinglyLinkedLists cannot iterate backwards.
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("SeqReverseIterate (unmanaged)", UMTE::SeqReverseIterateTest)
RUN_NAMED_TEST("SeqReverseIterate (unique)",    UPTE::SeqReverseIterateTest)
RUN_NAMED_TEST("SeqReverseIterate (RefPtr)",    RPTE::SeqReverseIterateTest)
#endif

RUN_NAMED_TEST("EraseNext (unmanaged)",         UMTE::EraseNextTest)
RUN_NAMED_TEST("EraseNext (unique)",            UPTE::EraseNextTest)
RUN_NAMED_TEST("EraseNext (RefPtr)",            RPTE::EraseNextTest)

RUN_NAMED_TEST("InsertAfter (unmanaged)",       UMTE::InsertAfterTest)
RUN_NAMED_TEST("InsertAfter (unique)",          UPTE::InsertAfterTest)
RUN_NAMED_TEST("InsertAfter (RefPtr)",          RPTE::InsertAfterTest)

// SinglyLinkedLists cannot perform inserts-before operations, either with an
// iterator or with a direct object reference.
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("Insert (unmanaged)",            UMTE::InsertTest)
RUN_NAMED_TEST("Insert (unique)",               UPTE::InsertTest)
RUN_NAMED_TEST("Insert (RefPtr)",               RPTE::InsertTest)

RUN_NAMED_TEST("DirectInsert (unmanaged)",      UMTE::DirectInsertTest)
RUN_NAMED_TEST("DirectInsert (unique)",         UPTE::DirectInsertTest)
RUN_NAMED_TEST("DirectInsert (RefPtr)",         RPTE::DirectInsertTest)
#endif

// SinglyLinkedLists cannot perform splice operations.
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("Splice (unmanaged)",            UMTE::SpliceTest)
RUN_NAMED_TEST("Splice (unique)",               UPTE::SpliceTest)
RUN_NAMED_TEST("Splice (RefPtr)",               RPTE::SpliceTest)
#endif

RUN_NAMED_TEST("ReplaceIfCopy (unmanaged)",     UMTE::ReplaceIfCopyTest)
#if TEST_WILL_NOT_COMPILE || 0
RUN_NAMED_TEST("ReplaceIfCopy (unique)",        UPTE::ReplaceIfCopyTest)
#endif
RUN_NAMED_TEST("ReplaceIfCopy (RefPtr)",        RPTE::ReplaceIfCopyTest)

RUN_NAMED_TEST("ReplaceIfMove (unmanaged)",     UMTE::ReplaceIfMoveTest)
RUN_NAMED_TEST("ReplaceIfMove (unique)",        UPTE::ReplaceIfMoveTest)
RUN_NAMED_TEST("ReplaceIfMove (RefPtr)",        RPTE::ReplaceIfMoveTest)

END_TEST_CASE(single_linked_list_tests);

}  // namespace intrusive_containers
}  // namespace tests
}  // namespace fbl
