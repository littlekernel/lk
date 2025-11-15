/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/bio.h>
#include <lib/heap.h>
#include <lib/unittest.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <kernel/event.h>
#include <stdlib.h>
#include <string.h>

#define TEST_DEVICE_SIZE ((size_t)(64 * 1024))  // 64KB
#define BLOCK_SIZE ((size_t)512)

static bool basic_read_write(void) {
    BEGIN_TEST;

    // Allocate backing memory
    void *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");

    // Initialize with pattern
    memset(mem, 0xAA, TEST_DEVICE_SIZE);

    // Create memory block device
    EXPECT_EQ(0, create_membdev("test_bdev", mem, TEST_DEVICE_SIZE), "");

    // Open the device
    bdev_t *dev = bio_open("test_bdev");
    ASSERT_NONNULL(dev, "failed to open bio device");

    // Verify device properties
    EXPECT_EQ(TEST_DEVICE_SIZE, dev->total_size, "");
    EXPECT_EQ(BLOCK_SIZE, dev->block_size, "");
    EXPECT_EQ(TEST_DEVICE_SIZE / BLOCK_SIZE, dev->block_count, "");

    // Test simple read
    uint8_t buf[256];
    ssize_t result = bio_read(dev, buf, 0, sizeof(buf));
    EXPECT_EQ((ssize_t)sizeof(buf), result, "");
    for (size_t i = 0; i < sizeof(buf); i++) {
        EXPECT_EQ(0xAA, buf[i], "");
    }

    // Test simple write
    memset(buf, 0x55, sizeof(buf));
    result = bio_write(dev, buf, 0, sizeof(buf));
    EXPECT_EQ((ssize_t)sizeof(buf), result, "");

    // Read back and verify
    memset(buf, 0, sizeof(buf));
    result = bio_read(dev, buf, 0, sizeof(buf));
    EXPECT_EQ((ssize_t)sizeof(buf), result, "");
    for (size_t i = 0; i < sizeof(buf); i++) {
        EXPECT_EQ(0x55, buf[i], "");
    }

    bio_close(dev);
    bio_unregister_device(dev);
    free(mem);

    END_TEST;
}

static bool block_read_write(void) {
    BEGIN_TEST;

    // Allocate backing memory
    void *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");

    // Initialize with zeros
    memset(mem, 0, TEST_DEVICE_SIZE);

    // Create memory block device
    EXPECT_EQ(0, create_membdev("test_bdev_block", mem, TEST_DEVICE_SIZE), "");

    // Open the device
    bdev_t *dev = bio_open("test_bdev_block");
    ASSERT_NONNULL(dev, "failed to open bio device");

    // Allocate block-sized buffers
    uint8_t *write_buf = memalign(CACHE_LINE, BLOCK_SIZE * 4);
    uint8_t *read_buf = memalign(CACHE_LINE, BLOCK_SIZE * 4);
    ASSERT_NONNULL(write_buf, "");
    ASSERT_NONNULL(read_buf, "");

    // Fill write buffer with pattern
    for (size_t i = 0; i < BLOCK_SIZE * 4; i++) {
        write_buf[i] = i & 0xFF;
    }

    // Write 4 blocks starting at block 2
    ssize_t result = bio_write_block(dev, write_buf, 2, 4);
    EXPECT_EQ((ssize_t)(BLOCK_SIZE * 4), result, "");

    // Read back and verify
    memset(read_buf, 0, BLOCK_SIZE * 4);
    result = bio_read_block(dev, read_buf, 2, 4);
    EXPECT_EQ((ssize_t)(BLOCK_SIZE * 4), result, "");

    for (size_t i = 0; i < BLOCK_SIZE * 4; i++) {
        EXPECT_EQ(write_buf[i], read_buf[i], "");
    }

    // Verify blocks 0 and 1 are still zero
    memset(read_buf, 0xFF, BLOCK_SIZE * 2);
    result = bio_read_block(dev, read_buf, 0, 2);
    EXPECT_EQ((ssize_t)(BLOCK_SIZE * 2), result, "");
    for (size_t i = 0; i < BLOCK_SIZE * 2; i++) {
        EXPECT_EQ(0, read_buf[i], "");
    }

    free(write_buf);
    free(read_buf);
    bio_close(dev);
    bio_unregister_device(dev);
    free(mem);

    END_TEST;
}

