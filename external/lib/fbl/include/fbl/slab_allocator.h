// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/compiler.h>
#include <fbl/algorithm.h>
#include <fbl/auto_lock.h>
#include <fbl/deleter.h>
#include <fbl/intrusive_single_list.h>
#include <fbl/mutex.h>
#include <fbl/new.h>
#include <fbl/null_lock.h>
#include <fbl/ref_ptr.h>
#include <fbl/slab_malloc.h>
#include <fbl/type_support.h>
#include <fbl/unique_ptr.h>

// Usage Notes:
//
// fbl::SlabAllocator<> is a utility class which implements a slab-style
// allocator for a given object type.  It can be used to dispense either managed
// or unmanaged pointer types.  Managed pointers automatically return to the
// allocator when they go completely out of scope, while unmanaged pointers must
// be manually returned to the allocator they came from.  Allocators may be
// "static" (meaning that there is only one allocator for the type for the
// entire process), or "instanced" meaning that there may be multiple instances
// of the allocator for the type, each with independent quotas.
//
// :: SlabAllocatorTraits<> ::
// The properties and behavior of a type of slab allocator is controlled using
// the SlabAllocatorTraits<> struct.  Things which can be controlled include...
//
// ++ The type of object and pointer created by the allocator.
// ++ The size of the slabs of memory which get allocated.
// ++ The synchronization primitive used to achieve thread safety.
// ++ The static/instanced/manual-delete nature of the allocator.
//
// Details on each of these items are included in the sections below.
//
// :: Memory limits and allocation behavior ::
//
// Slab allocators allocate large regions of memory (slabs) and carve them into
// properly aligned regions just large enough to hold an instance of the
// allocator's object type.  Internally, the allocator maintains a list of the
// slabs it has allocated as well as a free list of currently unused blocks of
// object memory.
//
// When allocations are performed...
// 1) Objects from the free list are used first.
// 2) If the free list is empty and the currently active slab has not been
//    completely used, a block of object memory is taken from the currently
//    active slab.
// 3) If the currently active slab has no more space, and the slab allocation
//    limit has not been reached, a new slab will be allocated using malloc and
//    single block of object memory will be carved out of it.
// 4) If all of the above fail, the allocation fails an nullptr is returned.
//
// Typically, allocation operations are O(1), but occasionally will be
// O(SlabMalloc::Allocate) if a new slab needs to be allocated.  Free operations
// are always O(1).
//
// Slab size is determined by the SLAB_SIZE parameter of the
// SlabAllocatorTraits<> struct and  defaults to 16KB.  The maximum number of
// slabs the allocator is allow to create is determined at construction time.
// Additionally, an optional bool (default == false) may be passed to the
// constructor telling it to attempt to allocate at least one slab up front.
// Setting the slab limit to 1 and pre-allocating that slab during construction
// will ensure O(1) for all allocations.
//
// :: Thread Safety ::
//
// By default, SlabAllocators use a fbl::Mutex is used internally to ensure
// that allocation and free operations are thread safe.  The only external
// function called while holding the internal mutex is ::malloc.
//
// This behavior may be changed by changing the LockType template parameter of
// the SlabAllocatorTraits<> struct to be the name of the class which will
// implement the locking behavior.  The class chosen must be compatible with
// fbl::AutoLock.  fbl::NullLock may be used if no locking is what is wanted.
// UnlockedInstancedSlabAllocatorTraits or UnlockedStaticSlabAllocatorTraits may
// be used as a shorthand for this.
//
// ** Example **
//
// using MyAllocatorTraits =
//     fbl::SlabAllocatorTraits<fbl::unique_ptr<MyObject>,
//                               fbl::DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
//                               fbl::NullLock,
//                               true>;
// fbl::SlabAllocator<MyAllocatorTraits> allocator;
//
// or...
//
// fbl::SlabAllocator<UnlockedStaticSlabAllocator<fbl::unique_ptr<MyObject>> allocator;
//
// :: Object Requirements ::
//
// Objects must be small enough that at least 1 can be allocated from a slab
// after taking alignment and internal slab bookkeeping into account.  If the
// object is too large (unlikely) the slab size must be increased.  This
// requirement is enforced with a static_assert, so any problems here should be
// caught at compile time.  MyAllocatorType::AllocsPerSlab is a constexpr which
// reports the number of allocations the compiler was able to carve out of each
// slab.
//
// All objects must derive from SlabAllocated<T> where T are the same
// SlabAllocatorTraits<> used to create the SlabAllocator itself.
//
// Deriving from SlabAllocated<> automatically provides the custom deletion
// behavior which allows the pointer to automatically return to the proper
// allocator when delete is called on the pointer (in the case of unmanaged
// pointers) or when the pointer goes completely out of scope (in the case of
// managed pointers).
//
// In the case of instanced slab allocators, the SlabAllocated<> class also
// provides storage for the pointer which will be used for the allocation to
// find its way back to its originating allocator.
//
// In the case of static slab allocators, the SlabAllocated<> class introduces
// no storage overhead to the object, it just supplies the type information
// needed for the object to automatically return to its allocator.
//
// In addition to instanced and static, there is a manual-delete flavor of slab
// allocator as well.  Manual-delete allocators are instanced (and have
// independent quotas), but objects allocated from manual-delete slab allocators
// do not pay the cost of a pointer to find their way back to the allocator they
// came from.  Instead, it is the user's responsibility to return the object to
// the allocator by calling allocator.Delete(obj_ptr) at the end of its life.
// Users are responsible for tracking which objects came from which allocator.
//
// :: Static Allocator Storage ::
//
// Static slab allocators require that the storage required for the allocator to
// function be declared instantiated in a translation unit somewhere in the
// program.  In addition, if the allocator is going to be used outside of just
// this one translation unit, the existance of the storage must be forward
// declared to all of the users of the allocator.
//
// Given the precondition...
//
//   class MyObject;
//   using SATraits = fbl::StaticAllocatorTraits<fbl::unique_ptr<MyObject>>;
//
// The formal syntax for forward declaring the existance of the allocator
// storage would be...
//
//   template<>
//   typename fbl::SlabAllocator<SATraits>::InternalAllocatorType
//   fbl::SlabAllocator<SATraits>::allocator_;
//
// And the formal syntax for instantiating the allocator storage would be...
//
//   template<>
//   typename fbl::SlabAllocator<SATraits>::InternalAllocatorType
//   fbl::SlabAllocator<SATraits>::allocator_(ctor_args...);
//
// To ease some of this pain, a two helper macro is provided.  In the header
// file in your program defines the allocator, you can write...
//
//   FWD_DECL_STATIC_SLAB_ALLOCATOR(SATraits);
//
// Then, in a .cpp file somewhere in your program, you can write...
//
//   DECLARE_STATIC_SLAB_ALLOCATOR_STORAGE(SATraits, ctor_args...);
//
// :: API ::
//
// The slab allocator API consists of 2 methods.
// ++ Ctor(size_t, bool)
// ++ New(...)
//
// The allocator constructor takes two arguments.  The first is the maximum
// number of slabs the allocator is permitted to allocate.  The second is a bool
// which specifies whether or not an attempt should be made to pre-allocate the
// first slab.  By default, this defaults to false.  As noted earlier, limiting
// the total number of slabs to 1 and pre-allocating the slab during
// construction guarantees O(1) allocations during operation.
//
// New(...) is used to construct and return a pointer (of designated type) to an
// object allocated from slab memory.  An appropriate form of nullptr will be
// returned if the allocator has reached its allocation limit.  New(...) will
// accept any set of parameters compatible with one of an object's constructors.
//
// ***********************
// ** Unmanaged Example **
// ***********************
//
// class MyObject : public fbl::SinglyLinkedListable<MyObject*> {
// public:
//     explicit MyObject(int val) : my_int_(val) { }
//     explicit MyObject(const char* val) : my_string_(val) { }
// private:
//     int my_int_ = 0;
//     const char* my_string_ = nullptr;
// };
//
// /* Make an instanced slab allocator with 4KB slabs which dispenses
//  * unmanaged pointers and uses no locks */
// using AllocatorTraits = fbl::UnlockedSlabAllocatorTraits<MyObject*, 4096u>;
// using AllocatorType   = fbl::SlabAllocator<AllocatorTraits>;
// using ObjListType     = fbl::SinglyLinkedList<MyObject*>;
//
// void my_function() {
//     AllocatorType allocator(1, true);   /* one pre-allocated slab */
//     ObjListType list;
//
//     /* Allocate a slab's worth of objects and put them on a list.  Use both
//      * constructors. */
//     for (size_t i = 0; i < AllocatorType::AllocsPerSlab; ++i) {
//         auto ptr = FlipACoin()
//                  ? allocator.New(5)                     /* int form */
//                  : allocator.New("this is a string");   /* string form */
//         list.push_front(ptr);
//     }
//
//     /* Do something with all of our objects */
//     for (auto& obj_ref : list)
//         DoSomething(obj_ref);
//
//     /* Return all of the objects to the allocator */
//     while(!list.is_empty())
//         delete list.pop_front();
// }
//
// ********************
// ** RefPtr Example **
// ********************
//
// /* Make a static slab allocator with default (16KB) sized slabs which
//  * dispenses RefPtr<>s and uses default (fbl::Mutex) locking.  Give the
//  * allocator permission to allocate up to 64 slabs, but do not attempt to
//  * pre-allocate the first.
//  */
// class MyObject;
// using AllocatorTraits = fbl::StaticSlabAllocatorTraits<fbl::RefPtr<MyObject>>;
// using AllocatorType   = fbl::SlabAllocator<AllocatorTraits>;
// using ObjListType     = fbl::SinglyLinkedList<fbl::RefPtr<MyObject>>;
//
// DECLARE_STATIC_SLAB_ALLOCATOR_STORAGE(AllocatorTraits, 64);
//
// class MyObject : public fbl::SlabAllocated<AllocatorTraits>,
//                  public fbl::RefCounted<MyObject>,
//                  public fbl::SinglyLinkedListable<fbl::RefCounted<MyObject>> {
// public:
//     explicit MyObject(int val) : my_int_(val) { }
//     explicit MyObject(const char* val) : my_string_(val) { }
// private:
//     int my_int_ = 0;
//     const char* my_string_ = nullptr;
// };
//
// void my_function() {
//     ObjListType list;
//
//     /* Allocate two slabs' worth of objects and put them on a list.  Use both
//      * constructors. */
//     for (size_t i = 0; i < (2 * AllocatorType::AllocsPerSlab); ++i) {
//         auto ptr = FlipACoin()
//                  ? AllocatorType::New(5)                     /* int form */
//                  : AllocatorType::New("this is a string");   /* string form */
//         list.push_front(ptr);
//     }
//
//     /* Do something with all of our objects */
//     for (auto& obj_ref : list)
//         DoSomething(obj_ref);
//
//     /* Clear the list and automatically return all of our objects */
//     list.clear();
// }
//
namespace fbl {

enum class SlabAllocatorFlavor {
    INSTANCED,
    STATIC,
    MANUAL_DELETE,
};

// fwd decls
template <typename T,
          size_t SLAB_SIZE,
          typename LockType,
          SlabAllocatorFlavor AllocatorFlavor,
          bool ENABLE_OBJ_COUNT>
struct SlabAllocatorTraits;
template <typename SATraits, typename = void> class SlabAllocator;
template <typename SATraits, typename = void> class SlabAllocated;

constexpr size_t DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE = (16 << 10u);

namespace internal {

template <bool>
class SAObjCounter;

template <>
class SAObjCounter<false> {
public:
    void Inc(void*) {}
    void Dec() {}
    void ResetMaxObjCount() {}
    size_t obj_count() const { return 0; }
    size_t max_obj_count() const { return 0; }
};

template <>
class SAObjCounter<true> {
public:
    void Inc(void* allocated_ptr) {
        if (allocated_ptr == nullptr) {
          return;
        }
        ++obj_count_;
        max_obj_count_ = max(obj_count_, max_obj_count_);
    }
    void Dec() { --obj_count_; }
    void ResetMaxObjCount() { max_obj_count_ = obj_count_; }
    size_t obj_count() const { return obj_count_; }
    size_t max_obj_count() const { return max_obj_count_; }

private:
    size_t obj_count_ = 0;
    size_t max_obj_count_ = 0;
};

// internal fwd-decls
template <typename T> struct SlabAllocatorPtrTraits;
template <typename SATraits> class SlabAllocator;

// Support for raw pointers
template <typename T>
struct SlabAllocatorPtrTraits<T*> {
    using ObjType = T;
    using PtrType = T*;

