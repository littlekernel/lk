// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/assert.h>
#include <fbl/intrusive_container_utils.h>
#include <fbl/intrusive_pointer_traits.h>
#include <fbl/macros.h>

// Usage Notes:
//
// fbl::SinglyLinkedList<> is a templated intrusive container class which
// allows users to manage singly linked lists of objects.
//
// The bookkeeping storage required to exist on a list is a property of the
// objects stored on the list eliminating the need for runtime bookkeeping
// allocation/deallocation to add/remove members to/from the container.
//
// Lists store pointers to the objects, not the objects themselves, and are
// templated based on the specific type of pointer to be stored.  Supported
// pointer types are....
// 1) T*            : raw unmanaged pointers
// 2) unique_ptr<T> : unique managed pointers.
// 3) RefPtr<T>     : managed pointers to ref-counted objects.
//
// Lists of managed pointer types hold references to objects and follow the
// rules of the particular managed pointer patterns.  Destroying or clearing a
// list of managed pointers will release the references to the objects and may
// end the lifecycle of an object if the reference held by the list happened to
// be the last.
//
// Lists of unmanaged pointer types perform no lifecycle management.  It is up to
// the user of the list class to make sure that lifecycles are managed properly.
// As an added safety, a list of unmanaged pointers will ZX_ASSERT if it is
// destroyed with elements still in it.
//
// Objects may exist in multiple lists (or other containers) through the use of
// custom trait classes.  It should be noted that it is possible to make
// different types lists of unique_ptr<T> for a given T, but there is little
// point in doing so as it is impossible to exist on multiple lists at the same
// time without violating the fundamental rules of unique_ptr<T> patterns.
//
// Default traits and a helper base class are provided to make it easy to
// implement list-able objects intended to exist on only one type of list.
//
////////////////////////////////////////////////////////////////////////////////
// Example: A simple list of unmanaged pointers to Foo objects
//
// class Foo : public fbl::SinglyLinkedListable<Foo*> {
//      ...
// };
//
// void Test() {
//     fbl::SinglyLinkedList<Foo*> list;
//
//     for (size_t i = 0; SOME_NUMBER; ++i)
//         list.push_front(new Foo(...));
//
//     for (const auto& foo : list)
//         foo.print();
//
//     while (!list.is_empty())
//         delete list.pop_front();
// }
//
////////////////////////////////////////////////////////////////////////////////
// Example: A simple list of unique pointers to Foo objects
//
// class Foo : public fbl::SinglyLinkedListable<unique_ptr<Foo>> {
//      ...
// };
//
// void Test() {
//     fbl::SinglyLinkedList<unique_ptr<Foo>> list;
//
//     for (size_t i = 0; SOME_NUMBER; ++i) {
//         unique_ptr<Foo> new_foo(new Foo(...));
//         list.push_front(fbl::move(new_foo));
//     }
//
//     for (const auto& foo : list)
//         foo.print();
//
//     list.clear();    // Could also just let the list go out of scope.
// }
//
////////////////////////////////////////////////////////////////////////////////
// Example: A more complicated example of a list of ref-counted objects which
// can exist on 3 different types of lists at the same time.
//
// class Foo : public fbl::SinglyLinkedListable<fbl::RefPtr<Foo>>
//           , public fbl::RefCounted<Foo> {
// public:
//     using NodeState = SinglyLinkedListNodeState<Foo>;
//     struct TypeATraits { static NodeState& node_state(Foo& foo) { return foo.type_a_state_; } }
//     struct TypeBTraits { static NodeState& node_state(Foo& foo) { return foo.type_b_state_; } }
//
//     /* Class implementation goes here */
//
// private:
//     friend struct TypeATraits;
//     friend struct TypeBTraits;
//     NodeState type_a_state_;
//     NodeState type_b_state_;
// };
//
// void Test() {
//     using DefaultList = fbl::SinglyLinkedList<fbl::RefPtr<Foo>>;
//     using TypeAList   = fbl::SinglyLinkedList<fbl::RefPtr<Foo>, Foo::TypeATraits>;
//     using TypeBList   = fbl::SinglyLinkedList<fbl::RefPtr<Foo>, Foo::TypeBTraits>;
//
//     DefaultList default_list;
//     TypeAList a_list;
//     TypeAList b_list;
//
//     for (size_t i = 0; SOME_NUMBER; ++i) {
//         RefPtr new_foo = AdoptRef(new Foo(...));
//
//         switch (i & 0x3) {
//             case 0: break;
//             case 1: a_list.push_front(new_foo); break;
//             case 2: b_list.push_front(new_foo); break;
//             case 3:
//                 a_list.push_front(new_foo);
//                 b_list.push_front(new_foo);
//                 break;
//         }
//
//         default_list.push_front(fbl::move(new_foo));
//     }
//
//     for (const auto& foo : default_list) foo.print();
//     for (const auto& foo : a_list) foo.print();
//     for (const auto& foo : b_list) foo.print();
//
//     default_list.clear();  // case 0 Foo's get cleaned up.
//     a_list.clear();        // case 1 Foo's get cleaned up.
//     b_list.clear();        // case 2 and 3 Foo's get cleaned up.
// }