static bool unaligned_read_write(void) {
    BEGIN_TEST;

    // Allocate backing memory
    void *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");

    // Initialize with zeros
    memset(mem, 0, TEST_DEVICE_SIZE);

    // Create memory block device
    EXPECT_EQ(0, create_membdev("test_bdev_unaligned", mem, TEST_DEVICE_SIZE), "");

    // Open the device
    bdev_t *dev = bio_open("test_bdev_unaligned");
    ASSERT_NONNULL(dev, "failed to open bio device");

    // Test unaligned write (offset not block-aligned)
    uint8_t pattern[1024];
    for (size_t i = 0; i < sizeof(pattern); i++) {
        pattern[i] = (i * 7) & 0xFF;
    }

    // Write at offset 100 (not block aligned)
    ssize_t result = bio_write(dev, pattern, 100, sizeof(pattern));
    EXPECT_EQ((ssize_t)sizeof(pattern), result, "");

    // Read back from same unaligned offset
    uint8_t read_buf[1024];
    memset(read_buf, 0, sizeof(read_buf));
    result = bio_read(dev, read_buf, 100, sizeof(read_buf));
    EXPECT_EQ((ssize_t)sizeof(read_buf), result, "");

    // Verify data matches
    for (size_t i = 0; i < sizeof(pattern); i++) {
        EXPECT_EQ(pattern[i], read_buf[i], "");
    }

    // Test partial block read at end of device
    off_t offset = TEST_DEVICE_SIZE - 256;
    uint8_t small_buf[256];
    memset(small_buf, 0xBB, sizeof(small_buf));
    result = bio_write(dev, small_buf, offset, sizeof(small_buf));
    EXPECT_EQ((ssize_t)sizeof(small_buf), result, "");

    memset(small_buf, 0, sizeof(small_buf));
    result = bio_read(dev, small_buf, offset, sizeof(small_buf));
    EXPECT_EQ((ssize_t)sizeof(small_buf), result, "");
    for (size_t i = 0; i < sizeof(small_buf); i++) {
        EXPECT_EQ(0xBB, small_buf[i], "");
    }

    bio_close(dev);
    bio_unregister_device(dev);
    free(mem);

    END_TEST;
}

static bool offset_operations(void) {
    BEGIN_TEST;

    // Allocate backing memory
    void *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");

    // Initialize each byte with its offset value (mod 256)
    for (size_t i = 0; i < TEST_DEVICE_SIZE; i++) {
        ((uint8_t *)mem)[i] = i & 0xFF;
    }

    // Create memory block device
    EXPECT_EQ(0, create_membdev("test_bdev_offset", mem, TEST_DEVICE_SIZE), "");

    // Open the device
    bdev_t *dev = bio_open("test_bdev_offset");
    ASSERT_NONNULL(dev, "failed to open bio device");

    // Test reads at various offsets
    uint8_t buf[128];

    // Read at offset 0
    ssize_t result = bio_read(dev, buf, 0, sizeof(buf));
    EXPECT_EQ((ssize_t)sizeof(buf), result, "");
    for (size_t i = 0; i < sizeof(buf); i++) {
        EXPECT_EQ(i & 0xFF, buf[i], "");
    }

    // Read at offset 1000
    result = bio_read(dev, buf, 1000, sizeof(buf));
    EXPECT_EQ((ssize_t)sizeof(buf), result, "");
    for (size_t i = 0; i < sizeof(buf); i++) {
        EXPECT_EQ((1000 + i) & 0xFF, buf[i], "");
    }

    // Read at offset 512 (block aligned)
    result = bio_read(dev, buf, 512, sizeof(buf));
    EXPECT_EQ((ssize_t)sizeof(buf), result, "");
    for (size_t i = 0; i < sizeof(buf); i++) {
        EXPECT_EQ((512 + i) & 0xFF, buf[i], "");
    }

    // Write at various offsets and verify
    uint8_t write_pattern[64];
    memset(write_pattern, 0xFF, sizeof(write_pattern));

    result = bio_write(dev, write_pattern, 2000, sizeof(write_pattern));
    EXPECT_EQ((ssize_t)sizeof(write_pattern), result, "");

    memset(buf, 0, sizeof(buf));
    result = bio_read(dev, buf, 2000, sizeof(write_pattern));
    EXPECT_EQ((ssize_t)sizeof(write_pattern), result, "");
    for (size_t i = 0; i < sizeof(write_pattern); i++) {
        EXPECT_EQ(0xFF, buf[i], "");
    }

    bio_close(dev);
    bio_unregister_device(dev);
    free(mem);

    END_TEST;
}

