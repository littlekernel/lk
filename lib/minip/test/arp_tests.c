/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/unittest.h>
#include <kernel/thread.h>
#include <lk/err.h>
#include <string.h>

#include "../minip-internal.h"
#include "testnetif.h"

/* pick fresh addresses each invocation so reruns of 'ut all' in one boot
 * don't collide with cache entries from the previous run (entries and the
 * results of past resolutions are permanent) */
static uint8_t next_host = 100;

static bool cache_update_lookup(void) {
    BEGIN_TEST;

    const ipv4_addr_t addr = IPV4(10, 99, 1, next_host++);
    const uint8_t mac1[6] = { 0x02, 1, 2, 3, 4, 5 };
    const uint8_t mac2[6] = { 0x02, 5, 4, 3, 2, 1 };

    EXPECT_NULL(arp_cache_lookup(addr), "unknown address should miss");

    arp_cache_update(addr, mac1);
    uint8_t *mac = arp_cache_lookup(addr);
    ASSERT_NONNULL(mac, "");
    EXPECT_BYTES_EQ(mac1, mac, 6, "");

    arp_cache_update(addr, mac2);
    mac = arp_cache_lookup(addr);
    ASSERT_NONNULL(mac, "");
    EXPECT_BYTES_EQ(mac2, mac, 6, "cache entry should update in place");

    /* 0.0.0.0 and x.x.x.255 are never cached */
    arp_cache_update(IPV4(0, 0, 0, 0), mac1);
    EXPECT_NULL(arp_cache_lookup(IPV4(0, 0, 0, 0)), "");

    END_TEST;
}

/* wait for a captured frame matching an ethertype and, optionally, dest mac */
static bool wait_for_frame(uint16_t ethertype, const uint8_t *dst_mac, lk_time_t timeout) {
    for (lk_time_t waited = 0; waited <= timeout; waited += 10) {
        for (uint idx = 0; idx < TESTNETIF_CAPTURE_FRAMES; idx++) {
            uint8_t frame[TESTNETIF_CAPTURE_BYTES];
            size_t len = testnetif_capture_get(idx, frame, sizeof(frame));
            if (len < sizeof(struct eth_hdr)) {
                continue;
            }
            const struct eth_hdr *eth = (const struct eth_hdr *)frame;
            if (ntohs(eth->type) != ethertype) {
                continue;
            }
            if (dst_mac && memcmp(eth->dst_mac, dst_mac, 6) != 0) {
                continue;
            }
            return true;
        }
        thread_sleep(10);
    }
    return false;
}

static void inject_arp_reply(ipv4_addr_t from_addr, const uint8_t *from_mac) {
    netif_t *n = testnetif_get();

    struct {
        struct eth_hdr eth;
        struct arp_pkt arp;
    } __PACKED frame;

    memset(&frame, 0, sizeof(frame));
    mac_addr_copy(frame.eth.dst_mac, n->mac_address);
    mac_addr_copy(frame.eth.src_mac, from_mac);
    frame.eth.type = htons(ETH_TYPE_ARP);
    frame.arp.htype = htons(0x0001);
    frame.arp.ptype = htons(0x0800);
    frame.arp.hlen = 6;
    frame.arp.plen = 4;
    frame.arp.oper = htons(ARP_OPER_REPLY);
    mac_addr_copy(frame.arp.sha, from_mac);
    frame.arp.spa = from_addr;
    mac_addr_copy(frame.arp.tha, n->mac_address);
    frame.arp.tpa = n->ipv4_addr;

    testnetif_inject(&frame, sizeof(frame));
}

static bool resolution_parks_and_flushes(void) {
    BEGIN_TEST;

    testnetif_get();
    testnetif_configure(NULL);

    const ipv4_addr_t target = IPV4(10, 99, 0, next_host++);

    /* a send to an unresolved address parks the frame and emits a request */
    udp_socket_t *sock = NULL;
    ASSERT_EQ(NO_ERROR, udp_open(target, 4444, 4444, &sock), "");
    uint8_t payload[32];
    memset(payload, 0xa5, sizeof(payload));
    EXPECT_EQ(NO_ERROR, udp_send(payload, sizeof(payload), sock), "");

    EXPECT_TRUE(wait_for_frame(ETH_TYPE_ARP, NULL, 1000), "no arp request was transmitted");
    EXPECT_NULL(arp_cache_lookup(target), "should not be resolved yet");

    /* answering the request should release the parked frame to the peer mac */
    inject_arp_reply(target, testnetif_peer_mac());

    for (lk_time_t waited = 0; waited < 1000 && !arp_cache_lookup(target); waited += 10) {
        thread_sleep(10);
    }
    EXPECT_NONNULL(arp_cache_lookup(target), "reply should have resolved the entry");
    EXPECT_TRUE(wait_for_frame(ETH_TYPE_IPV4, testnetif_peer_mac(), 1000),
                "parked frame was not released to the resolved mac");

    udp_close(sock);

    END_TEST;
}

static bool resolution_timeout_drops(void) {
    BEGIN_TEST;

    testnetif_get();
    testnetif_configure(NULL);

    pktbuf_stats_t before;
    pktbuf_get_stats(&before);

    const ipv4_addr_t target = IPV4(10, 99, 0, next_host++);

    udp_socket_t *sock = NULL;
    ASSERT_EQ(NO_ERROR, udp_open(target, 4445, 4445, &sock), "");
    uint8_t payload[16];
    memset(payload, 0x5a, sizeof(payload));
    EXPECT_EQ(NO_ERROR, udp_send(payload, sizeof(payload), sock), "");

    /* never answer; resolution gives up after its retries and frees the
     * parked frame (3 x 500ms, plus slop) */
    for (lk_time_t waited = 0; waited < 2500; waited += 50) {
        thread_sleep(50);
        pktbuf_stats_t now;
        pktbuf_get_stats(&now);
        if (now.bufs_free >= before.bufs_free) {
            break;
        }
    }

    pktbuf_stats_t after;
    pktbuf_get_stats(&after);
    EXPECT_EQ(before.bufs_free, after.bufs_free, "parked frame leaked on resolution failure");
    EXPECT_NULL(arp_cache_lookup(target), "");

    udp_close(sock);

    END_TEST;
}

BEGIN_TEST_CASE(arp_tests)
RUN_TEST(cache_update_lookup)
RUN_TEST(resolution_parks_and_flushes)
RUN_TEST(resolution_timeout_drops)
END_TEST_CASE(arp_tests)