    static constexpr bool IsManaged = false;
    static constexpr PtrType CreatePtr(ObjType* ptr) { return ptr; }
};

// Support for unique_ptr
template <typename T>
struct SlabAllocatorPtrTraits<unique_ptr<T>> {
    using ObjType = T;
    using PtrType = unique_ptr<T>;

    static constexpr bool IsManaged = true;
    static constexpr PtrType CreatePtr(ObjType* ptr) { return PtrType(ptr); }
};

// Support for RefPtr
template <typename T>
struct SlabAllocatorPtrTraits<RefPtr<T>> {
    using ObjType = T;
    using PtrType = RefPtr<T>;

    static constexpr bool IsManaged = true;
    static constexpr PtrType CreatePtr(ObjType* ptr) { return AdoptRef<ObjType>(ptr); }
};

// Trait class used to set the origin of a slab allocated object, if needed.
template <typename SATraits, typename = void>
struct SlabOriginSetter {
    static inline void SetOrigin(typename SATraits::ObjType* ptr,
                                 internal::SlabAllocator<SATraits>* origin) {
        ZX_DEBUG_ASSERT((ptr != nullptr) && (origin != nullptr));
        ptr->slab_origin_ = origin;
    }
};

// Slab allocated objects from STATIC and MANUAL_DELETE slab allocators do not
// have (or need) a slab_origin.  Their "origin setter" is a no-op.
template <typename SATraits>
struct SlabOriginSetter<SATraits,
                        typename enable_if<
                            (SATraits::AllocatorFlavor == SlabAllocatorFlavor::STATIC) ||
                            (SATraits::AllocatorFlavor == SlabAllocatorFlavor::MANUAL_DELETE)
                        >::type> {

    static inline void SetOrigin(typename SATraits::ObjType* ptr,
                                 internal::SlabAllocator<SATraits>* origin) { }
};

// Non-templated SlabAllocatorBase.  Any code which does not strictly depend on
// trait/type awareness lives here in order to minimize code size explosion due
// to template expansion.
class SlabAllocatorBase {
protected:
    struct FreeListEntry : public SinglyLinkedListable<FreeListEntry*> { };