typedef struct {
    event_t event;
    ssize_t result;
} async_cookie_t;

static void async_callback(void *cookie, bdev_t *dev, ssize_t status) {
    async_cookie_t *async_cookie = (async_cookie_t *)cookie;
    async_cookie->result = status;
    event_signal(&async_cookie->event, false);
}

static bool async_read_write(void) {
    BEGIN_TEST;

    // Allocate backing memory
    void *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");

    // Initialize with zeros
    memset(mem, 0, TEST_DEVICE_SIZE);

    // Create memory block device
    EXPECT_EQ(0, create_membdev("test_bdev_async", mem, TEST_DEVICE_SIZE), "");

    // Open the device
    bdev_t *dev = bio_open("test_bdev_async");
    ASSERT_NONNULL(dev, "failed to open bio device");

    // Test async write
    uint8_t write_buf[512];
    for (size_t i = 0; i < sizeof(write_buf); i++) {
        write_buf[i] = (i ^ 0xAA) & 0xFF;
    }

    async_cookie_t write_cookie;
    event_init(&write_cookie.event, false, 0);
    write_cookie.result = -1;

    status_t status = bio_write_async(dev, write_buf, 1024, sizeof(write_buf),
                                      async_callback, &write_cookie);
    EXPECT_EQ(NO_ERROR, status, "");

    // Wait for the async operation to complete
    event_wait(&write_cookie.event);
    EXPECT_EQ((ssize_t)sizeof(write_buf), write_cookie.result, "");
    event_destroy(&write_cookie.event);

    // Test async read
    uint8_t read_buf[512];
    memset(read_buf, 0, sizeof(read_buf));

    async_cookie_t read_cookie;
    event_init(&read_cookie.event, false, 0);
    read_cookie.result = -1;

    status = bio_read_async(dev, read_buf, 1024, sizeof(read_buf),
                           async_callback, &read_cookie);
    EXPECT_EQ(NO_ERROR, status, "");

    // Wait for the async operation to complete
    event_wait(&read_cookie.event);
    EXPECT_EQ((ssize_t)sizeof(read_buf), read_cookie.result, "");
    event_destroy(&read_cookie.event);

    // Verify data
    for (size_t i = 0; i < sizeof(write_buf); i++) {
        EXPECT_EQ(write_buf[i], read_buf[i], "");
    }

    bio_close(dev);
    bio_unregister_device(dev);
    free(mem);

    END_TEST;
}

