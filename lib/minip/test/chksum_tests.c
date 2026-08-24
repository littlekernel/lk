/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/unittest.h>
#include <string.h>

#include "../minip-internal.h"

static bool fold_all_ones(void) {
    BEGIN_TEST;

    const uint8_t buf[4] = { 0xff, 0xff, 0xff, 0xff };
    EXPECT_EQ(0xffff, ones_sum16(0, buf, sizeof(buf)), "");

    END_TEST;
}

static bool zero_length(void) {
    BEGIN_TEST;

    const uint8_t byte = 0xa5;
    EXPECT_EQ(0, ones_sum16(0, &byte, 0), "");

    END_TEST;
}

static bool odd_length_matches_padded(void) {
    BEGIN_TEST;

    const uint8_t odd[3] = { 0x12, 0x34, 0x56 };
    const uint8_t padded[4] = { 0x12, 0x34, 0x56, 0x00 };
    EXPECT_EQ(ones_sum16(0, padded, 4), ones_sum16(0, odd, 3),
              "trailing byte should act as if zero padded");

    END_TEST;
}

static bool chained_matches_whole(void) {
    BEGIN_TEST;

    uint8_t buf[32];
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = (uint8_t)(i * 7 + 3);
    }
    uint16_t whole = ones_sum16(0, buf, sizeof(buf));
    uint16_t chained = ones_sum16(ones_sum16(0, buf, 16), buf + 16, 16);
    EXPECT_EQ(whole, chained, "checksum should chain across even sized pieces");

    END_TEST;
}

static bool ipv4_header_round_trip(void) {
    BEGIN_TEST;

    struct ipv4_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.ver_ihl = 0x45;
    hdr.len = htons(84);
    hdr.ttl = 64;
    hdr.proto = IP_PROTO_ICMP;
    hdr.src_addr = IPV4(192, 168, 1, 99);
    hdr.dst_addr = IPV4(10, 0, 0, 1);
    hdr.chksum = 0;
    hdr.chksum = ~ones_sum16(0, &hdr, sizeof(hdr));

    /* summing the header including its checksum must give all ones */
    EXPECT_EQ(0xffff, ones_sum16(0, &hdr, sizeof(hdr)), "");

    /* and corruption must break that */
    hdr.ttl ^= 0x40;
    EXPECT_NE(0xffff, ones_sum16(0, &hdr, sizeof(hdr)), "");

    END_TEST;
}

BEGIN_TEST_CASE(chksum_tests)
RUN_TEST(fold_all_ones)
RUN_TEST(zero_length)
RUN_TEST(odd_length_matches_padded)
RUN_TEST(chained_matches_whole)
RUN_TEST(ipv4_header_round_trip)
END_TEST_CASE(chksum_tests)