namespace fbl {

// Fwd decl of classes used by tests.
namespace tests {
namespace intrusive_containers {
class SinglyLinkedListChecker;
template <typename> class SequenceContainerTestEnvironment;
}  // namespace tests
}  // namespace intrusive_containers

// SinglyLinkedListNodeState<T>
//
// The state needed to be a member of a SinglyLinkedList<T>.  All members of a
// specific type SinglyLinkedList<T> must expose a SinglyLinkedListNodeState<T>
// to the list implementation via the supplied traits.  See
// DefaultSinglyLinkedListTraits<T>
template <typename T>
struct SinglyLinkedListNodeState {
    using PtrTraits = internal::ContainerPtrTraits<T>;
    constexpr SinglyLinkedListNodeState() { }
    ~SinglyLinkedListNodeState() {
        // Note; this ASSERT can only be enforced for lists made of managed
        // pointer types.  Lists formed from unmanaged pointers can potentially
        // leave next_ in a non-null state during destruction if the list is
        // cleared using "clear_unsafe".
        ZX_DEBUG_ASSERT(!PtrTraits::IsManaged || (next_ == nullptr));
    }

    bool IsValid() const     { return true; }
    bool InContainer() const { return (next_ != nullptr); }

private:
    template <typename, typename> friend class SinglyLinkedList;
    template <typename> friend class tests::intrusive_containers::SequenceContainerTestEnvironment;
    friend class tests::intrusive_containers::SinglyLinkedListChecker;

    typename PtrTraits::RawPtrType next_ = nullptr;
};

// DefaultSinglyLinkedListNodeState<T>
//
// The default implementation of traits needed to be a member of a singly linked
// list.  Any valid traits implementation must expose a static node_state method
// compatible with DefaultSinglyLinkedListTraits<T>::node_state(...).  To use
// the default traits, an object may...
//
// 1) Be friends with DefaultSinglyLinkedListTraits<T> and have a private
//    sll_node_state_ member.
// 2) Have a public sll_node_state_ member (not recommended)
// 3) Derive from SinglyLinkedListable<T> (easiest)
template <typename T>
struct DefaultSinglyLinkedListTraits {
    using PtrTraits = internal::ContainerPtrTraits<T>;
    static SinglyLinkedListNodeState<T>& node_state(typename PtrTraits::RefType obj) {
        return obj.sll_node_state_;
    }
};

// SinglyLinkedListable<T>
//
// A helper class which makes it simple to exist on a singly linked list.
// Simply derive your object from SinglyLinkedListable and you are done.
template <typename T>
struct SinglyLinkedListable {
public:
    bool InContainer() const { return sll_node_state_.InContainer(); }

private:
    friend struct DefaultSinglyLinkedListTraits<T>;
    SinglyLinkedListNodeState<T> sll_node_state_;
};

template <typename T, typename _NodeTraits = DefaultSinglyLinkedListTraits<T>>
class SinglyLinkedList {
private:
    // Private fwd decls of the iterator implementation.
    template <typename IterTraits> class iterator_impl;
    struct iterator_traits;
    struct const_iterator_traits;

public:
    // Aliases used to reduce verbosity and expose types/traits to tests
    using PtrTraits     = internal::ContainerPtrTraits<T>;
    using NodeTraits    = _NodeTraits;
    using PtrType       = typename PtrTraits::PtrType;
    using RawPtrType    = typename PtrTraits::RawPtrType;
    using ValueType     = typename PtrTraits::ValueType;
    using CheckerType   = ::fbl::tests::intrusive_containers::SinglyLinkedListChecker;
    using ContainerType = SinglyLinkedList<T, NodeTraits>;