static bool large_transfer(void) {
    BEGIN_TEST;

    // Allocate backing memory
    void *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");

    // Initialize with zeros
    memset(mem, 0, TEST_DEVICE_SIZE);

    // Create memory block device
    EXPECT_EQ(0, create_membdev("test_bdev_large", mem, TEST_DEVICE_SIZE), "");

    // Open the device
    bdev_t *dev = bio_open("test_bdev_large");
    ASSERT_NONNULL(dev, "failed to open bio device");

    // Allocate large buffers for transfer
    size_t transfer_size = (size_t)(32 * 1024);  // 32KB
    uint8_t *large_write_buf = memalign(CACHE_LINE, transfer_size);
    uint8_t *large_read_buf = memalign(CACHE_LINE, transfer_size);
    ASSERT_NONNULL(large_write_buf, "");
    ASSERT_NONNULL(large_read_buf, "");

    // Fill with pattern
    for (size_t i = 0; i < transfer_size; i++) {
        large_write_buf[i] = (i * 3) & 0xFF;
    }

    // Write large chunk
    ssize_t result = bio_write(dev, large_write_buf, 0, transfer_size);
    EXPECT_EQ((ssize_t)transfer_size, result, "");

    // Read back
    memset(large_read_buf, 0, transfer_size);
    result = bio_read(dev, large_read_buf, 0, transfer_size);
    EXPECT_EQ((ssize_t)transfer_size, result, "");

    // Verify all data
    for (size_t i = 0; i < transfer_size; i++) {
        EXPECT_EQ(large_write_buf[i], large_read_buf[i], "");
    }

    free(large_write_buf);
    free(large_read_buf);
    bio_close(dev);
    bio_unregister_device(dev);
    free(mem);

    END_TEST;
}

static bool subdev_basic(void) {
    BEGIN_TEST;

    // Backing memory with known pattern
    uint8_t *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");
    for (size_t i = 0; i < TEST_DEVICE_SIZE; i++) {
        mem[i] = (uint8_t)(i & 0xFF);
    }

    // Create parent device and open it
    EXPECT_EQ(0, create_membdev("sub_parent", mem, TEST_DEVICE_SIZE), "");
    bdev_t *parent = bio_open("sub_parent");
    ASSERT_NONNULL(parent, "failed to open parent");

    // Publish subdevice starting at block 7, 30 blocks long
    const bnum_t startblock = 7;
    const bnum_t sub_blocks = 30;
    EXPECT_EQ(NO_ERROR, bio_publish_subdevice("sub_parent", "sub_dev", startblock, sub_blocks), "");

    // Open subdevice
    bdev_t *sub = bio_open("sub_dev");
    ASSERT_NONNULL(sub, "failed to open sub device");

    // Verify properties
    EXPECT_EQ(BLOCK_SIZE, sub->block_size, "");
    EXPECT_EQ(sub_blocks, sub->block_count, "");
    EXPECT_EQ((off_t)(sub_blocks * BLOCK_SIZE), sub->total_size, "");

    // Read from sub offset 0 and verify pattern
    uint8_t buf[128];
    ssize_t r = bio_read(sub, buf, 0, sizeof(buf));
    EXPECT_EQ((ssize_t)sizeof(buf), r, "");
    size_t base = (size_t)startblock * BLOCK_SIZE;
    for (size_t i = 0; i < sizeof(buf); i++) {
        EXPECT_EQ((uint8_t)((base + i) & 0xFF), buf[i], "");
    }

    // Read from an unaligned offset inside subdevice
    memset(buf, 0, sizeof(buf));
    r = bio_read(sub, buf, 600, sizeof(buf));
    EXPECT_EQ((ssize_t)sizeof(buf), r, "");
    for (size_t i = 0; i < sizeof(buf); i++) {
        EXPECT_EQ((uint8_t)((base + 600 + i) & 0xFF), buf[i], "");
    }

    bio_close(sub);
    bio_unregister_device(sub);
    bio_close(parent);
    bio_unregister_device(parent);
    free(mem);

    END_TEST;
}

static bool subdev_write_propagates(void) {
    BEGIN_TEST;

    // Backing memory zeros
    uint8_t *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");
    memset(mem, 0, TEST_DEVICE_SIZE);

    EXPECT_EQ(0, create_membdev("sub_parent2", mem, TEST_DEVICE_SIZE), "");
    bdev_t *parent = bio_open("sub_parent2");
    ASSERT_NONNULL(parent, "");

    const bnum_t startblock = 10;
    const bnum_t sub_blocks = 20;
    EXPECT_EQ(NO_ERROR, bio_publish_subdevice("sub_parent2", "sub_dev2", startblock, sub_blocks), "");
    bdev_t *sub = bio_open("sub_dev2");
    ASSERT_NONNULL(sub, "");

    uint8_t pattern[200];
    for (size_t i = 0; i < sizeof(pattern); i++) {
        pattern[i] = (uint8_t)((i * 5) & 0xFF);
    }

    // Write to subdevice at offset 123
    ssize_t w = bio_write(sub, pattern, 123, sizeof(pattern));
    EXPECT_EQ((ssize_t)sizeof(pattern), w, "");

    // Verify memory backing was updated at the correct base offset
    size_t base = (size_t)startblock * BLOCK_SIZE;
    for (size_t i = 0; i < sizeof(pattern); i++) {
        EXPECT_EQ(pattern[i], mem[base + 123 + i], "");
    }

    bio_close(sub);
    bio_unregister_device(sub);
    bio_close(parent);
    bio_unregister_device(parent);
    free(mem);

    END_TEST;
}

