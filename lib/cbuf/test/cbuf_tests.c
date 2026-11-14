/*
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kernel/thread.h>
#include <lib/cbuf.h>
#include <lib/heap.h>
#include <lib/unittest.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <rand.h>
#include <stdlib.h>

static bool basic(void) {
    BEGIN_TEST;
    cbuf_t cbuf;

    cbuf_initialize(&cbuf, 16);

    EXPECT_EQ(15UL, cbuf_space_avail(&cbuf), "");
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");

    EXPECT_EQ(8UL, cbuf_write(&cbuf, "abcdefgh", 8, false), "");

    EXPECT_EQ(7UL, cbuf_space_avail(&cbuf), "");
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");

    // Only 7 bytes should fit since if we write all 16 bytes,
    // head == tail and we can't distinguish it from the start case.
    EXPECT_EQ(7UL, cbuf_write(&cbuf, "ijklmnop", 8, false), "");

    EXPECT_EQ(0UL, cbuf_space_avail(&cbuf), "");
    EXPECT_TRUE(cbuf_is_full(&cbuf), "");

    // Nothing should fit.
    EXPECT_EQ(0UL, cbuf_write(&cbuf, "XXXXXXXX", 8, false), "");

    EXPECT_EQ(0UL, cbuf_space_avail(&cbuf), "");
    EXPECT_TRUE(cbuf_is_full(&cbuf), "");

    // Read a few bytes.
    {
        char buf[32];
        EXPECT_EQ(3UL, cbuf_read(&cbuf, buf, 3, false), "");
        EXPECT_FALSE(cbuf_is_full(&cbuf), "");
        for (int i = 0; i < 3; ++i) {
            EXPECT_EQ(buf[i], 'a' + i, "");
        }

        // Try reading 32 bytes.
        EXPECT_EQ(12UL, cbuf_read(&cbuf, buf, 32, false), "");
        EXPECT_FALSE(cbuf_is_full(&cbuf), "");
        for (int i = 0; i < 12; ++i) {
            EXPECT_EQ(buf[i], 'd' + i, "");
        }
    }

    cbuf_reset(&cbuf);

    EXPECT_EQ(15UL, cbuf_space_avail(&cbuf), "");
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");

    END_TEST;
}

bool random(void) {
    BEGIN_TEST;

    cbuf_t cbuf;
    cbuf_initialize(&cbuf, 32);

    // Random tests. Keep writing in random chunks up to 8 bytes, then
    // reading in chunks up to 8 bytes. Verify values.

    size_t pos_out = 0;
    size_t pos_in = 0;
    while (pos_in < 256) {
        if (pos_out < 256) {
            // Write up to 8 bytes.
            unsigned char buf_out[8];
            size_t to_write_random = rand() & 7;
            size_t to_write = MIN(to_write_random, 256 - pos_out);
            for (size_t i = 0; i < to_write; ++i) {
                buf_out[i] = pos_out + i;
            }
            // Advance the out pointer based on how many bytes fit.
            size_t wrote = cbuf_write(&cbuf, buf_out, to_write, false);
            EXPECT_LE(wrote, to_write, "");
            pos_out += wrote;
        }

        // Read up to 8 bytes, make sure they are right.
        if (pos_in < pos_out) {
            unsigned char buf_in[8];
            size_t to_read_random = rand() & 7;
            size_t to_read = MIN(to_read_random, pos_out - pos_in);
            size_t read = cbuf_read(&cbuf, buf_in, to_read, false);
            EXPECT_LE(read, to_read, "");

            for (size_t i = 0; i < read; ++i) {
                EXPECT_EQ(pos_in + i, buf_in[i], "");
            }

            pos_in += read;
        }

        EXPECT_LE(pos_in, pos_out, "");
    }

    free(cbuf.buf);

    END_TEST;
}

static bool is_full_edge_cases(void) {
    BEGIN_TEST;
    cbuf_t cbuf;

    cbuf_initialize(&cbuf, 8);

    // Empty buffer should not be full
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");
    EXPECT_EQ(7UL, cbuf_space_avail(&cbuf), "");

    // Write 1 byte at a time until full
    for (int i = 0; i < 7; ++i) {
        EXPECT_FALSE(cbuf_is_full(&cbuf), "");
        char c = 'A' + i;
        EXPECT_EQ(1UL, cbuf_write_char(&cbuf, c, false), "");
    }

    // Now should be full
    EXPECT_TRUE(cbuf_is_full(&cbuf), "");
    EXPECT_EQ(0UL, cbuf_space_avail(&cbuf), "");

    // Try to write one more byte - should fail
    EXPECT_EQ(0UL, cbuf_write_char(&cbuf, 'X', false), "");
    EXPECT_TRUE(cbuf_is_full(&cbuf), "");

    // Read one byte - should no longer be full
    char c;
    EXPECT_EQ(1UL, cbuf_read_char(&cbuf, &c, false), "");
    EXPECT_EQ('A', c, "");
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");

    // Write one byte - should be full again
    EXPECT_EQ(1UL, cbuf_write_char(&cbuf, 'X', false), "");
    EXPECT_TRUE(cbuf_is_full(&cbuf), "");

    // Read all remaining bytes
    char buf[8];
    EXPECT_EQ(7UL, cbuf_read(&cbuf, buf, 8, false), "");
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");
    EXPECT_EQ(7UL, cbuf_space_avail(&cbuf), "");

    free(cbuf.buf);

    END_TEST;
}

static bool is_full_wraparound(void) {
    BEGIN_TEST;
    cbuf_t cbuf;

    cbuf_initialize(&cbuf, 16);

    // Fill partially
    EXPECT_EQ(10UL, cbuf_write(&cbuf, "0123456789", 10, false), "");
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");

    // Read some, causing wraparound opportunity
    char buf[16];
    EXPECT_EQ(5UL, cbuf_read(&cbuf, buf, 5, false), "");
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");

    // Write more - this should wrap around
    EXPECT_EQ(5UL, cbuf_write(&cbuf, "ABCDE", 5, false), "");
    EXPECT_FALSE(cbuf_is_full(&cbuf), "");

    // Fill to capacity (5 more bytes available)
    EXPECT_EQ(5UL, cbuf_write(&cbuf, "FGHIJ", 5, false), "");
    EXPECT_TRUE(cbuf_is_full(&cbuf), "");

    // Verify can't write more
    EXPECT_EQ(0UL, cbuf_write_char(&cbuf, 'X', false), "");
    EXPECT_TRUE(cbuf_is_full(&cbuf), "");

    free(cbuf.buf);

    END_TEST;
}

static bool peek_tests(void) {
    BEGIN_TEST;
    cbuf_t cbuf;

    cbuf_initialize(&cbuf, 16);

    // Initial peek of empty buffer should return 0
    iovec_t regions[2];
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(0UL, cbuf_peek(&cbuf, regions), "");
    EXPECT_EQ(NULL, regions[0].iov_base, "");
    EXPECT_EQ(0UL, regions[0].iov_len, "");
    EXPECT_EQ(NULL, regions[1].iov_base, "");
    EXPECT_EQ(0UL, regions[1].iov_len, "");

    // Write some data, no wraparound
    EXPECT_EQ(8UL, cbuf_write(&cbuf, "01234567", 8, false), "");

    // cbuf_peek should return 8, with regions[0] having length 8 and regions[1] being empty
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(8UL, cbuf_peek(&cbuf, regions), "");
    EXPECT_NE(NULL, regions[0].iov_base, "");
    EXPECT_EQ(8UL, regions[0].iov_len, "");
    EXPECT_EQ(0, memcmp(regions[0].iov_base, "01234567", 8), "");
    EXPECT_EQ(NULL, regions[1].iov_base, "");
    EXPECT_EQ(0UL, regions[1].iov_len, "");

    // cbuf_peek_at inside the data range
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(5UL, cbuf_peek_at(&cbuf, 3, 5, regions), "");
    EXPECT_NE(NULL, regions[0].iov_base, "");
    EXPECT_EQ(5UL, regions[0].iov_len, "");
    EXPECT_EQ(0, memcmp(regions[0].iov_base, "34567", 5), "");
    EXPECT_EQ(NULL, regions[1].iov_base, "");
    EXPECT_EQ(0UL, regions[1].iov_len, "");

    // cbuf_peek_at partially out of range
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(5UL, cbuf_peek_at(&cbuf, 3, 10, regions), "");
    EXPECT_NE(NULL, regions[0].iov_base, "");
    EXPECT_EQ(5UL, regions[0].iov_len, "");
    EXPECT_EQ(0, memcmp(regions[0].iov_base, "34567", 5), "");
    EXPECT_EQ(NULL, regions[1].iov_base, "");
    EXPECT_EQ(0UL, regions[1].iov_len, "");

    // cbuf_peek_at completely out of range
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(0UL, cbuf_peek_at(&cbuf, 8, 5, regions), "");
    EXPECT_EQ(NULL, regions[0].iov_base, "");
    EXPECT_EQ(0UL, regions[0].iov_len, "");
    EXPECT_EQ(NULL, regions[1].iov_base, "");
    EXPECT_EQ(0UL, regions[1].iov_len, "");

    // cbuf_peek_at way out of range
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(0UL, cbuf_peek_at(&cbuf, 100, 5, regions), "");
    EXPECT_EQ(NULL, regions[0].iov_base, "");
    EXPECT_EQ(0UL, regions[0].iov_len, "");
    EXPECT_EQ(NULL, regions[1].iov_base, "");
    EXPECT_EQ(0UL, regions[1].iov_len, "");

    // Now consume some data to move tail, then write enough to cause wraparound
    char dummy[4];
    EXPECT_EQ(4UL, cbuf_read(&cbuf, dummy, 4, false), "");

    // Write 8 more bytes to wrap around
    EXPECT_EQ(8UL, cbuf_write(&cbuf, "89ABCDEF", 8, false), "");

    // Write 1 more byte to cause non-zero regions[1] length
    EXPECT_EQ(1UL, cbuf_write(&cbuf, "G", 1, false), "");

    // cbuf_peek should return 13, regions[0] len 12, regions[1] len 1.
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(13UL, cbuf_peek(&cbuf, regions), "");
    EXPECT_NE(NULL, regions[0].iov_base, "");
    EXPECT_EQ(12UL, regions[0].iov_len, "");
    EXPECT_EQ(0, memcmp(regions[0].iov_base, "456789ABCDEF", 12), "");
    EXPECT_NE(NULL, regions[1].iov_base, "");
    EXPECT_EQ(1UL, regions[1].iov_len, "");
    EXPECT_EQ(0, memcmp(regions[1].iov_base, "G", 1), "");

    // cbuf_peek_at with offset that doesn't wrap
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(4UL, cbuf_peek_at(&cbuf, 2, 4, regions), "");
    EXPECT_NE(NULL, regions[0].iov_base, "");
    EXPECT_EQ(4UL, regions[0].iov_len, "");
    EXPECT_EQ(0, memcmp(regions[0].iov_base, "6789", 4), "");
    EXPECT_EQ(NULL, regions[1].iov_base, "");
    EXPECT_EQ(0UL, regions[1].iov_len, "");

    // cbuf_peek_at with offset that wraps
    memset(regions, 0x55, sizeof(regions));
    EXPECT_EQ(3UL, cbuf_peek_at(&cbuf, 10, 3, regions), "");
    EXPECT_NE(NULL, regions[0].iov_base, "");
    EXPECT_EQ(2UL, regions[0].iov_len, "");
    EXPECT_EQ(0, memcmp(regions[0].iov_base, "EF", 2), "");
    EXPECT_NE(NULL, regions[1].iov_base, "");
    EXPECT_EQ(1UL, regions[1].iov_len, "");
    EXPECT_EQ(0, memcmp(regions[1].iov_base, "G", 1), "");

    free(cbuf.buf);
    END_TEST;
}

// cbuf_read_etc with a blocking read, each run by a helper thread so that only
// a throwaway thread is ever killed (a kill signal is never cleared).
static cbuf_t etc_cbuf;
static uint8_t etc_cbuf_buf[16];

static int read_interruptible(void *arg) {
    char buf[4];
    return (int)cbuf_read_etc(&etc_cbuf, buf, sizeof(buf), true, WAIT_QUEUE_INTERRUPTIBLE);
}

static int read_uninterruptible_after_kill(void *arg) {
    thread_signal_kill(get_current_thread());
    char buf[4];
    return (int)cbuf_read_etc(&etc_cbuf, buf, sizeof(buf), true, 0);
}

static bool helper_blocked(thread_t *t) {
    for (int i = 0; i < 1000; i++) {
        THREAD_LOCK(state);
        const bool blocked = t->state == THREAD_BLOCKED;
        THREAD_UNLOCK(state);
        if (blocked) {
            return true;
        }
        thread_sleep(1);
    }
    return false;
}

static bool read_etc_tests(void) {
    BEGIN_TEST;

    cbuf_initialize_etc(&etc_cbuf, sizeof(etc_cbuf_buf), etc_cbuf_buf);

    // a kill ends a blocked interruptible read
    thread_t *t = thread_create("cbuf kill", read_interruptible, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "");
    thread_resume(t);
    ASSERT_TRUE(helper_blocked(t), "the read blocks on an empty cbuf");
    thread_signal_kill(t);
    int ret = 0;
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, 5000), "the kill ends the read");
    EXPECT_EQ(ERR_INTERRUPTED, ret, "");

    // data ends it as before
    t = thread_create("cbuf data", read_interruptible, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "");
    thread_resume(t);
    ASSERT_TRUE(helper_blocked(t), "the read blocks on an empty cbuf");
    EXPECT_EQ(2UL, cbuf_write(&etc_cbuf, "ab", 2, true), "");
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, 5000), "data ends the read");
    EXPECT_EQ(2, ret, "");

    // without the flag a latched kill changes nothing: the read waits for data
    t = thread_create("cbuf nokill", read_uninterruptible_after_kill, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "");
    thread_resume(t);
    ASSERT_TRUE(helper_blocked(t), "the read blocks on an empty cbuf");
    EXPECT_EQ(3UL, cbuf_write(&etc_cbuf, "cde", 3, true), "");
    ASSERT_EQ(NO_ERROR, thread_join(t, &ret, 5000), "data ends the read");
    EXPECT_EQ(3, ret, "");

    // a non-blocking read takes no wait at all
    char buf[4];
    EXPECT_EQ((ssize_t)0, cbuf_read_etc(&etc_cbuf, buf, sizeof(buf), false, WAIT_QUEUE_INTERRUPTIBLE), "");

    END_TEST;
}

BEGIN_TEST_CASE(cbuf_tests)
RUN_TEST(basic)
RUN_TEST(random)
RUN_TEST(is_full_edge_cases)
RUN_TEST(is_full_wraparound)
RUN_TEST(peek_tests)
RUN_TEST(read_etc_tests)
END_TEST_CASE(cbuf_tests)