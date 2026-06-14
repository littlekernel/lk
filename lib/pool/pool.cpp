// Copyright (c) 2015 Ytai Ben-tsvi
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include "lib/pool.h"

#include <assert.h>

void pool_init(pool_t *pool, size_t object_size, size_t object_align, size_t object_count,
               void *storage) {
    assert(pool);
    assert(!object_count || storage);
    assert((intptr_t)storage % pool_storage_align(object_size, object_align) == 0);

    size_t offset = 0;
    for (size_t i = 0; i < object_count; ++i) {
        pool_free(pool, static_cast<uint8_t *>(storage) + offset);
        offset += pool_padded_object_size(object_size, object_align);
    }
}

void *pool_alloc(pool_t *pool) {
    assert(pool);

    void *result = pool->next_free;
    if (!result) {
        return NULL;
    }
    pool->next_free = *((void **)result);
    return result;
}

void pool_free(pool_t *pool, void *object) {
    assert(pool);
    assert(object);

    *(static_cast<void **>(object)) = pool->next_free;
    pool->next_free = object;
}