static bool subdev_block_ops(void) {
    BEGIN_TEST;

    // Backing memory zeros
    uint8_t *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "failed to allocate memory");
    memset(mem, 0, TEST_DEVICE_SIZE);

    EXPECT_EQ(0, create_membdev("sub_parent3", mem, TEST_DEVICE_SIZE), "");
    bdev_t *parent = bio_open("sub_parent3");
    ASSERT_NONNULL(parent, "");

    const bnum_t startblock = 3;
    const bnum_t sub_blocks = 16;
    EXPECT_EQ(NO_ERROR, bio_publish_subdevice("sub_parent3", "sub_dev3", startblock, sub_blocks), "");
    bdev_t *sub = bio_open("sub_dev3");
    ASSERT_NONNULL(sub, "");

    // Prepare 2 blocks of data
    const uint count = 2;
    uint8_t *wbuf = memalign(CACHE_LINE, BLOCK_SIZE * count);
    uint8_t *rbuf = memalign(CACHE_LINE, BLOCK_SIZE * count);
    ASSERT_NONNULL(wbuf, "");
    ASSERT_NONNULL(rbuf, "");
    for (size_t i = 0; i < BLOCK_SIZE * count; i++) {
        wbuf[i] = (uint8_t)((0xA0 + i) & 0xFF);
    }

    // Write to blocks 3..4 within the subdevice
    ssize_t w = bio_write_block(sub, wbuf, 3, count);
    EXPECT_EQ((ssize_t)(BLOCK_SIZE * count), w, "");

    // Verify in backing memory
    size_t base = (size_t)startblock * BLOCK_SIZE + 3 * BLOCK_SIZE;
    for (size_t i = 0; i < BLOCK_SIZE * count; i++) {
        EXPECT_EQ(wbuf[i], mem[base + i], "");
    }

    // Read back via block interface and verify
    memset(rbuf, 0, BLOCK_SIZE * count);
    ssize_t r = bio_read_block(sub, rbuf, 3, count);
    EXPECT_EQ((ssize_t)(BLOCK_SIZE * count), r, "");
    for (size_t i = 0; i < BLOCK_SIZE * count; i++) {
        EXPECT_EQ(wbuf[i], rbuf[i], "");
    }

    free(wbuf);
    free(rbuf);
    bio_close(sub);
    bio_unregister_device(sub);
    bio_close(parent);
    bio_unregister_device(parent);
    free(mem);

    END_TEST;
}

typedef struct {
    event_t event;
    ssize_t result;
} sub_async_cookie_t;

static void sub_async_cb(void *cookie, bdev_t *dev, ssize_t status) {
    sub_async_cookie_t *c = (sub_async_cookie_t *)cookie;
    c->result = status;
    event_signal(&c->event, false);
}

