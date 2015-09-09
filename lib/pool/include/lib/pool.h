/**
 * A generic pool allocator.
 *
 * This is an efficient (constant-time allocation and freeing) allocator for objects of a fixed size
 * and alignment (typically, fixed type), based on a fixed-size object pool.
 *
 * The main API works with void* buffers and a couple of helper macros are used for a type-safe
 * (well, as far as C can go with type safety) "templatization" of the pool for a specific object
 * type.
 *
 * Typical usage:
 *
 * typedef struct {
 *   ...
 * } foo_t;
 *
 * // "Specialize" the pool for type foo_t.
 * #define FOO_POOL_STORAGE_SIZE(object_count) TYPED_POOL_STORAGE_SIZE(foo_t, object_count)
 * #define FOO_POOL_STORAGE_ALIGN() TYPED_POOL_STORAGE_ALIGN(foo_t)
 * #define DEFINE_FOO_POOL_STORAGE(name, count) DEFINE_TYPED_POOL_STORAGE(foo_t, name, count)
 * #define FOO_POOL_INIT(pool, count, storage) TYPED_POOL_INIT(foo_t, pool, count, storage)
 * #define FOO_POOL_ALLOC(pool) TYPED_POOL_ALLOC(foo_t, pool)
 * #define FOO_POOL_FREE(pool, object) TYPED_POOL_FREE(foo_t, pool, object)
 *
 * // Allocate storage for a pool of 100 objects.
 * DEFINE_FOO_POOL_STORAGE(foo_pool_storage, 100);
 *
 * // Alternatively, allocate storage some other way, using the FOO_POOL_STORAGE_SIZE() and
 * // FOO_POOL_STORAGE_ALIGN() to determine the required size and alignment of the storage buffer.
 *
 * // Instantiate the pool.
 * pool_t pool;
 * FOO_POOL_INIT(&pool, 100, &foo_pool_storage);
 *
 * // Allocate a foo_t from the pool.
 * foo_t * newfoo =  FOO_POOL_ALLOC(&pool);
 *
 * if (!newfoo) {
 *   // Handle allocation failure.
 * } else {
 *   // Use newfoo.
 *   ...
 *   // Free it.
 *   FOO_POOL_FREE(&pool, newfoo);
 * }
 */
#pragma once

#include <compiler.h>
#include <stddef.h>
#include <stdint.h>

__BEGIN_CDECLS

/**
 * Pool type.
 */
typedef struct {
    // Private:
    void * next_free;
} pool_t;

/**
 * Helper macro, not for public usage.
 */
#define _MAX(a,b) \
    ((a) > (b) ? (a) : (b))

/**
 * Helper macro, not for public usage.
 */
#define _PAD(size, align) \
    (((size) + (align) - 1) / (align) * (align))

/**
 * Calculates the required alignment for the pool storage given the size and alignment of the object
 * type.
 */
#define POOL_STORAGE_ALIGN(object_size, object_align) \
    (_MAX(__alignof(void *), object_align))

/**
 * Helper macro, not for public usage.
 */
#define POOL_PADDED_OBJECT_SIZE(object_size, object_align) \
    _PAD(_MAX(sizeof(void *), object_size), POOL_STORAGE_ALIGN(object_size, object_align))

/**
 * Calculates the size of the pool storage given the size and alignment of the object type and the
 * total number of objects in the pool.
 */
#define POOL_STORAGE_SIZE(object_size, object_align, object_count) \
    ((object_count) * POOL_PADDED_OBJECT_SIZE(object_size, object_align))

/**
 * Convenience macro for static allocation of pool storage.
 */
#define DEFINE_POOL_STORAGE(name, object_size, object_align, object_count) \
    uint8_t name[POOL_STORAGE_SIZE(object_size, object_align, object_count)] \
    __attribute__((aligned(POOL_STORAGE_ALIGN(object_size, object_align))))

/**
 * Initialize the pool object.
 * Provided storage must be aligned to POOL_STORAGE_ALIGN(object_size, object_align) and of size of
 * at least POOL_STORAGE_SIZE(object_size, object_align, object_count).
 * The DEFINE_POOL_STORAGE(...) makes this process simple for cases when the storage is to be
 * statically allocated.
 */
void pool_init(pool_t * pool,
               size_t object_size,
               size_t object_align,
               size_t object_count,
               void * storage);

/**
 * Allocate an object from the pool.
 * Returns NULL if all pool objects are currently allocated.
 * Otherwise, the return value is guarantee to be aligned at object_align and be at least of size
 * object_size.
 */
void * pool_alloc(pool_t * pool);

/**
 * Free an object previously allocated with pool_alloc.
 */
void pool_free(pool_t * pool, void * object);

/**
 * This set of macros help in "specializing" the pool API to a specific object type.
 * See example at the header of this file.
 */

#define TYPED_POOL_STORAGE_SIZE(type, object_count) \
    POOL_STORAGE_SIZE(sizeof(type), __alignof(type), object_count)

#define TYPED_POOL_STORAGE_ALIGN(type) \
    POOL_STORAGE_ALIGN(sizeof(type), __alignof(type))

#define DEFINE_TYPED_POOL_STORAGE(type, name, count) \
    DEFINE_POOL_STORAGE(name, sizeof(type), __alignof(type), count)

#define TYPED_POOL_INIT(type, pool, count, storage) \
    pool_init(pool, sizeof(type), __alignof(type), count, storage)

#define TYPED_POOL_ALLOC(type, pool) \
    ((type*) pool_alloc(pool))

#define TYPED_POOL_FREE(type, pool, object) \
    pool_free(pool, object)

__END_CDECLS
