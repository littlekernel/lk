// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/assert.h>
#include <fbl/intrusive_container_utils.h>
#include <fbl/intrusive_pointer_traits.h>
#include <fbl/intrusive_single_list.h>
#include <fbl/macros.h>

namespace fbl {

// Fwd decl of sanity checker class used by tests.
namespace tests {
namespace intrusive_containers {
class HashTableChecker;
}  // namespace tests
}  // namespace intrusive_containers

// DefaultHashTraits defines a default implementation of traits used to
// define the hash function for a hash table.
//
// At a minimum, a class or a struct which is to be used to define the
// hash traits of a hashtable must define...
//
// GetHash : A static method which take a constant reference to an instance of
//           the container's KeyType and returns an instance of the container's
//           HashType representing the hashed value of the key.  The value must
//           be on the range from [0, Container::kNumBuckets - 1]
//
// DefaultHashTraits generates a compliant implementation of hash traits taking
// its KeyType, ObjType, HashType and NumBuckets from template parameters.
// Users of DefaultHashTraits only need to implement a static method of ObjType
// named GetHash which takes a const reference to a KeyType and returns a
// HashType.  The default implementation will automatically mod by the number of
// buckets given in the template parameters.  If the user's hash function
// already automatically guarantees that the returned hash value will be in the
// proper range, he/she should implement their own hash traits to avoid the
// extra div/mod operation.
template <typename KeyType,
          typename ObjType,
          typename HashType,
          HashType kNumBuckets>
struct DefaultHashTraits {
    static_assert(is_unsigned_integer<HashType>::value, "HashTypes must be unsigned integers");
    static HashType GetHash(const KeyType& key) {
        return static_cast<HashType>(ObjType::GetHash(key) % kNumBuckets);
    }
};

template <typename  _KeyType,
          typename  _PtrType,
          typename  _BucketType = SinglyLinkedList<_PtrType>,
          typename  _HashType   = size_t,
          _HashType _NumBuckets = 37,
          typename  _KeyTraits  = DefaultKeyedObjectTraits<
                                    _KeyType,
                                    typename internal::ContainerPtrTraits<_PtrType>::ValueType>,
          typename  _HashTraits = DefaultHashTraits<
                                    _KeyType,
                                    typename internal::ContainerPtrTraits<_PtrType>::ValueType,
                                    _HashType,
                                    _NumBuckets>>
class HashTable {
private:
    // Private fwd decls of the iterator implementation.
    template <typename IterTraits> class iterator_impl;
    struct iterator_traits;
    struct const_iterator_traits;

public:
    // Pointer types/traits
    using PtrType      = _PtrType;
    using PtrTraits    = internal::ContainerPtrTraits<PtrType>;
    using ValueType    = typename PtrTraits::ValueType;

    // Key types/traits
    using KeyType      = _KeyType;
    using KeyTraits    = _KeyTraits;

    // Hash types/traits
    using HashType     = _HashType;
    using HashTraits   = _HashTraits;

    // Bucket types/traits
    using BucketType   = _BucketType;
    using NodeTraits   = typename BucketType::NodeTraits;

    // Declarations of the standard iterator types.
    using iterator       = iterator_impl<iterator_traits>;
    using const_iterator = iterator_impl<const_iterator_traits>;

    // An alias for the type of this specific HashTable<...> and its test sanity checker.
    using ContainerType = HashTable<_KeyType, _PtrType, _BucketType, _HashType,
                                    _NumBuckets, _KeyTraits, _HashTraits>;
    using CheckerType   = ::fbl::tests::intrusive_containers::HashTableChecker;

    // The number of buckets should be a nice prime such as 37, 211, 389 unless
    // The hash function is really good. Lots of cheap hash functions have
    // hidden periods for which the mod with prime above 'mostly' fixes.
    static constexpr HashType kNumBuckets = _NumBuckets;

