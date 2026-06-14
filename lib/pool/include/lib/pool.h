//
// Copyright (c) 2015 Ytai Ben-tsvi
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lk/compiler.h>
#include <stddef.h>
#include <stdlib.h>

// A generic pool allocator.
//
// This is an efficient (constant-time allocation and freeing) allocator for objects of a fixed size
// and alignment, based on a fixed-size object pool.
//
// Typical usage:
//
// // Calculate storage requirements for a pool of 100 objects, each 64 bytes, aligned to 64 bytes
// size_t object_count = 100;
// size_t object_size = 64;
// size_t object_align = 64;
// size_t storage_size = pool_storage_size(object_size, object_align, object_count);
// size_t storage_align = pool_storage_align(object_size, object_align);
//
// // Allocate properly aligned storage buffer
// // Or use static allocation with appropriate alignment
// void *storage = memalign(storage_align, storage_size);
//
// // Initialize the pool
// pool_t pool;
// pool_init(&pool, object_size, object_align, object_count, storage);
//
// // Allocate an object from the pool
// void *obj = pool_alloc(&pool);
// if (!obj) {
//   // Handle allocation failure (pool exhausted)
// } else {
//   // Use obj...
//   // Free it back to the pool
//   pool_free(&pool, obj);
// }
__BEGIN_CDECLS

// Pool type.
typedef struct {
    // Private:
    void *next_free;
} pool_t;

// Calculates the required alignment for the pool storage given the size and alignment of the object
// type.
static inline size_t pool_storage_align(size_t object_size, size_t object_align) {
    return MAX(__alignof(void *), object_align);
}

// The size of a pool object, including padding for alignment.
static inline size_t pool_padded_object_size(size_t object_size, size_t object_align) {

#define _PAD(size, align) (((size) + (align) - 1) / (align) * (align))

    size_t size = MAX(sizeof(void *), object_size);
    return _PAD(size, pool_storage_align(object_size, object_align));
#undef _PAD
}

// Calculates the size of the pool storage given the size and alignment of the object type and the
// total number of objects in the pool.
static inline size_t pool_storage_size(size_t object_size, size_t object_align,
                                       size_t object_count) {
    return (object_count)*pool_padded_object_size(object_size, object_align);
}

// Initialize the pool object.
// Provided storage must be aligned to pool_storage_align(object_size, object_align) and of size of
// at least POOL_STORAGE_SIZE(object_size, object_align, object_count).
// The DEFINE_POOL_STORAGE(...) makes this process simple for cases when the storage is to be
// statically allocated.
void pool_init(pool_t *pool, size_t object_size, size_t object_align, size_t object_count,
               void *storage);

// Allocate an object from the pool.
// Returns NULL if all pool objects are currently allocated.
// Otherwise, the return value is guarantee to be aligned at object_align and be at least of size
// object_size.
void *pool_alloc(pool_t *pool);

// Free an object previously allocated with pool_alloc.
void pool_free(pool_t *pool, void *object);

__END_CDECLS
