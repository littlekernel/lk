/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/unittest.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <lk/err.h>
#include <string.h>

#include "../minip-internal.h"
#include "testnetif.h"

static void signal_event_cb(void *arg) {
    event_signal((event_t *)arg, true);
}

static bool timer_fires(void) {
    BEGIN_TEST;

    event_t evt = EVENT_INITIAL_VALUE(evt, false, 0);
    net_timer_t timer;
    memset(&timer, 0, sizeof(timer));

    EXPECT_TRUE(net_timer_set(&timer, &signal_event_cb, &evt, 20), "");
    EXPECT_EQ(NO_ERROR, event_wait_timeout(&evt, 1000), "timer never fired");

    END_TEST;
}

static bool timer_cancel(void) {
    BEGIN_TEST;

    event_t evt = EVENT_INITIAL_VALUE(evt, false, 0);
    net_timer_t timer;
    memset(&timer, 0, sizeof(timer));

    EXPECT_TRUE(net_timer_set(&timer, &signal_event_cb, &evt, 100), "");
    EXPECT_TRUE(net_timer_cancel(&timer), "");
    EXPECT_FALSE(net_timer_cancel(&timer), "second cancel should find nothing");

    EXPECT_EQ(ERR_TIMED_OUT, event_wait_timeout(&evt, 300), "cancelled timer fired anyway");

    END_TEST;
}

static bool timer_rearm(void) {
    BEGIN_TEST;

    event_t evt = EVENT_INITIAL_VALUE(evt, false, 0);
    net_timer_t timer;
    memset(&timer, 0, sizeof(timer));

    /* arm far in the future, then move it close; the second set is a move,
     * not a new queue */
    EXPECT_TRUE(net_timer_set(&timer, &signal_event_cb, &evt, 10000), "");
    EXPECT_FALSE(net_timer_set(&timer, &signal_event_cb, &evt, 20), "");
    EXPECT_EQ(NO_ERROR, event_wait_timeout(&evt, 1000), "moved timer never fired");

    END_TEST;
}

static int fire_order[2];
static int fire_order_next;

static void order_cb(void *arg) {
    if (fire_order_next < 2) {
        fire_order[fire_order_next++] = (int)(uintptr_t)arg;
    }
}

static bool timer_ordering(void) {
    BEGIN_TEST;

    net_timer_t t1, t2;
    memset(&t1, 0, sizeof(t1));
    memset(&t2, 0, sizeof(t2));
    fire_order_next = 0;

    /* queue the later one first */
    EXPECT_TRUE(net_timer_set(&t1, &order_cb, (void *)2, 80), "");
    EXPECT_TRUE(net_timer_set(&t2, &order_cb, (void *)1, 20), "");

    thread_sleep(300);

    ASSERT_EQ(2, fire_order_next, "both timers should have fired");
    EXPECT_EQ(1, fire_order[0], "earlier deadline should fire first");
    EXPECT_EQ(2, fire_order[1], "");

    END_TEST;
}

static bool route_lookup_and_ref(void) {
    BEGIN_TEST;

    netif_t *n = testnetif_get();

    /* add a second subnet on the test interface exactly once per boot;
     * route table slots are never freed */
    static bool added = false;
    if (!added) {
        ASSERT_EQ(NO_ERROR, ipv4_add_route(IPV4(10, 98, 0, 0), 0x00ffffff, n), "");
        added = true;
    }

    ipv4_route_t *r = ipv4_search_route(IPV4(10, 98, 0, 5));
    ASSERT_NONNULL(r, "");
    EXPECT_EQ(n, r->interface, "route should point at the test interface");
    EXPECT_TRUE(r->ref >= 1, "search should hold a reference");

    /* a second search on the same route stacks another reference */
    ipv4_route_t *r2 = ipv4_search_route(IPV4(10, 98, 0, 9));
    ASSERT_NONNULL(r2, "");
    EXPECT_EQ(r, r2, "same subnet should give the same route");

    ipv4_dec_route_ref(r2);
    ipv4_dec_route_ref(r);

    /* the interface's own subnet route from netif_set_ipv4_addr */
    r = ipv4_search_route(IPV4(10, 99, 0, 42));
    ASSERT_NONNULL(r, "");
    EXPECT_EQ(n, r->interface, "");
    ipv4_dec_route_ref(r);

    END_TEST;
}

static bool ipaddr_parse(void) {
    BEGIN_TEST;

    static const struct {
        const char *str;
        ipv4_addr_t addr;
    } good[] = {
        { "0.0.0.0", IPV4(0, 0, 0, 0) },
        { "127.0.0.1", IPV4(127, 0, 0, 1) },
        { "10.0.2.15", IPV4(10, 0, 2, 15) },
        { "255.255.255.255", IPV4(255, 255, 255, 255) },
        { "1.2.3.255", IPV4(1, 2, 3, 255) },
        { "192.168.000.001", IPV4(192, 168, 0, 1) },
    };

    for (size_t i = 0; i < countof(good); i++) {
        ipv4_addr_t addr = IPV4_BCAST;
        EXPECT_EQ(NO_ERROR, minip_parse_ipaddr_checked(good[i].str, strlen(good[i].str), &addr),
                  good[i].str);
        EXPECT_EQ(good[i].addr, addr, good[i].str);
        EXPECT_EQ(good[i].addr, minip_parse_ipaddr(good[i].str, strlen(good[i].str)), good[i].str);
    }

    static const char *bad[] = {
        "",
        ".",
        "1.2.3",
        "1.2.3.4.5",         /* used to write past the end of the octet array */
        "1.2.3.4.5.6.7.8.9",
        "1..2.3",
        "1.2.3.",
        "256.1.1.1",
        "1.2.3.256",
        "0000.1.1.1",
        "1.2.3.4x",
        "irc.sortix.org",
        "localhost",
        "-1.2.3.4",
        " 1.2.3.4",
    };

    for (size_t i = 0; i < countof(bad); i++) {
        ipv4_addr_t addr = IPV4_BCAST;
        EXPECT_LT(minip_parse_ipaddr_checked(bad[i], strlen(bad[i]), &addr), 0, bad[i]);
        EXPECT_EQ(IPV4_BCAST, addr, "failed parse should not touch the output");
        EXPECT_EQ((ipv4_addr_t)IPV4_NONE, minip_parse_ipaddr(bad[i], strlen(bad[i])), bad[i]);
    }

    /* the length argument bounds the parse */
    ipv4_addr_t addr = IPV4_BCAST;
    EXPECT_EQ(NO_ERROR, minip_parse_ipaddr_checked("10.0.0.1 and trailing junk", 8, &addr), "");
    EXPECT_EQ(IPV4(10, 0, 0, 1), addr, "");

    END_TEST;
}

BEGIN_TEST_CASE(netstack_tests)
RUN_TEST(timer_fires)
RUN_TEST(timer_cancel)
RUN_TEST(timer_rearm)
RUN_TEST(timer_ordering)
RUN_TEST(route_lookup_and_ref)
RUN_TEST(ipaddr_parse)
END_TEST_CASE(netstack_tests)
