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

BEGIN_TEST_CASE(bio_tests)
RUN_TEST(basic_read_write)
RUN_TEST(block_read_write)
RUN_TEST(unaligned_read_write)
RUN_TEST(offset_operations)
RUN_TEST(async_read_write)
RUN_TEST(large_transfer)
END_TEST_CASE(bio_tests)
