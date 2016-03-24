/* Copyright (c) 2015 Ytai Ben-tsvi */

#include <lib/pool.h>
#include <assert.h>

void pool_init(pool_t *pool,
               size_t object_size,
               size_t object_align,
               size_t object_count,
               void *storage)
{
    assert(pool);
    assert(!object_count || storage);
    assert((intptr_t) storage % POOL_STORAGE_ALIGN(object_size, object_align) == 0);

    size_t offset = 0;
    for (size_t i = 0; i < object_count; ++i) {
        pool_free(pool, (uint8_t *) storage + offset);
        offset += POOL_PADDED_OBJECT_SIZE(object_size, object_align);
    }
}

void *pool_alloc(pool_t *pool)
{
    assert(pool);

    void *result = pool->next_free;
    if (!result) {
        return NULL;
    }
    pool->next_free = *((void **) result);
    return result;
}

void pool_free(pool_t *pool, void *object)
{
    assert(pool);
    assert(object);

    *((void **) object) = pool->next_free;
    pool->next_free = object;
}