    // Hash tables only support constant order erase if their underlying bucket
    // type does.
    static constexpr bool SupportsConstantOrderErase = BucketType::SupportsConstantOrderErase;
    static constexpr bool SupportsConstantOrderSize = true;
    static constexpr bool IsAssociative = true;
    static constexpr bool IsSequenced = false;

    static_assert(kNumBuckets > 0, "Hash tables must have at least one bucket");
    static_assert(is_unsigned_integer<HashType>::value, "HashTypes must be unsigned integers");

    constexpr HashTable() {}
    ~HashTable() { ZX_DEBUG_ASSERT(PtrTraits::IsManaged || is_empty()); }

    // Standard begin/end, cbegin/cend iterator accessors.
    iterator begin()              { return       iterator(this,       iterator::BEGIN); }
    const_iterator begin()  const { return const_iterator(this, const_iterator::BEGIN); }
    const_iterator cbegin() const { return const_iterator(this, const_iterator::BEGIN); }

    iterator end()              { return       iterator(this,       iterator::END); }
    const_iterator end()  const { return const_iterator(this, const_iterator::END); }
    const_iterator cend() const { return const_iterator(this, const_iterator::END); }

    // make_iterator : construct an iterator out of a reference to an object.
    iterator make_iterator(ValueType& obj) {
        HashType ndx = GetHash(KeyTraits::GetKey(obj));
        return iterator(this, ndx, buckets_[ndx].make_iterator(obj));
    }

    void insert(const PtrType& ptr) { insert(PtrType(ptr)); }
    void insert(PtrType&& ptr) {
        ZX_DEBUG_ASSERT(ptr != nullptr);
        KeyType key = KeyTraits::GetKey(*ptr);
        BucketType& bucket = GetBucket(key);

        // Duplicate keys are disallowed.  Debug assert if someone tries to to
        // insert an element with a duplicate key.  If the user thought that
        // there might be a duplicate key in the HashTable already, he/she
        // should have used insert_or_find() instead.
        ZX_DEBUG_ASSERT(FindInBucket(bucket, key).IsValid() == false);

        bucket.push_front(fbl::move(ptr));
        ++count_;
    }

    // insert_or_find
    //
    // Insert the element pointed to by ptr if it is not already in the
    // HashTable, or find the element that the ptr collided with instead.
    //
    // 'iter' is an optional out parameter pointer to an iterator which
    // will reference either the newly inserted item, or the item whose key
    // collided with ptr.
    //
    // insert_or_find returns true if there was no collision and the item was
    // successfully inserted, otherwise it returns false.
    //
    bool insert_or_find(const PtrType& ptr, iterator* iter = nullptr) {
        return insert_or_find(PtrType(ptr), iter);
    }

    bool insert_or_find(PtrType&& ptr, iterator* iter = nullptr) {
        ZX_DEBUG_ASSERT(ptr != nullptr);
        KeyType  key         = KeyTraits::GetKey(*ptr);
        HashType ndx         = GetHash(key);
        auto&    bucket      = buckets_[ndx];
        auto     bucket_iter = FindInBucket(bucket, key);

        if (bucket_iter.IsValid()) {
            if (iter) *iter = iterator(this, ndx, bucket_iter);
            return false;
        }

        bucket.push_front(fbl::move(ptr));
        ++count_;
        if (iter) *iter = iterator(this, ndx, bucket.begin());
        return true;
    }

    // insert_or_replace
    //
    // Find the element in the hashtable with the same key as *ptr and replace
    // it with ptr, then return the pointer to the element which was replaced.
    // If no element in the hashtable shares a key with *ptr, simply add ptr to
    // the hashtable and return nullptr.
    //
    PtrType insert_or_replace(const PtrType& ptr) {
        return insert_or_replace(PtrType(ptr));
    }