    struct Slab {
        explicit Slab(size_t initial_bytes_used) : bytes_used_(initial_bytes_used) { }

        void* Allocate(size_t alloc_size, size_t slab_storage_limit) {
            if ((bytes_used_ + alloc_size) > slab_storage_limit)
                return nullptr;

            void* ret = storage_ + bytes_used_;
            bytes_used_ += alloc_size;
            return ret;
        }

        SinglyLinkedListNodeState<Slab*> sll_node_state_;
        size_t                           bytes_used_;
        uint8_t                          storage_[];
    };

    static constexpr size_t SlabOverhead = offsetof(Slab, storage_);

public:
    DISALLOW_COPY_ASSIGN_AND_MOVE(SlabAllocatorBase);

    SlabAllocatorBase(size_t slab_size,
                      size_t alloc_size,
                      size_t alloc_alignment,
                      size_t initial_slab_used,
                      size_t max_slabs,
                      bool   alloc_initial)
        : slab_size_(slab_size),
          slab_alignment_(max(alignof(Slab), alloc_alignment)),
          slab_storage_limit_(slab_size - SlabOverhead + initial_slab_used),
          alloc_size_(alloc_size),
          initial_slab_used_(initial_slab_used),
          max_slabs_(max_slabs) {
        // Attempt to ensure that at least one slab has been allocated before
        // finishing construction if the user has asked us to do so.  In some
        // situations, this can help to ensure that allocation performance is
        // always O(1), provided that the slab limit has been configured to be
        // 1.
        if (alloc_initial) {
            // No need to take the lock here, no one can possible know about us
            // yet.
            void* first_alloc = AllocateLocked();
            if (first_alloc != nullptr)
                ReturnToFreeListLocked(first_alloc);
        }
    }