    // Declarations of the standard iterator types.
    using iterator       = iterator_impl<iterator_traits>;
    using const_iterator = iterator_impl<const_iterator_traits>;

    // Singly linked lists do not support constant order erase (erase using an
    // iterator or direct object reference).
    static constexpr bool SupportsConstantOrderErase = false;
    static constexpr bool SupportsConstantOrderSize = false;
    static constexpr bool IsAssociative = false;
    static constexpr bool IsSequenced = true;

    // Default construction gives an empty list.
    constexpr SinglyLinkedList() { }

    // Rvalue construction is permitted, but will result in the move of the list
    // contents from one instance of the list to the other (even for unmanaged
    // pointers)
    SinglyLinkedList(SinglyLinkedList<T, NodeTraits>&& other_list) {
        swap(other_list);
    }

    // Rvalue assignment is permitted for managed lists, and when the target is
    // an empty list of unmanaged pointers.  Like Rvalue construction, it will
    // result in the move of the source contents to the destination.
    SinglyLinkedList& operator=(SinglyLinkedList&& other_list) {
        ZX_DEBUG_ASSERT(PtrTraits::IsManaged || is_empty());

        clear();
        swap(other_list);

        return *this;
    }

    ~SinglyLinkedList() {
        // It is considered an error to allow a list of unmanaged pointers to
        // destruct of there are still elements in it.  Managed pointer lists
        // will automatically release their references to their elements.
        ZX_DEBUG_ASSERT(PtrTraits::IsManaged || is_empty());
        clear();
    }

    // Standard begin/end, cbegin/cend iterator accessors.
    iterator        begin()       { return iterator(head_); }
    const_iterator  begin() const { return const_iterator(head_); }
    const_iterator cbegin() const { return const_iterator(head_); }

    iterator          end()       { return iterator(sentinel()); }
    const_iterator    end() const { return const_iterator(sentinel()); }
    const_iterator   cend() const { return const_iterator(sentinel()); }

    // make_iterator : construct an iterator out of a reference to an object.
    iterator make_iterator(ValueType& obj) { return iterator(&obj); }

    // is_empty
    //
    // True if the list has at least one element in it, false otherwise.
    bool is_empty() const {
        ZX_DEBUG_ASSERT(head_ != nullptr);
        return internal::is_sentinel_ptr(head_);
    }

    // front
    //
    // Return a reference to the element at the front of the list without
    // removing it.  It is an error to call front on an empty list.
    typename PtrTraits::RefType      front()       { ZX_DEBUG_ASSERT(!is_empty()); return *head_; }
    typename PtrTraits::ConstRefType front() const { ZX_DEBUG_ASSERT(!is_empty()); return *head_; }