    PtrType insert_or_replace(PtrType&& ptr) {
        ZX_DEBUG_ASSERT(ptr != nullptr);
        KeyType  key    = KeyTraits::GetKey(*ptr);
        HashType ndx    = GetHash(key);
        auto&    bucket = buckets_[ndx];
        auto     orig   = PtrTraits::GetRaw(ptr);

        PtrType replaced = bucket.replace_if(
            [key](const ValueType& other) -> bool {
                return KeyTraits::EqualTo(key, KeyTraits::GetKey(other));
            },
            fbl::move(ptr));

        if (orig == PtrTraits::GetRaw(replaced)) {
            bucket.push_front(fbl::move(replaced));
            count_++;
            return nullptr;
        }

        return replaced;
    }

    iterator find(const KeyType& key) {
        HashType ndx         = GetHash(key);
        auto&    bucket      = buckets_[ndx];
        auto     bucket_iter = FindInBucket(bucket, key);

        return bucket_iter.IsValid() ? iterator(this, ndx, bucket_iter)
                                     : iterator(this, iterator::END);
    }

    const_iterator find(const KeyType& key) const {
        HashType    ndx         = GetHash(key);
        const auto& bucket      = buckets_[ndx];
        auto        bucket_iter = FindInBucket(bucket, key);

        return bucket_iter.IsValid() ? const_iterator(this, ndx, bucket_iter)
                                     : const_iterator(this, const_iterator::END);
    }

    PtrType erase(const KeyType& key) {
        BucketType& bucket = GetBucket(key);

        PtrType ret = internal::KeyEraseUtils<BucketType, KeyTraits>::erase(bucket, key);
        if (ret != nullptr)
            --count_;

        return ret;
    }

    PtrType erase(const iterator& iter) {
        if (!iter.IsValid())
            return PtrType(nullptr);

        return direct_erase(buckets_[iter.bucket_ndx_], *iter);
    }

    PtrType erase(ValueType& obj) {
        return direct_erase(GetBucket(obj), obj);
    }

    // clear
    //
    // Clear out the all of the hashtable buckets.  For managed pointer types,
    // this will release all references held by the hashtable to the objects
    // which were in it.
    void clear() {
        for (auto& e : buckets_)
            e.clear();
        count_ = 0;
    }

    // clear_unsafe
    //
    // Perform a clear_unsafe on all buckets and reset the internal count to
    // zero.  See comments in fbl/intrusive_single_list.h
    // Think carefully before calling this!
    void clear_unsafe() {
        static_assert(PtrTraits::IsManaged == false,
                     "clear_unsafe is not allowed for containers of managed pointers");

        for (auto& e : buckets_)
            e.clear_unsafe();

        count_ = 0;
    }

    size_t size()      const { return count_; }
    bool   is_empty()  const { return count_ == 0; }

    // erase_if
    //
    // Find the first member of the hash table which satisfies the predicate
    // given by 'fn' and erase it from the list, returning a referenced pointer
    // to the removed element.  Return nullptr if no member satisfies the
    // predicate.
    template <typename UnaryFn>
    PtrType erase_if(UnaryFn fn) {
        if (is_empty())
            return PtrType(nullptr);

        for (HashType i = 0; i < kNumBuckets; ++i) {
            auto& bucket = buckets_[i];
            if (!bucket.is_empty()) {
                PtrType ret = bucket.erase_if(fn);
                if (ret != nullptr) {
                    --count_;
                    return ret;
                }
            }
        }

        return PtrType(nullptr);
    }

    // find_if
    //
    // Find the first member of the hash table which satisfies the predicate
    // given by 'fn' and return an iterator to it.  Return end() if no member
    // satisfies the predicate.
    template <typename UnaryFn>
    const_iterator find_if(UnaryFn fn) const {
        for (auto iter = begin(); iter.IsValid(); ++iter)
            if (fn(*iter))
                return iter;

        return end();
    }