    ~SlabAllocatorBase() {
#if ZX_DEBUG_ASSERT_IMPLEMENTED
        size_t allocated_count = 0;
        size_t free_list_size = this->free_list_.size_slow();
#endif
        // null out the free list so that it does not assert that we left
        // unmanaged pointers on it as we destruct, and so that the free list
        // does not attempt to auto-destruct the managed objects which were
        // present on it after the slab memory has been freed
        this->free_list_.clear_unsafe();

        while (!slab_list_.is_empty()) {
            Slab* free_me = slab_list_.pop_front();
#if ZX_DEBUG_ASSERT_IMPLEMENTED
            size_t bytes_used = free_me->bytes_used_ - initial_slab_used_;
            ZX_DEBUG_ASSERT(free_me->bytes_used_ >= initial_slab_used_);
            ZX_DEBUG_ASSERT((bytes_used % alloc_size_) == 0);
            allocated_count += (bytes_used / alloc_size_);
#endif
            SlabMalloc::Free(reinterpret_cast<void*>(free_me));
        }

        // Make sure that everything which was ever allocated had been returned
        // to the free list before we were destroyed.
        ZX_DEBUG_ASSERT_COND(free_list_size == allocated_count);
    }

    size_t max_slabs() const { return max_slabs_; }
    size_t slab_count() const { return slab_count_; }

protected:
    void* AllocateLocked() {
        // If we can alloc from the free list, do so.
        if (!free_list_.is_empty()) {
            return free_list_.pop_front();
        }

        // If we can allocate from the currently active slab, do so.
        if (!slab_list_.is_empty()) {
            auto& active_slab = slab_list_.front();
            void* mem = active_slab.Allocate(alloc_size_, slab_storage_limit_);
            if (mem)
                return mem;
        }

        // If we are allowed to allocate new slabs, try to do so.
        if (slab_count_ < max_slabs_) {
            void* slab_mem = SlabMalloc::Allocate(slab_size_, slab_alignment_);
            if (slab_mem != nullptr) {
                Slab* slab = new (slab_mem) Slab(initial_slab_used_);

                slab_count_++;
                slab_list_.push_front(slab);

                return slab->Allocate(alloc_size_, slab_storage_limit_);
            }
        }

        // Looks like we have run out of resources.
        return nullptr;
    }