    // push_front
    //
    // Push an element onto the front of the lists.  Lvalue and Rvalue
    // versions are supplied in order to support move semantics.  It
    // is an error to attempt to push a nullptr instance of PtrType.
    void push_front(const PtrType& ptr) { push_front(PtrType(ptr)); }
    void push_front(PtrType&& ptr) {
        ZX_DEBUG_ASSERT(ptr != nullptr);

        auto& ptr_ns = NodeTraits::node_state(*ptr);
        ZX_DEBUG_ASSERT(!ptr_ns.InContainer());

        ptr_ns.next_ = head_;
        head_        = PtrTraits::Leak(ptr);
    }

    // insert_after
    //
    // Insert an element after iter in the list.  It is an error to attempt to
    // push a nullptr instance of PtrType, or to attempt to push with iter ==
    // end().
    void insert_after(const iterator& iter, const PtrType& ptr) {
        insert_after(iter, PtrType(ptr));
    }
    void insert_after(const iterator& iter, PtrType&& ptr) {
        ZX_DEBUG_ASSERT(iter.IsValid());
        ZX_DEBUG_ASSERT(ptr != nullptr);

        auto& iter_ns = NodeTraits::node_state(*iter.node_);
        auto& ptr_ns  = NodeTraits::node_state(*ptr);
        ZX_DEBUG_ASSERT(!ptr_ns.InContainer());

        ptr_ns.next_  = iter_ns.next_;
        iter_ns.next_ = PtrTraits::Leak(ptr);
    }

    // pop_front
    //
    // Removes the head of the list and transfer the pointer to the
    // caller.  If the list is empty, return a nullptr instance of
    // PtrType.
    PtrType pop_front() {
        if (is_empty())
            return PtrType(nullptr);

        auto& head_ns = NodeTraits::node_state(*head_);
        PtrType ret = PtrTraits::Reclaim(head_);

        head_ = head_ns.next_;
        head_ns.next_ = nullptr;

        return ret;
    }

    // clear
    //
    // Clear out the list, unlinking all of the elements in the process.  For
    // managed pointer types, this will release all references held by the list
    // to the objects which were in it.
    void clear() {
        while (!is_empty()) {
            auto& head_ns = NodeTraits::node_state(*head_);
            RawPtrType tmp = head_;
            head_ = head_ns.next_;
            head_ns.next_ = nullptr;
            PtrTraits::Reclaim(tmp);
        }
    }

    // clear_unsafe
    //
    // A special clear operation which just resets the internal container
    // structure, but leaves all of the node-state(s) of the current element(s)
    // alone.
    //
    // Only usable with containers of unmanaged pointers (Very Bad things can happen
    // if you try this with containers of managed pointers).
    //
    // Note: While this can be useful in special cases (such as resetting a free
    // list for a pool/slab allocator during destruction), you normally do not
    // want this behavior.  Think carefully before calling this!
    void clear_unsafe() {
        static_assert(PtrTraits::IsManaged == false,
                     "clear_unsafe is not allowed for containers of managed pointers");
        head_ = sentinel();
    }

    // erase_next
    //
    // Remove the element in the list which follows iter.  If there is no
    // element in the list which follows iter, return a nullptr instance of
    // PtrType.  It is an error to attempt to erase_next an invalid iterator
    // (either an uninitialized iterator, or an iterator which is equal to
    // end())
    PtrType erase_next(const iterator& iter) {
        ZX_DEBUG_ASSERT(iter.IsValid());
        auto& iter_ns = NodeTraits::node_state(*iter);

        if (internal::is_sentinel_ptr(iter_ns.next_))
            return PtrType(nullptr);

        auto& next_ns = NodeTraits::node_state(*iter_ns.next_);

        PtrType ret = PtrTraits::Reclaim(iter_ns.next_);
        iter_ns.next_ = next_ns.next_;
        next_ns.next_ = nullptr;
        return ret;
    }

    // swap
    //
    // swaps the contest of two lists.
    void swap(SinglyLinkedList<T, NodeTraits>& other) {
        auto tmp = head_;
        head_ = other.head_;
        other.head_ = tmp;
    }