    template <typename UnaryFn>
    iterator find_if(UnaryFn fn) {
        for (auto iter = begin(); iter.IsValid(); ++iter)
            if (fn(*iter))
                return iter;

        return end();
    }

private:
    // The traits of a non-const iterator
    struct iterator_traits {
        using RefType    = typename PtrTraits::RefType;
        using RawPtrType = typename PtrTraits::RawPtrType;
        using IterType   = typename BucketType::iterator;

        static IterType BucketBegin(BucketType& bucket) { return bucket.begin(); }
        static IterType BucketEnd  (BucketType& bucket) { return bucket.end(); }
    };

    // The traits of a const iterator
    struct const_iterator_traits {
        using RefType    = typename PtrTraits::ConstRefType;
        using RawPtrType = typename PtrTraits::ConstRawPtrType;
        using IterType   = typename BucketType::const_iterator;

        static IterType BucketBegin(const BucketType& bucket) { return bucket.cbegin(); }
        static IterType BucketEnd  (const BucketType& bucket) { return bucket.cend(); }
    };

    // The shared implementation of the iterator
    template <class IterTraits>
    class iterator_impl {
    public:
        iterator_impl() { }
        iterator_impl(const iterator_impl& other) {
            hash_table_ = other.hash_table_;
            bucket_ndx_ = other.bucket_ndx_;
            iter_       = other.iter_;
        }

        iterator_impl& operator=(const iterator_impl& other) {
            hash_table_ = other.hash_table_;
            bucket_ndx_ = other.bucket_ndx_;
            iter_       = other.iter_;
            return *this;
        }

        bool IsValid() const { return iter_.IsValid(); }
        bool operator==(const iterator_impl& other) const { return iter_ == other.iter_; }
        bool operator!=(const iterator_impl& other) const { return iter_ != other.iter_; }

        // Prefix
        iterator_impl& operator++() {
            if (!IsValid()) return *this;
            ZX_DEBUG_ASSERT(hash_table_);

            // Bump the bucket iterator and go looking for a new bucket if the
            // iterator has become invalid.
            ++iter_;
            advance_if_invalid_iter();

            return *this;
        }

        iterator_impl& operator--() {
            // If we have never been bound to a HashTable instance, the we had
            // better be invalid.
            if (!hash_table_) {
                ZX_DEBUG_ASSERT(!IsValid());
                return *this;
            }

            // Back up the bucket iterator.  If it is still valid, then we are done.
            --iter_;
            if (iter_.IsValid())
                return *this;

            // If the iterator is invalid after backing up, check previous
            // buckets to see if they contain any nodes.
            while (bucket_ndx_) {
                --bucket_ndx_;
                auto& bucket = GetBucket(bucket_ndx_);
                if (!bucket.is_empty()) {
                    iter_ = --IterTraits::BucketEnd(bucket);
                    ZX_DEBUG_ASSERT(iter_.IsValid());
                    return *this;
                }
            }

            // Looks like we have backed up past the beginning.  Update the
            // bookkeeping to point at the end of the last bucket.
            bucket_ndx_ = kNumBuckets - 1;
            iter_ = IterTraits::BucketEnd(GetBucket(bucket_ndx_));

            return *this;
        }

        // Postfix
        iterator_impl operator++(int) {
            iterator_impl ret(*this);
            ++(*this);
            return ret;
        }

        iterator_impl operator--(int) {
            iterator_impl ret(*this);
            --(*this);
            return ret;
        }

        typename PtrTraits::PtrType CopyPointer()    const { return iter_.CopyPointer(); }
        typename IterTraits::RefType operator*()     const { return iter_.operator*(); }
        typename IterTraits::RawPtrType operator->() const { return iter_.operator->(); }

    private:
        friend ContainerType;
        using IterType = typename IterTraits::IterType;

        enum BeginTag { BEGIN };
        enum EndTag { END };

        iterator_impl(const ContainerType* hash_table, BeginTag)
            : hash_table_(hash_table),
              bucket_ndx_(0),
              iter_(IterTraits::BucketBegin(GetBucket(0))) {
            advance_if_invalid_iter();
        }

