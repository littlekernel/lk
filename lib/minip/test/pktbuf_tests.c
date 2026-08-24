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

#include <kernel/thread.h>
#include <platform.h>

static void my_free_cb(void *buf, void *arg, bool reschedule) {
    bool *called_ptr = (bool *)arg;
    if (called_ptr) {
        *called_ptr = true;
    }
}

static void counting_free_cb(void *buf, void *arg, bool reschedule) {
    (*(int *)arg)++;
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

static bool alloc_rx_test(void) {
    BEGIN_TEST;

    pktbuf_t *p = pktbuf_alloc_rx();
    ASSERT_NONNULL(p, "");
    EXPECT_EQ(0U, pktbuf_avail_head(p), "RX buffers should have no headroom");
    EXPECT_EQ((u32)PKTBUF_SIZE, pktbuf_avail_tail(p), "");
    EXPECT_TRUE(p->flags & PKTBUF_FLAG_EOF, "");

    pktbuf_free(p, false);

    END_TEST;
}

static bool refcount_test(void) {
    BEGIN_TEST;

    uint8_t buf[256];
    int count = 0;

    pktbuf_t *p = pktbuf_alloc_empty();
    ASSERT_NONNULL(p, "");
    pktbuf_add_buffer(p, buf, sizeof(buf), 32, 0, counting_free_cb, &count);
    EXPECT_EQ(1, p->ref, "");

    pktbuf_ref(p);
    EXPECT_EQ(2, p->ref, "");

    pktbuf_free(p, false);
    EXPECT_EQ(0, count, "callback must not run while a reference is outstanding");
    EXPECT_EQ(1, p->ref, "");

    pktbuf_free(p, false);
    EXPECT_EQ(1, count, "callback must run exactly once on the last free");

    END_TEST;
}

static bool accounting_baseline(void) {
    BEGIN_TEST;

    pktbuf_stats_t before;
    pktbuf_get_stats(&before);

    for (int i = 0; i < 32; i++) {
        pktbuf_t *p = pktbuf_alloc();
        ASSERT_NONNULL(p, "");
        pktbuf_t *rx = pktbuf_alloc_rx();
        ASSERT_NONNULL(rx, "");
        pktbuf_t *e = pktbuf_alloc_empty();
        ASSERT_NONNULL(e, "");

        uint8_t buf[64];
        pktbuf_add_buffer(e, buf, sizeof(buf), 0, 0, my_free_cb, NULL);

        pktbuf_free(rx, false);
        pktbuf_free(e, false);
        pktbuf_free(p, false);
    }

    pktbuf_stats_t after;
    pktbuf_get_stats(&after);
    EXPECT_EQ(before.bufs_free, after.bufs_free, "data buffers leaked");
    EXPECT_EQ(before.hdrs_free, after.hdrs_free, "headers leaked");

    END_TEST;
}

static bool alloc_exhaustion(void) {
    BEGIN_TEST;

    pktbuf_stats_t stats;
    pktbuf_get_stats(&stats);

    /* nothing else should be allocating from the pool while this runs */
    struct list_node list = LIST_INITIAL_VALUE(list);
    size_t count = 0;
    pktbuf_t *p;
    while ((p = pktbuf_alloc()) != NULL) {
        list_add_tail(&list, &p->list);
        count++;
        ASSERT_TRUE(count <= stats.bufs_total, "allocated more buffers than the pool holds");
    }
    EXPECT_EQ(stats.bufs_free, count, "should have allocated every free buffer");

    while ((p = list_remove_head_type(&list, pktbuf_t, list)) != NULL) {
        pktbuf_free(p, false);
    }

    p = pktbuf_alloc();
    EXPECT_NONNULL(p, "pool should recover after freeing");
    if (p) {
        pktbuf_free(p, false);
    }

    END_TEST;
}

static int delayed_free_thread(void *arg) {
    thread_sleep(20);
    pktbuf_free((pktbuf_t *)arg, false);
    return 0;
}

static bool alloc_timeout_test(void) {
    BEGIN_TEST;

    /* drain the pool; on non LK_EMBEDDED builds this also grows it to its ceiling */
    struct list_node list = LIST_INITIAL_VALUE(list);
    pktbuf_t *p;
    while ((p = pktbuf_alloc_timeout(1)) != NULL) {
        list_add_tail(&list, &p->list);
    }

    pktbuf_stats_t stats;
    pktbuf_get_stats(&stats);
    EXPECT_EQ(stats.bufs_max, stats.bufs_total, "pool should have grown to its ceiling");
    EXPECT_EQ(0U, stats.bufs_free, "");

    /* with the pool empty, a timed alloc must wait at least the timeout */
    lk_time_t t = current_time();
    p = pktbuf_alloc_timeout(50);
    t = current_time() - t;
    EXPECT_NULL(p, "");
    EXPECT_TRUE(t >= 50, "timeout returned early");

    /* a free from another thread should satisfy a waiter */
    pktbuf_t *tofree = list_remove_head_type(&list, pktbuf_t, list);
    ASSERT_NONNULL(tofree, "");
    thread_t *th = thread_create("pktbuf test free", delayed_free_thread, tofree,
                                 DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(th, "");
    thread_resume(th);
    p = pktbuf_alloc_timeout(1000);
    EXPECT_NONNULL(p, "alloc_timeout should be satisfied by a cross thread free");
    thread_join(th, NULL, INFINITE_TIME);
    if (p) {
        pktbuf_free(p, false);
    }

    while ((p = list_remove_head_type(&list, pktbuf_t, list)) != NULL) {
        pktbuf_free(p, false);
    }

    END_TEST;
}

BEGIN_TEST_CASE(pktbuf_tests)
RUN_TEST(basic_alloc_free)
RUN_TEST(append_prepend_consume)
RUN_TEST(custom_buffer)
RUN_TEST(reset_test)
RUN_TEST(recommended_rx_depth_test)
RUN_TEST(alloc_rx_test)
RUN_TEST(refcount_test)
RUN_TEST(accounting_baseline)
RUN_TEST(alloc_exhaustion)
RUN_TEST(alloc_timeout_test)
END_TEST_CASE(pktbuf_tests)
