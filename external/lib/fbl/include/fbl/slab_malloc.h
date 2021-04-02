// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <stdlib.h>

namespace fbl {
namespace internal {

// TODO(johngro): Explore options for making this better.
//
// Making the kernel/user-mode decision here using an #ifdef and having two
// different implementations here feels a bit ugly.  Making the allocation
// behavior a template parameter is an option, but seems like a waste since
// user-mode will probably have no reason to allocate with anything other than
// malloc/free, while the kernel will *always* use the one kernel-approved
// method for allocating slabs.
//
// One option might be to push all the slab allocation code into a private file
// (slab_allocator_priv.h?) and have an outer include file available only to
// user mode which defines the SlabMalloc struct, and a different one in the
// kernel director which defines the kernel's SlabMalloc struct.
#ifdef _KERNEL

struct SlabMalloc {
    // TODO(johngro): Replace this implementation with a kernel implementation
    // which does not use the heap.
    static void* Allocate(size_t amt, size_t align) {
        void* mem = ::malloc(amt);
        ZX_DEBUG_ASSERT((reinterpret_cast<uintptr_t>(mem) % align) == 0);
        return mem;
    }

    static void Free(void* ptr) { ::free(ptr); }
};

#else

struct SlabMalloc {
    // TODO(johngro): Replace this implementation with a kernel implementation
    // which does not use the heap.
    static void* Allocate(size_t amt, size_t align) {
        return ::aligned_alloc(align, amt);
    }

    static void Free(void* ptr) { ::free(ptr); }
};

#endif

}  // namespace internal
}  // namespace fbl

