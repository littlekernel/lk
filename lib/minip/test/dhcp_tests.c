/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* Tests for the DHCP option parser, run against canned option blobs. No
 * network, no server, no interface required.
 */

#include <lib/unittest.h>
#include <lib/minip.h>
#include <lk/compiler.h>
#include <string.h>

#include "../minip-internal.h"

static bool basic_options(void) {
    BEGIN_TEST;

    static const uint8_t opts[] = {
        53, 1, 5,                       /* message type: DHCPACK */
        1, 4, 255, 255, 255, 0,         /* netmask 255.255.255.0 */
        3, 4, 10, 0, 2, 2,              /* router 10.0.2.2 */
        6, 4, 10, 0, 2, 3,              /* dns 10.0.2.3 */
        54, 4, 10, 0, 2, 2,             /* server id 10.0.2.2 */
        255,                            /* end */
    };

    dhcp_options_t o;
    dhcp_parse_options(opts, sizeof(opts), &o);

    EXPECT_EQ(5, o.op, "");
    EXPECT_EQ(IPV4(255, 255, 255, 0), o.netmask, "");
    EXPECT_EQ(IPV4(10, 0, 2, 2), o.gateway, "");
    EXPECT_EQ(IPV4(10, 0, 2, 3), o.dns, "");
    EXPECT_EQ(IPV4(10, 0, 2, 2), o.server, "");

    END_TEST;
}

static bool multiple_servers_and_padding(void) {
    BEGIN_TEST;

    /* pad bytes between options, and option lists longer than one entry:
     * only the first address of each is used
     */
    static const uint8_t opts[] = {
        0, 0,                           /* pad */
        53, 1, 2,                       /* message type: DHCPOFFER */
        0,                              /* pad */
        6, 8, 8, 8, 8, 8, 1, 1, 1, 1,   /* dns 8.8.8.8, 1.1.1.1 */
        3, 8, 10, 0, 0, 1, 10, 0, 0, 2, /* routers 10.0.0.1, 10.0.0.2 */
        255,
        99, 99, 99,                     /* junk past the end marker */
    };

    dhcp_options_t o;
    dhcp_parse_options(opts, sizeof(opts), &o);

    EXPECT_EQ(2, o.op, "");
    EXPECT_EQ(IPV4(8, 8, 8, 8), o.dns, "");
    EXPECT_EQ(IPV4(10, 0, 0, 1), o.gateway, "");
    EXPECT_EQ((ipv4_addr_t)IPV4_NONE, o.netmask, "absent option should stay zero");
    EXPECT_EQ((ipv4_addr_t)IPV4_NONE, o.server, "absent option should stay zero");

    END_TEST;
}

static bool classless_static_routes(void) {
    BEGIN_TEST;

    /* RFC3442 option 121: [prefix width][significant dest bytes][router].
     * The 0/0 entry supplies the gateway when option 3 is absent.
     */
    static const uint8_t opts[] = {
        53, 1, 5,
        121, 14,
            24, 10, 0, 2, 10, 0, 2, 2,  /* 10.0.2.0/24 via 10.0.2.2 */
            0, 192, 168, 1, 1,          /* default via 192.168.1.1 */
        255,
    };

    dhcp_options_t o;
    dhcp_parse_options(opts, sizeof(opts), &o);
    EXPECT_EQ(IPV4(192, 168, 1, 1), o.gateway, "0/0 entry should become the gateway");

    /* an explicit option 3 that arrives first wins over the 0/0 entry */
    static const uint8_t opts2[] = {
        53, 1, 5,
        3, 4, 10, 0, 2, 2,
        121, 5,
            0, 192, 168, 1, 1,
        255,
    };

    dhcp_parse_options(opts2, sizeof(opts2), &o);
    EXPECT_EQ(IPV4(10, 0, 2, 2), o.gateway, "option 3 should win");

    END_TEST;
}

static bool hostile_options(void) {
    BEGIN_TEST;

    dhcp_options_t o;

    /* an empty option area */
    dhcp_parse_options(NULL, 0, &o);
    EXPECT_EQ(-1, o.op, "no message type should read as -1");
    EXPECT_EQ((ipv4_addr_t)IPV4_NONE, o.gateway, "");

    /* an option whose length runs off the end of the buffer */
    static const uint8_t truncated[] = { 53, 1, 5, 3, 4, 10, 0 };
    dhcp_parse_options(truncated, sizeof(truncated), &o);
    EXPECT_EQ(5, o.op, "options before the bad one still parse");
    EXPECT_EQ((ipv4_addr_t)IPV4_NONE, o.gateway, "truncated option should be dropped");

    /* a length byte with no room for itself */
    static const uint8_t dangling[] = { 53 };
    dhcp_parse_options(dangling, sizeof(dangling), &o);
    EXPECT_EQ(-1, o.op, "");

    /* wrong option lengths are ignored rather than half applied */
    static const uint8_t wrong_len[] = {
        53, 2, 5, 5,        /* message type must be 1 byte */
        1, 2, 255, 255,     /* netmask must be 4 bytes */
        54, 5, 1, 2, 3, 4, 5,
        255,
    };
    dhcp_parse_options(wrong_len, sizeof(wrong_len), &o);
    EXPECT_EQ(-1, o.op, "");
    EXPECT_EQ((ipv4_addr_t)IPV4_NONE, o.netmask, "");
    EXPECT_EQ((ipv4_addr_t)IPV4_NONE, o.server, "");

    /* a classless route entry whose destination bytes run past the option */
    static const uint8_t bad_route[] = {
        121, 4, 32, 10, 0, 2,
        255,
    };
    dhcp_parse_options(bad_route, sizeof(bad_route), &o);
    EXPECT_EQ((ipv4_addr_t)IPV4_NONE, o.gateway, "");

    /* the whole area is pad bytes, with no end marker */
    static const uint8_t all_pad[16] = { 0 };
    dhcp_parse_options(all_pad, sizeof(all_pad), &o);
    EXPECT_EQ(-1, o.op, "");

    END_TEST;
}

BEGIN_TEST_CASE(dhcp_tests)
RUN_TEST(basic_options)
RUN_TEST(multiple_servers_and_padding)
RUN_TEST(classless_static_routes)
RUN_TEST(hostile_options)
END_TEST_CASE(dhcp_tests)