    void ReturnToFreeListLocked(void* ptr) {
        FreeListEntry* free_obj = new (ptr) FreeListEntry;
        free_list_.push_front(free_obj);
    }

private:
    // Constant properties of the allocator passed to us by our templated
    // wrapper during construction.
    const size_t slab_size_;
    const size_t slab_alignment_;
    const size_t slab_storage_limit_;
    const size_t alloc_size_;
    const size_t initial_slab_used_;
    const size_t max_slabs_;

    SinglyLinkedList<FreeListEntry*> free_list_;
    SinglyLinkedList<Slab*>          slab_list_;
    size_t                           slab_count_ = 0;
};

template <typename SATraits>
class SlabAllocator : public SlabAllocatorBase {
public:
    using PtrTraits = typename SATraits::PtrTraits;
    using PtrType   = typename SATraits::PtrType;
    using ObjType   = typename SATraits::ObjType;

protected:
    static constexpr size_t SLAB_SIZE  = SATraits::SLAB_SIZE;
    static constexpr size_t AllocSize  = max(sizeof(FreeListEntry), sizeof(ObjType));
    static constexpr size_t AllocAlign = max(alignof(FreeListEntry), alignof(ObjType));

    static_assert(AllocAlign > 0, "Alignment requirements cannot be zero!");
    static_assert(!(AllocSize % AllocAlign),
                  "Allocation size must be a multiple of allocation alignment!");

    static constexpr size_t SlabStorageMisalignment = SlabAllocatorBase::SlabOverhead % AllocAlign;
    static constexpr size_t InitialSlabUse = SlabStorageMisalignment
                                           ? AllocAlign - SlabStorageMisalignment
                                           : 0;
    static constexpr size_t TotalSlabOverhead = SlabAllocatorBase::SlabOverhead + InitialSlabUse;

    static_assert((sizeof(Slab) < SATraits::SLAB_SIZE) || (TotalSlabOverhead < SATraits::SLAB_SIZE),
                  "SLAB_SIZE too small to hold slab bookkeeping");

public:
    static constexpr size_t AllocsPerSlab = (SLAB_SIZE - TotalSlabOverhead) / AllocSize;

    static_assert(AllocsPerSlab > 0, "SLAB_SIZE too small to hold even 1 allocation");

    // Slab allocated objects must derive from SlabAllocated<SATraits>.
    static_assert(is_base_of<SlabAllocated<SATraits>, ObjType>::value,
                  "Objects which are slab allocated from an allocator of type "
                  "SlabAllocator<T> must derive from SlabAllocated<T>.");

    DISALLOW_COPY_ASSIGN_AND_MOVE(SlabAllocator);

    explicit SlabAllocator(size_t max_slabs, bool alloc_initial = false)
        : SlabAllocatorBase(SLAB_SIZE,
                            AllocSize,
                            AllocAlign,
                            InitialSlabUse,
                            max_slabs,
                            alloc_initial) { }

    ~SlabAllocator() { }

    template <typename... ConstructorSignature>
    PtrType New(ConstructorSignature&&... args) {
        void* mem = Allocate();

        if (mem == nullptr)
            return nullptr;

        // Construct the object
        //
        // Note: This rather odd forwarding of this construction operation to
        // the non-internal form of the slab allocator is deliberate.  This
        // prevents object with private constructors from needing to be friends
        // of a fbl::internal class (a class which they should not need to know
        // about).
        ObjType* obj = ::fbl::SlabAllocator<SATraits>::ConstructObject(
                mem,
                fbl::forward<ConstructorSignature>(args)...);

        // Now, record the slab allocator this object came from so it can be
        // returned later on.
        //
        // Note: This is a no-op in the case of an object which came from a
        // static slab allocator (who's road home is determined purely by type)
        SlabOriginSetter<SATraits>::SetOrigin(obj, this);

        return PtrTraits::CreatePtr(obj);
    }

    size_t obj_count() const {
        static_assert(SATraits::ENABLE_OBJ_COUNT,
                      "Error accessing obj_count: Object counter not enabled in SATraits.");
        return sa_obj_counter_.obj_count();
    }
    size_t max_obj_count() const {
        static_assert(SATraits::ENABLE_OBJ_COUNT,
                      "Error accessing max_obj_count: Object counter not enabled in SATraits.");
        return sa_obj_counter_.max_obj_count();
    }
    void ResetMaxObjCount() {
        static_assert(SATraits::ENABLE_OBJ_COUNT,
                      "Error performing ResetMaxObjCount: Object counter not enabled in SATraits.");
        AutoLock alloc_lock(&alloc_lock_);
        sa_obj_counter_.ResetMaxObjCount();
    }

protected:
    friend class ::fbl::SlabAllocator<SATraits>;
    friend class ::fbl::SlabAllocated<SATraits>;