    // size_slow
    //
    // count the elements in the list in O(n) fashion
    size_t size_slow() const {
        size_t size = 0;

        for (auto iter = cbegin(); iter != cend(); ++iter)
            size++;

        return size;
    }

    // erase_if
    //
    // Find the first member of the list which satisfies the predicate given by
    // 'fn' and erase it from the list, returning a referenced pointer to the
    // removed element.  Return nullptr if no member satisfies the predicate.
    template <typename UnaryFn>
    PtrType erase_if(UnaryFn fn) {
        using ConstRefType  = typename PtrTraits::ConstRefType;

        if (is_empty())
            return PtrType(nullptr);

        auto iter = begin();
        if (fn(static_cast<ConstRefType>(*iter)))
            return pop_front();

        for (auto prev = iter++; iter != end(); prev = iter++) {
            if (fn(static_cast<ConstRefType>(*iter)))
                return erase_next(prev);
        }

        return PtrType(nullptr);
    }

    // find_if
    //
    // Find the first member of the list which satisfies the predicate given by
    // 'fn' and return an iterator in the list which refers to it.  Return end()
    // if no member satisfies the predicate.
    template <typename UnaryFn>
    const_iterator find_if(UnaryFn fn) const {
        for (auto iter = begin(); iter.IsValid(); ++iter) {
            if (fn(*iter))
                return iter;
        }

        return end();
    }

    template <typename UnaryFn>
    iterator find_if(UnaryFn fn) {
        const_iterator citer = const_cast<const ContainerType*>(this)->find_if(fn);
        return iterator(citer.node_);
    }

    // replace_if (copy)
    //
    // Find the first member of the list which satisfies the predicate given by
    // 'fn' and replace it in the list, returning a referenced pointer to the
    // replaced element.  If no member satisfies the predicate, simply return
    // nullptr instead.
    template <typename UnaryFn>
    PtrType replace_if(UnaryFn fn, const PtrType& ptr) {
        using ConstRefType  = typename PtrTraits::ConstRefType;
        ZX_DEBUG_ASSERT(ptr != nullptr);

        auto& ptr_ns = NodeTraits::node_state(*ptr);
        ZX_DEBUG_ASSERT(!ptr_ns.InContainer());

        auto iter = begin();
        if (iter.IsValid()) {
            RawPtrType* prev_next_ptr = &head_;

            while (iter.IsValid()) {
                auto& iter_ns = NodeTraits::node_state(*iter);

                if (fn(static_cast<ConstRefType>(*iter))) {
                    PtrType new_ref = ptr;
                    RawPtrType replaced;

                    replaced       = *prev_next_ptr;
                    *prev_next_ptr = PtrTraits::Leak(new_ref);
                    ptr_ns.next_   = iter_ns.next_;
                    iter_ns.next_  = nullptr;

                    return PtrTraits::Reclaim(replaced);
                }

                prev_next_ptr = &iter_ns.next_;
                ++iter;
            }
        }

        return nullptr;
    }

    // replace_if (move)
    //
    // Same as the copy version, except that if no member satisfies the
    // predicate, the original reference is returned instead of nullptr.
    template <typename UnaryFn>
    PtrType replace_if(UnaryFn fn, PtrType&& ptr) {
        using ConstRefType  = typename PtrTraits::ConstRefType;
        ZX_DEBUG_ASSERT(ptr != nullptr);

        auto& ptr_ns = NodeTraits::node_state(*ptr);
        ZX_DEBUG_ASSERT(!ptr_ns.InContainer());

        auto iter = begin();
        if (iter.IsValid()) {
            RawPtrType* prev_next_ptr = &head_;

            while (iter.IsValid()) {
                auto& iter_ns = NodeTraits::node_state(*iter);

                if (fn(static_cast<ConstRefType>(*iter))) {
                    RawPtrType replaced;

                    replaced       = *prev_next_ptr;
                    *prev_next_ptr = PtrTraits::Leak(ptr);
                    ptr_ns.next_   = iter_ns.next_;
                    iter_ns.next_  = nullptr;

                    return PtrTraits::Reclaim(replaced);
                }

                prev_next_ptr = &iter_ns.next_;
                ++iter;
            }
        }

        return fbl::move(ptr);
    }

private:
    // The traits of a non-const iterator
    struct iterator_traits {
        using RefType    = typename PtrTraits::RefType;
        using RawPtrType = typename PtrTraits::RawPtrType;
    };

