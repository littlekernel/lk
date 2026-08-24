/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* UDP unit tests, run entirely over loopback (127.0.0.1) so they work on
 * NIC-less targets too: never wait for a configured interface here.
 */

#include <lib/unittest.h>
#include <lib/minip.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <lk/err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../minip-internal.h"

#define UDP_TEST_PORT 30099

struct udp_sink {
    event_t event;
    thread_t *cb_thread;
    uint32_t src_addr;
    uint16_t src_port;
    size_t len;
    uint8_t data[64];
    uint count;
};

static void udp_sink_cb(void *data, size_t len, uint32_t srcaddr, uint16_t srcport, void *arg) {
    struct udp_sink *sink = (struct udp_sink *)arg;

    sink->cb_thread = get_current_thread();
    sink->src_addr = srcaddr;
    sink->src_port = srcport;
    sink->len = len;
    memcpy(sink->data, data, MIN(len, sizeof(sink->data)));
    sink->count++;
    event_signal(&sink->event, true);
}

static bool loopback_roundtrip(void) {
    BEGIN_TEST;

    static struct udp_sink sink;
    memset(&sink, 0, sizeof(sink));
    event_init(&sink.event, false, EVENT_FLAG_AUTOUNSIGNAL);

    ASSERT_EQ(0, udp_listen(UDP_TEST_PORT, udp_sink_cb, &sink), "");

    udp_socket_t *sock = NULL;
    ASSERT_EQ(NO_ERROR, udp_open(IPV4(127, 0, 0, 1), UDP_TEST_PORT + 1, UDP_TEST_PORT, &sock), "");

    for (int i = 0; i < 8; i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "udp-datagram-%d", i);

        ASSERT_EQ(NO_ERROR, udp_send(msg, strlen(msg), sock), "");
        ASSERT_EQ(NO_ERROR, event_wait_timeout(&sink.event, 2000), "datagram should arrive");

        EXPECT_EQ(strlen(msg), sink.len, "");
        EXPECT_BYTES_EQ((const uint8_t *)msg, sink.data, strlen(msg), "");
        EXPECT_EQ(IPV4(127, 0, 0, 1), sink.src_addr, "");
        EXPECT_EQ(UDP_TEST_PORT + 1, sink.src_port, "");
    }
    EXPECT_EQ(8U, sink.count, "");

    /* delivery happens on the stack worker, never the sending thread */
    EXPECT_NE(get_current_thread(), sink.cb_thread, "callback should run on the netstack thread");

    EXPECT_EQ(NO_ERROR, udp_close(sock), "");
    EXPECT_EQ(0, udp_listen(UDP_TEST_PORT, NULL, NULL), "listener should unregister");

    END_TEST;
}

static bool listen_registration(void) {
    BEGIN_TEST;

    EXPECT_NE(0, udp_listen(UDP_TEST_PORT + 2, NULL, NULL),
              "removing a listener that doesn't exist should fail");

    ASSERT_EQ(0, udp_listen(UDP_TEST_PORT + 2, udp_sink_cb, NULL), "");
    EXPECT_NE(0, udp_listen(UDP_TEST_PORT + 2, udp_sink_cb, NULL), "duplicate listen should fail");
    EXPECT_EQ(0, udp_listen(UDP_TEST_PORT + 2, NULL, NULL), "");

    END_TEST;
}

BEGIN_TEST_CASE(udp_tests)
RUN_TEST(loopback_roundtrip)
RUN_TEST(listen_registration)
END_TEST_CASE(udp_tests)
