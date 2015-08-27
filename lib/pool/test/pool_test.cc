#include <lib/pool.h>

#include <gtest/gtest.h>

#define DEFINE_DOUBLE_POOL_STORAGE(name, count) DEFINE_TYPED_POOL_STORAGE(double, name, count)
#define DOUBLE_POOL_INIT(pool, count, storage) TYPED_POOL_INIT(double, pool, count, storage)
#define DOUBLE_POOL_ALLOC(pool) TYPED_POOL_ALLOC(double, pool)
#define DOUBLE_POOL_FREE(pool, object) TYPED_POOL_FREE(double, pool, object)

TEST(Pool, Basic) {
    pool_t pool;
    DEFINE_DOUBLE_POOL_STORAGE(storage, 5);
    DOUBLE_POOL_INIT(&pool, 3, storage);

    // First 3 allocations should succeed.
    double * d0 = DOUBLE_POOL_ALLOC(&pool);
    EXPECT_NE(nullptr, d0);
    EXPECT_EQ(0, (intptr_t) d0 % __alignof(double));

    double * d1 = DOUBLE_POOL_ALLOC(&pool);
    EXPECT_NE(nullptr, d1);
    EXPECT_EQ(0, (intptr_t) d1 % __alignof(double));

    double * d2 = DOUBLE_POOL_ALLOC(&pool);
    EXPECT_NE(nullptr, d2);
    EXPECT_EQ(0, (intptr_t) d2 % __alignof(double));

    // All objects need to be different.
    EXPECT_NE(d1, d0);
    EXPECT_NE(d2, d0);
    EXPECT_NE(d2, d1);

    // Next allocation should fail.
    double * d3 = DOUBLE_POOL_ALLOC(&pool);
    EXPECT_EQ(nullptr, d3);

    // But after we free something it should succeed.
    DOUBLE_POOL_FREE(&pool, d0);
    d3 = DOUBLE_POOL_ALLOC(&pool);
    EXPECT_NE(nullptr, d3);
    EXPECT_EQ(0, (intptr_t) d3 % __alignof(double));
}