static bool subdev_async(void) {
    BEGIN_TEST;

    uint8_t *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "");
    memset(mem, 0, TEST_DEVICE_SIZE);

    EXPECT_EQ(0, create_membdev("sub_parent4", mem, TEST_DEVICE_SIZE), "");
    bdev_t *parent = bio_open("sub_parent4");
    ASSERT_NONNULL(parent, "");

    const bnum_t startblock = 5;
    const bnum_t sub_blocks = 64;
    EXPECT_EQ(NO_ERROR, bio_publish_subdevice("sub_parent4", "sub_dev4", startblock, sub_blocks), "");
    bdev_t *sub = bio_open("sub_dev4");
    ASSERT_NONNULL(sub, "");

    // Async write into subdevice
    uint8_t wbuf[512];
    for (size_t i = 0; i < sizeof(wbuf); i++) {
        wbuf[i] = (uint8_t)(i ^ 0x5A);
    }
    sub_async_cookie_t wcookie = {0};
    event_init(&wcookie.event, false, 0);
    status_t st = bio_write_async(sub, wbuf, 1024, sizeof(wbuf), sub_async_cb, &wcookie);
    EXPECT_EQ(NO_ERROR, st, "");
    event_wait(&wcookie.event);
    EXPECT_EQ((ssize_t)sizeof(wbuf), wcookie.result, "");
    event_destroy(&wcookie.event);

    // Async read back from subdevice
    uint8_t rbuf[512];
    memset(rbuf, 0, sizeof(rbuf));
    sub_async_cookie_t rcookie = {0};
    event_init(&rcookie.event, false, 0);
    st = bio_read_async(sub, rbuf, 1024, sizeof(rbuf), sub_async_cb, &rcookie);
    EXPECT_EQ(NO_ERROR, st, "");
    event_wait(&rcookie.event);
    EXPECT_EQ((ssize_t)sizeof(rbuf), rcookie.result, "");
    event_destroy(&rcookie.event);

    // Verify data matches
    for (size_t i = 0; i < sizeof(wbuf); i++) {
        EXPECT_EQ(wbuf[i], rbuf[i], "");
    }

    bio_close(sub);
    bio_unregister_device(sub);
    bio_close(parent);
    bio_unregister_device(parent);
    free(mem);

    END_TEST;
}

static bool subdev_nested(void) {
    BEGIN_TEST;

    uint8_t *mem = memalign(CACHE_LINE, TEST_DEVICE_SIZE);
    ASSERT_NONNULL(mem, "");
    memset(mem, 0, TEST_DEVICE_SIZE);

    EXPECT_EQ(0, create_membdev("sub_parent5", mem, TEST_DEVICE_SIZE), "");
    bdev_t *parent = bio_open("sub_parent5");
    ASSERT_NONNULL(parent, "");

    // sub1: offset 10 blocks
    EXPECT_EQ(NO_ERROR, bio_publish_subdevice("sub_parent5", "sub_lvl1", 10, 40), "");
    // sub2 of sub1: offset 5 blocks
    EXPECT_EQ(NO_ERROR, bio_publish_subdevice("sub_lvl1", "sub_lvl2", 5, 10), "");

    bdev_t *sub1 = bio_open("sub_lvl1");
    ASSERT_NONNULL(sub1, "");

    bdev_t *sub2 = bio_open("sub_lvl2");
    ASSERT_NONNULL(sub2, "");

    // Write into sub2 and verify absolute placement
    uint8_t data[256];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(0xC0 + (i & 0x3F));
    }
    EXPECT_EQ((ssize_t)sizeof(data), bio_write(sub2, data, 33, sizeof(data)), "");

    size_t base = (size_t)(10 + 5) * BLOCK_SIZE; // total block offset into parent
    for (size_t i = 0; i < sizeof(data); i++) {
        EXPECT_EQ(data[i], mem[base + 33 + i], "");
    }

    bio_close(sub2);
    bio_unregister_device(sub2);
    bio_close(sub1);
    bio_unregister_device(sub1);
    // Close/unregister parent last
    bio_close(parent);
    bio_unregister_device(parent);
    free(mem);

    END_TEST;
}

BEGIN_TEST_CASE(bio_tests)
RUN_TEST(basic_read_write)
RUN_TEST(block_read_write)
RUN_TEST(unaligned_read_write)
RUN_TEST(offset_operations)
RUN_TEST(async_read_write)
RUN_TEST(large_transfer)
RUN_TEST(subdev_basic)
RUN_TEST(subdev_write_propagates)
RUN_TEST(subdev_block_ops)
RUN_TEST(subdev_async)
RUN_TEST(subdev_nested)
END_TEST_CASE(bio_tests)
