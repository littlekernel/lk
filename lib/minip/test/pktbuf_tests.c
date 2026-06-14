/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/pktbuf.h>
#include <lib/unittest.h>
#include <string.h>
#include <stdlib.h>

static void my_free_cb(void *buf, void *arg) {
    bool *called_ptr = (bool *)arg;
    if (called_ptr) {
        *called_ptr = true;
    }
}

static bool basic_alloc_free(void) {
    BEGIN_TEST;

    pktbuf_t *p = pktbuf_alloc();
    ASSERT_NONNULL(p, "");
    EXPECT_NONNULL(p->buffer, "");
    EXPECT_EQ((uint32_t)PKTBUF_MAX_HDR, pktbuf_avail_head(p), "Header space mismatch");
    EXPECT_EQ((uint32_t)PKTBUF_MAX_DATA, pktbuf_avail_tail(p), "Tail space mismatch");
    EXPECT_EQ(0UL, p->dlen, "Initial data length should be 0");

    pktbuf_free(p, false);

    END_TEST;
}

static bool append_prepend_consume(void) {
    BEGIN_TEST;

    pktbuf_t *p = pktbuf_alloc();
    ASSERT_NONNULL(p, "");

    // Test append data
    pktbuf_append_data(p, "hello", 5);
    EXPECT_EQ(5UL, p->dlen, "");
    EXPECT_BYTES_EQ((const uint8_t *)"hello", p->data, 5, "Appended data mismatch");

    // Test append space
    void *new_data = pktbuf_append(p, 5);
    ASSERT_NONNULL(new_data, "");
    memcpy(new_data, "world", 5);
    EXPECT_EQ(10UL, p->dlen, "");
    EXPECT_BYTES_EQ((const uint8_t *)"helloworld", p->data, 10, "Combined data mismatch");

    // Test prepend space
    void *prepended = pktbuf_prepend(p, 4);
    ASSERT_NONNULL(prepended, "");
    memcpy(prepended, "say_", 4);
    EXPECT_EQ(14UL, p->dlen, "");
    EXPECT_BYTES_EQ((const uint8_t *)"say_helloworld", p->data, 14, "Prepended data mismatch");

    // Test consume space
    void *consumed = pktbuf_consume(p, 4);
    ASSERT_NONNULL(consumed, "");
    EXPECT_BYTES_EQ((const uint8_t *)"say_", consumed, 4, "Consumed data mismatch");
    EXPECT_EQ(10UL, p->dlen, "");
    EXPECT_BYTES_EQ((const uint8_t *)"helloworld", p->data, 10, "Remaining data mismatch");

    // Test consume tail space
    pktbuf_consume_tail(p, 5);
    EXPECT_EQ(5UL, p->dlen, "");
    EXPECT_BYTES_EQ((const uint8_t *)"hello", p->data, 5, "Consumed tail data mismatch");

    pktbuf_free(p, false);

    END_TEST;
}

static bool custom_buffer(void) {
    BEGIN_TEST;

    pktbuf_t *p = pktbuf_alloc_empty();
    ASSERT_NONNULL(p, "");
    EXPECT_EQ((uint32_t)PKTBUF_FLAG_EOF, p->flags, "Empty packet flags mismatch");

    uint8_t buf[256];
    bool cb_called = false;
    pktbuf_add_buffer(p, buf, 256, 32, 0, my_free_cb, &cb_called);

    EXPECT_EQ(buf, p->buffer, "Custom buffer pointer mismatch");
    EXPECT_EQ(32UL, pktbuf_avail_head(p), "Custom header size mismatch");
    EXPECT_EQ(0UL, p->dlen, "Initial data length mismatch");
    EXPECT_FALSE(cb_called, "Callback should not be called yet");

    pktbuf_free(p, false);
    EXPECT_TRUE(cb_called, "Callback should have been called upon free");

    END_TEST;
}

static bool reset_test(void) {
    BEGIN_TEST;

    pktbuf_t *p = pktbuf_alloc();
    ASSERT_NONNULL(p, "");

    pktbuf_append_data(p, "test", 4);
    pktbuf_reset(p, 32);

    EXPECT_EQ(32UL, pktbuf_avail_head(p), "Reset header offset mismatch");
    EXPECT_EQ(0UL, p->dlen, "Reset length should be 0");

    pktbuf_free(p, false);

    END_TEST;
}

static bool recommended_rx_depth_test(void) {
    BEGIN_TEST;

    EXPECT_EQ(0UL, pktbuf_recommended_eth_rx_depth(0), "Depth 0 should return 0");

    size_t d8 = pktbuf_recommended_eth_rx_depth(8);
    EXPECT_NE(0UL, d8, "Recommended depth should be non-zero");
    // Ensure returned depth is a power of 2
    EXPECT_EQ(0UL, d8 & (d8 - 1), "Recommended depth must be power of 2");

    END_TEST;
}

BEGIN_TEST_CASE(pktbuf_tests)
RUN_TEST(basic_alloc_free)
RUN_TEST(append_prepend_consume)
RUN_TEST(custom_buffer)
RUN_TEST(reset_test)
RUN_TEST(recommended_rx_depth_test)
END_TEST_CASE(pktbuf_tests)