    // The traits of a const iterator
    struct const_iterator_traits {
        using RefType    = typename PtrTraits::ConstRefType;
        using RawPtrType = typename PtrTraits::ConstRawPtrType;
    };

    // The shared implementation of the iterator
    template <class IterTraits>
    class iterator_impl {
    public:
        iterator_impl() { }
        iterator_impl(const iterator_impl& other) { node_ = other.node_; }

        iterator_impl& operator=(const iterator_impl& other) {
            node_ =  other.node_;
            return *this;
        }

        bool IsValid() const { return (node_ != nullptr) && !internal::is_sentinel_ptr(node_); }
        bool operator==(const iterator_impl& other) const { return node_ == other.node_; }
        bool operator!=(const iterator_impl& other) const { return node_ != other.node_; }

        // Prefix
        iterator_impl& operator++() {
            if (!IsValid()) return *this;

            node_ = NodeTraits::node_state(*node_).next_;

            return *this;
        }

        // Postfix
        iterator_impl operator++(int) {
            iterator_impl ret(*this);
            ++(*this);
            return ret;
        }

        typename PtrTraits::PtrType CopyPointer() const {
            return IsValid() ? PtrTraits::Copy(node_) : nullptr;
        }

        typename IterTraits::RefType operator*() const {
            ZX_DEBUG_ASSERT(IsValid());
            return *node_;
        }

        typename IterTraits::RawPtrType operator->() const {
            ZX_DEBUG_ASSERT(IsValid());
            return node_;
        }

    private:
        friend class SinglyLinkedList<T, NodeTraits>;

        explicit iterator_impl(typename IterTraits::RawPtrType node)
            : node_(const_cast<typename PtrTraits::RawPtrType>(node)) { }

        typename PtrTraits::RawPtrType node_ = nullptr;
    };

    // The test framework's 'checker' class is our friend.
    friend CheckerType;

    // move semantics only
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(SinglyLinkedList);

    // Note: the sentinel value we use for singly linked list is a bit different
    // from the sentinel value we use for everything else.  Instead of being the
    // this pointer of the container with the sentinel bit set, the sentinel is
    // just the bit with nothing else (aka; nullptr | kContainerSentinelBit).
    //
    // The reasons which drive this decision are as follows.
    // 1) When swapping lists, if the sentinel value was list specific, we would
    //    need to update the sentinel values at the end of each list.  This would
    //    be an O(n) operation for a SLL, whereas it is an O(1) operation for
    //    every other container.
    // 2) The sentinel value used by a list cannot simply be nullptr, or the
    //    node state for an element which is list-able would not be able to
    //    distinguish between an element which was not InContainer() and one
    //    which was InContainer, but located at the end of the list.
    constexpr RawPtrType sentinel() const {
        return internal::make_sentinel<RawPtrType>(nullptr);
    }

    // State consists of just a head pointer.
    RawPtrType head_ = sentinel();
};

// Explicit declaration of constexpr storage.
template <typename T, typename NodeTraits>
constexpr bool SinglyLinkedList<T, NodeTraits>::SupportsConstantOrderErase;
template <typename T, typename NodeTraits>
constexpr bool SinglyLinkedList<T, NodeTraits>::SupportsConstantOrderSize;
template <typename T, typename NodeTraits>
constexpr bool SinglyLinkedList<T, NodeTraits>::IsAssociative;
template <typename T, typename NodeTraits>
constexpr bool SinglyLinkedList<T, NodeTraits>::IsSequenced;

}  // namespace fbl