    void* Allocate() {
        AutoLock alloc_lock(&this->alloc_lock_);
        void* ptr = AllocateLocked();
        sa_obj_counter_.Inc(ptr);
        return ptr;
    }

    void ReturnToFreeList(void* ptr) {
        FreeListEntry* free_obj = new (ptr) FreeListEntry;
        {
            AutoLock alloc_lock(&alloc_lock_);
            ReturnToFreeListLocked(free_obj);
            sa_obj_counter_.Dec();
        }
    }

    typename SATraits::LockType alloc_lock_;
    SAObjCounter<SATraits::ENABLE_OBJ_COUNT> sa_obj_counter_;
};
}  // namespace internal

////////////////////////////////////////////////////////////////////////////////
//
// Fundamental traits which control the properties of a slab allocator.
//
// Parameters:
// ++ T
//  The pointer type of the object to be created by the allocator.  Must be one
//  of the following...
//  ++ ObjectType*
//  ++ fbl::unique_ptr<ObjectType>
//  ++ fbl::RefPtr<ObjectType>
//
// ++ SLAB_SIZE
//  The size (in bytes) of an individual slab.  Defaults to 16KB
//
// ++ LockType
//  The fbl::AutoLock compatible class which will handle synchronization.
//
// ++ AllocatorType
//  Selects between a the three flavors of allocator.
//  ++ INSTANCED - Allocations come from an instance of an allocator.
//     Allocation objects carry the overhead of an "origin pointer" which will
//     be used to find their way home when the delete operator is applied to the
//     object.  Each instance of an allocator has it's own slab quota.
//  ++ STATIC - Allocations come from a static instance of an allocator.  There
//     is only one allocation pool for the entire process.  All allocator
//     methods are static members of the allocator's type and use the
//     MyAllocator::Method syntax (instead of my_allocator.Method).  Allocation
//     objects carry no overhead and will find their way home based on type when
//     the delete operator is applied to them.
//  ++ MANUAL_DELETE - A type of INSTANCED allocator where objects have no
//     pointer overhead.  The delete operator of the object is hidden in the
//     SlabAllocated<> class preventing users from delete'ing these objects.
//     Users must be aware of where their allocations came from and are
//     responsible for calling allocator.Delete in order to destruct and return
//     the object to the allocator it came from.  MANUAL_DELETE allocators are
//     only permitted for unmanaged pointer types.
//
////////////////////////////////////////////////////////////////////////////////
template <typename T,
          size_t   _SLAB_SIZE = DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
          typename _LockType  = ::fbl::Mutex,
          SlabAllocatorFlavor _AllocatorFlavor = SlabAllocatorFlavor::INSTANCED,
          bool _ENABLE_OBJ_COUNT = false>
struct SlabAllocatorTraits {
    using PtrTraits     = internal::SlabAllocatorPtrTraits<T>;
    using PtrType       = typename PtrTraits::PtrType;
    using ObjType       = typename PtrTraits::ObjType;
    using LockType      = _LockType;