        iterator_impl(const ContainerType* hash_table, EndTag)
            : hash_table_(hash_table),
              bucket_ndx_(kNumBuckets - 1),
              iter_(IterTraits::BucketEnd(GetBucket(kNumBuckets - 1))) { }

        iterator_impl(const ContainerType* hash_table, HashType bucket_ndx, const IterType& iter)
            : hash_table_(hash_table),
              bucket_ndx_(bucket_ndx),
              iter_(iter) { }

        BucketType& GetBucket(HashType ndx) {
            return const_cast<ContainerType*>(hash_table_)->buckets_[ndx];
        }

        void advance_if_invalid_iter() {
            // If the iterator has run off the end of it's current bucket, then
            // check to see if there are nodes in any of the remaining buckets.
            if (!iter_.IsValid()) {
                while (bucket_ndx_ < (kNumBuckets - 1)) {
                    ++bucket_ndx_;
                    auto& bucket = GetBucket(bucket_ndx_);

                    if (!bucket.is_empty()) {
                        iter_ = IterTraits::BucketBegin(bucket);
                        ZX_DEBUG_ASSERT(iter_.IsValid());
                        break;
                    } else if (bucket_ndx_ == (kNumBuckets - 1)) {
                        iter_ = IterTraits::BucketEnd(bucket);
                    }
                }
            }
        }

        const ContainerType* hash_table_ = nullptr;
        HashType bucket_ndx_ = 0;
        IterType iter_;
    };

    PtrType direct_erase(BucketType& bucket, ValueType& obj) {
        PtrType ret = internal::DirectEraseUtils<BucketType>::erase(bucket, obj);

        if (ret != nullptr)
            --count_;

        return ret;
    }

    static typename BucketType::iterator FindInBucket(BucketType& bucket,
                                                      const KeyType& key) {
        return bucket.find_if(
            [key](const ValueType& other) -> bool {
                return KeyTraits::EqualTo(key, KeyTraits::GetKey(other));
            });
    }

    static typename BucketType::const_iterator FindInBucket(const BucketType& bucket,
                                                            const KeyType& key) {
        return bucket.find_if(
            [key](const ValueType& other) -> bool {
                return KeyTraits::EqualTo(key, KeyTraits::GetKey(other));
            });
    }

    // The test framework's 'checker' class is our friend.
    friend CheckerType;

    // Iterators need to access our bucket array in order to iterate.
    friend iterator;
    friend const_iterator;

    // Hash tables may not currently be copied, assigned or moved.
    DISALLOW_COPY_ASSIGN_AND_MOVE(HashTable);

    BucketType& GetBucket(const KeyType& key) { return buckets_[GetHash(key)]; }
    BucketType& GetBucket(const ValueType& obj) { return GetBucket(KeyTraits::GetKey(obj)); }

    static HashType GetHash(const KeyType& obj) {
        HashType ret = HashTraits::GetHash(obj);
        ZX_DEBUG_ASSERT((ret >= 0) && (ret < kNumBuckets));
        return ret;
    }

    size_t count_ = 0UL;
    BucketType buckets_[kNumBuckets];
};

// Explicit declaration of constexpr storage.
#define HASH_TABLE_PROP(_type, _name) \
template <typename KeyType, typename PtrType, typename BucketType, typename HashType, \
          HashType NumBuckets, typename KeyTraits, typename HashTraits> \
constexpr _type HashTable<KeyType, PtrType, BucketType, HashType, \
                          NumBuckets, KeyTraits, HashTraits>::_name

HASH_TABLE_PROP(HashType, kNumBuckets);
HASH_TABLE_PROP(bool, SupportsConstantOrderErase);
HASH_TABLE_PROP(bool, SupportsConstantOrderSize);
HASH_TABLE_PROP(bool, IsAssociative);
HASH_TABLE_PROP(bool, IsSequenced);

#undef HASH_TABLE_PROP

}  // namespace fbl