    static constexpr size_t SLAB_SIZE = _SLAB_SIZE;
    static constexpr SlabAllocatorFlavor AllocatorFlavor = _AllocatorFlavor;
    static constexpr bool ENABLE_OBJ_COUNT = _ENABLE_OBJ_COUNT;
};

////////////////////////////////////////////////////////////////////////////////
//
// Implementation of an instanced slab allocator.
//
////////////////////////////////////////////////////////////////////////////////
template <typename SATraits>
class SlabAllocator<SATraits,
                    typename enable_if<
                        (SATraits::AllocatorFlavor == SlabAllocatorFlavor::INSTANCED) ||
                        (SATraits::AllocatorFlavor == SlabAllocatorFlavor::MANUAL_DELETE)
                    >::type>
      : public internal::SlabAllocator<SATraits> {
public:
    using PtrTraits         = typename SATraits::PtrTraits;
    using PtrType           = typename SATraits::PtrType;
    using ObjType           = typename SATraits::ObjType;
    using BaseAllocatorType = internal::SlabAllocator<SATraits>;

    static constexpr size_t AllocsPerSlab = BaseAllocatorType::AllocsPerSlab;

    explicit SlabAllocator(size_t max_slabs, bool alloc_initial = false)
        : BaseAllocatorType(max_slabs, alloc_initial) { }

    ~SlabAllocator() { }

    void Delete(ObjType* ptr) {
        static_assert(SATraits::AllocatorFlavor == SlabAllocatorFlavor::MANUAL_DELETE,
                      "Only MANUAL_DELETE slab allocators have a Delete method!");
        ptr->~ObjType();
        BaseAllocatorType::ReturnToFreeList(ptr);
    }

private:
    friend class internal::SlabAllocator<SATraits>; // internal::SA<> gets to call ConstructObject

    template <typename... ConstructorSignature>
    static ObjType* ConstructObject(void* mem, ConstructorSignature&&... args) {
        return new (mem) ObjType(fbl::forward<ConstructorSignature>(args)...);
    }
};

template <typename SATraits>
class SlabAllocated<SATraits,
                    typename enable_if<
                        (SATraits::AllocatorFlavor == SlabAllocatorFlavor::INSTANCED)
                    >::type> {
public:
    using AllocatorType = internal::SlabAllocator<SATraits>;
    using ObjType       = typename SATraits::ObjType;

     SlabAllocated() { }
    ~SlabAllocated() { }

    DISALLOW_COPY_ASSIGN_AND_MOVE(SlabAllocated);

    void operator delete(void* ptr) {
        // Note: this is a bit sketchy...  We have been destructed at this point
        // in time, but we are about to access our slab_origin_ member variable.
        // The *only* reason that this is OK is that we know that our destructor
        // does not touch slab_origin_, and no one else in our hierarchy should
        // be able to modify slab_origin_ because it is private.
        ObjType* obj_ptr = reinterpret_cast<ObjType*>(ptr);

        ZX_DEBUG_ASSERT(obj_ptr != nullptr);
        ZX_DEBUG_ASSERT(obj_ptr->slab_origin_ != nullptr);
        obj_ptr->slab_origin_->ReturnToFreeList(obj_ptr);
    }

private:
    friend struct internal::SlabOriginSetter<SATraits>;
    AllocatorType* slab_origin_ = nullptr;
};

template <typename SATraits>
class SlabAllocated<SATraits,
                    typename enable_if<
                        (SATraits::PtrTraits::IsManaged == false) &&
                        (SATraits::AllocatorFlavor == SlabAllocatorFlavor::MANUAL_DELETE)
                    >::type> {
public:
     SlabAllocated() { }
    ~SlabAllocated() { }

    DISALLOW_COPY_ASSIGN_AND_MOVE(SlabAllocated);

protected:
    // Object which come from a MANUAL_DELETE slab allocator may not be
    // destroyed using the delete operator.  Instead, users must return the
    // object to its allocator using the Delete method of the allocator
    // instance.
    //
    // Hide the delete operator, and halt-and-catch-fire if some Bad Person ever
    // manages to generate a call to this operator.
    //
    // Note: it would be nice to either = delete this operator, or at least make
    // it private, but we cannot.  To do so would prevent the implemementer of
    // the slab allocated object from defining a destructor.
    void operator delete(void*) { ZX_DEBUG_ASSERT(false); }
};

// Shorthand for declaring the properties of an instanced allocator (somewhat
// superfluous as the default is instanced)
template <typename T,
          size_t   SLAB_SIZE = DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
          typename LockType  = ::fbl::Mutex,
          bool     ENABLE_OBJ_COUNT = false>
using InstancedSlabAllocatorTraits =
    SlabAllocatorTraits<T, SLAB_SIZE, LockType, SlabAllocatorFlavor::INSTANCED, ENABLE_OBJ_COUNT>;

template <typename T,
          size_t   SLAB_SIZE = DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
          bool     ENABLE_OBJ_COUNT = false>
using UnlockedInstancedSlabAllocatorTraits =
    SlabAllocatorTraits<T, SLAB_SIZE, ::fbl::NullLock, SlabAllocatorFlavor::INSTANCED,
                        ENABLE_OBJ_COUNT>;

template <typename T,
          size_t   SLAB_SIZE = DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
          bool     ENABLE_OBJ_COUNT = false>
using UnlockedSlabAllocatorTraits =
    SlabAllocatorTraits<T, SLAB_SIZE, ::fbl::NullLock, SlabAllocatorFlavor::INSTANCED,
                        ENABLE_OBJ_COUNT>;

// Shorthand for declaring the properties of a MANUAL_DELETE slab allocator.
template <typename T,
          size_t   SLAB_SIZE = DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
          typename LockType  = ::fbl::Mutex,
          bool     ENABLE_OBJ_COUNT = false>
using ManualDeleteSlabAllocatorTraits =
    SlabAllocatorTraits<T, SLAB_SIZE, LockType, SlabAllocatorFlavor::MANUAL_DELETE,
                        ENABLE_OBJ_COUNT>;

template <typename T,
          size_t   SLAB_SIZE = DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
          bool     ENABLE_OBJ_COUNT = false>
using UnlockedManualDeleteSlabAllocatorTraits =
    SlabAllocatorTraits<T, SLAB_SIZE, ::fbl::NullLock, SlabAllocatorFlavor::MANUAL_DELETE,
                        ENABLE_OBJ_COUNT>;

////////////////////////////////////////////////////////////////////////////////
//
// Implementation of a static slab allocator.
//
////////////////////////////////////////////////////////////////////////////////
template <typename SATraits>
class SlabAllocator<SATraits,
                    typename enable_if<
                        (SATraits::AllocatorFlavor == SlabAllocatorFlavor::STATIC)
                    >::type> {
public:
    using PtrTraits             = typename SATraits::PtrTraits;
    using PtrType               = typename SATraits::PtrType;
    using ObjType               = typename SATraits::ObjType;
    using InternalAllocatorType = internal::SlabAllocator<SATraits>;

    static constexpr size_t AllocsPerSlab = InternalAllocatorType::AllocsPerSlab;

    // Do not allow instantiation of static slab allocators.
    SlabAllocator() = delete;

    template <typename... ConstructorSignature>
    static PtrType New(ConstructorSignature&&... args) {
        return allocator_.New(fbl::forward<ConstructorSignature>(args)...);
    }

    static size_t max_slabs() { return allocator_.max_slabs(); }
    static size_t obj_count() { return allocator_.obj_count(); }
    static size_t max_obj_count() { return allocator_.max_obj_count(); }
    static size_t slab_count() { return allocator_.slab_count(); }
    static void ResetMaxObjCount() { allocator_.ResetMaxObjCount(); }

private:
    friend class SlabAllocated<SATraits>;           // SlabAllocated<> gets to call ReturnToFreeList
    friend class internal::SlabAllocator<SATraits>; // internal::SA<> gets to call ConstructObject

    template <typename... ConstructorSignature>
    static ObjType* ConstructObject(void* mem, ConstructorSignature&&... args) {
        return new (mem) ObjType(fbl::forward<ConstructorSignature>(args)...);
    }

    static void ReturnToFreeList(void* ptr) { allocator_.ReturnToFreeList(ptr); }
    static InternalAllocatorType allocator_;
};

template <typename SATraits>
class SlabAllocated<SATraits,
                    typename enable_if<
                        (SATraits::AllocatorFlavor == SlabAllocatorFlavor::STATIC)
                    >::type> {
public:
    SlabAllocated() { }
    DISALLOW_COPY_ASSIGN_AND_MOVE(SlabAllocated);

    using AllocatorType = SlabAllocator<SATraits>;
    using ObjType       = typename SATraits::ObjType;

    void operator delete(void* ptr) {
        ZX_DEBUG_ASSERT(ptr != nullptr);
        AllocatorType::ReturnToFreeList(reinterpret_cast<ObjType*>(ptr));
    }
};

// Shorthand for declaring the properties of a static allocator
template <typename T,
          size_t   SLAB_SIZE = DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
          typename LockType  = ::fbl::Mutex,
          bool     ENABLE_OBJ_COUNT = false>
using StaticSlabAllocatorTraits =
    SlabAllocatorTraits<T, SLAB_SIZE, LockType, SlabAllocatorFlavor::STATIC, ENABLE_OBJ_COUNT>;

template <typename T,
          size_t   SLAB_SIZE = DEFAULT_SLAB_ALLOCATOR_SLAB_SIZE,
          bool     ENABLE_OBJ_COUNT = false>
using UnlockedStaticSlabAllocatorTraits =
    SlabAllocatorTraits<T, SLAB_SIZE, ::fbl::NullLock, SlabAllocatorFlavor::STATIC,
                        ENABLE_OBJ_COUNT>;

// Shorthand for declaring the global storage required for a static allocator
#define DECLARE_STATIC_SLAB_ALLOCATOR_STORAGE(ALLOC_TRAITS, ...) \
template<> ::fbl::SlabAllocator<ALLOC_TRAITS>::InternalAllocatorType \
fbl::SlabAllocator<ALLOC_TRAITS>::allocator_(__VA_ARGS__)

// Shorthand for forward declaring the existance of the storage required to use
// a static slab allocator.  Use this macro in your header file if your static
// slab allocator is to be used outside of a single translational unit.
#define FWD_DECL_STATIC_SLAB_ALLOCATOR(ALLOC_TRAITS) \
template<> ::fbl::SlabAllocator<ALLOC_TRAITS>::InternalAllocatorType \
fbl::SlabAllocator<ALLOC_TRAITS>::allocator_;


}  // namespace fbl
