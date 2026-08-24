/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* DNS tests. The builder and parser are exercised against canned messages
 * with no network at all; the resolver itself is then driven end to end
 * over the test interface, with the test acting as the name server.
 */

#include <lib/unittest.h>
#include <lib/minip.h>
#include <endian.h>
#include <lk/compiler.h>
#include <lk/err.h>
#include <stdio.h>
#include <string.h>

#include "../minip-internal.h"
#include "testnetif.h"

#define DNS_FLAGS_RESPONSE  0x8180  /* QR | RD | RA */
#define DNS_FLAGS_QUERY     0x0100  /* RD */

/* A message under construction. Static: these run near the default stack
 * size under 'ut all'.
 */
static uint8_t pkt[512];
static size_t pkt_len;

static void pkt_u8(uint8_t v) {
    pkt[pkt_len++] = v;
}

static void pkt_u16(uint16_t v) {
    uint16_t n = htons(v);
    memcpy(pkt + pkt_len, &n, sizeof(n));
    pkt_len += sizeof(n);
}

static void pkt_u32(uint32_t v) {
    uint32_t n = htonl(v);
    memcpy(pkt + pkt_len, &n, sizeof(n));
    pkt_len += sizeof(n);
}

/* a compression pointer at the given offset */
static void pkt_ptr(uint16_t offset) {
    pkt_u16(0xc000 | offset);
}

/* an uncompressed name */
static void pkt_name(const char *name) {
    for (const char *label = name;;) {
        const char *dot = strchr(label, '.');
        size_t len = dot ? (size_t)(dot - label) : strlen(label);

        pkt_u8((uint8_t)len);
        memcpy(pkt + pkt_len, label, len);
        pkt_len += len;

        if (!dot) {
            break;
        }
        label = dot + 1;
    }
    pkt_u8(0);
}

/* header plus the question section, which is just what a query is */
static bool pkt_question(uint16_t id, const char *name, uint16_t flags, uint16_t ancount) {
    ssize_t len = dns_build_query(pkt, sizeof(pkt), id, name, DNS_TYPE_A);
    if (len < 0) {
        return false;
    }
    pkt_len = (size_t)len;

    uint16_t v = htons(flags);
    memcpy(pkt + 2, &v, sizeof(v));
    v = htons(ancount);
    memcpy(pkt + 6, &v, sizeof(v));

    return true;
}

static void pkt_rr_header(uint16_t type, uint16_t cls, uint32_t ttl, uint16_t rdlen) {
    pkt_u16(type);
    pkt_u16(cls);
    pkt_u32(ttl);
    pkt_u16(rdlen);
}

/* a CNAME record; returns the offset of its encoded target name so that a
 * later record can point at it
 */
static size_t pkt_rr_cname(uint32_t ttl, const char *name) {
    pkt_rr_header(DNS_TYPE_CNAME, DNS_CLASS_IN, ttl, 0);

    size_t rdlen_offset = pkt_len - 2;
    size_t start = pkt_len;
    pkt_name(name);

    uint16_t rdlen = htons((uint16_t)(pkt_len - start));
    memcpy(pkt + rdlen_offset, &rdlen, sizeof(rdlen));

    return start;
}

static void pkt_rr_a(uint32_t ttl, ipv4_addr_t addr) {
    pkt_rr_header(DNS_TYPE_A, DNS_CLASS_IN, ttl, 4);
    memcpy(pkt + pkt_len, &addr, sizeof(addr));
    pkt_len += sizeof(addr);
}

static bool build_query(void) {
    BEGIN_TEST;

    uint8_t buf[64];
    ssize_t len = dns_build_query(buf, sizeof(buf), 0x1234, "example.com", DNS_TYPE_A);
    ASSERT_EQ((ssize_t)(12 + 13 + 4), len, "header + name + type/class");

    static const uint8_t expected[] = {
        0x12, 0x34,                                     /* id */
        0x01, 0x00,                                     /* recursion desired */
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* one question */
        7, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
        3, 'c', 'o', 'm',
        0,
        0x00, 0x01,                                     /* type A */
        0x00, 0x01,                                     /* class IN */
    };
    EXPECT_BYTES_EQ(expected, buf, sizeof(expected), "");

    /* a single trailing dot is the root and changes nothing */
    ssize_t len2 = dns_build_query(buf, sizeof(buf), 0x1234, "example.com.", DNS_TYPE_A);
    EXPECT_EQ(len, len2, "");
    EXPECT_BYTES_EQ(expected, buf, sizeof(expected), "");

    /* the query type is a parameter, so a future AAAA lookup fits */
    EXPECT_EQ(len, dns_build_query(buf, sizeof(buf), 1, "example.com", 28), "");
    EXPECT_EQ(28, (buf[len - 4] << 8) | buf[len - 3], "the query type goes on the wire");

    /* names that cannot be encoded */
    EXPECT_LT(dns_build_query(buf, sizeof(buf), 1, "", DNS_TYPE_A), 0, "empty name");
    EXPECT_LT(dns_build_query(buf, sizeof(buf), 1, ".", DNS_TYPE_A), 0, "root only");
    EXPECT_LT(dns_build_query(buf, sizeof(buf), 1, "a..b", DNS_TYPE_A), 0, "empty label");
    EXPECT_LT(dns_build_query(buf, sizeof(buf), 1, ".a.b", DNS_TYPE_A), 0, "leading dot");

    static char long_label[80];
    memset(long_label, 'a', 64);
    long_label[64] = '\0';
    EXPECT_LT(dns_build_query(buf, sizeof(buf), 1, long_label, DNS_TYPE_A), 0, "64 byte label");
    long_label[63] = '\0';
    EXPECT_LT(dns_build_query(buf, sizeof(buf), 1, long_label, DNS_TYPE_A), 0,
              "63 bytes is legal but does not fit this buffer");
    EXPECT_GT(dns_build_query(pkt, sizeof(pkt), 1, long_label, DNS_TYPE_A), 0,
              "63 bytes should encode into a big enough buffer");

    /* a name longer than the 255 byte encoded limit */
    static char long_name[400];
    for (size_t i = 0; i < sizeof(long_name) - 1; i++) {
        long_name[i] = ((i % 8) == 7) ? '.' : 'a';
    }
    long_name[sizeof(long_name) - 1] = '\0';
    EXPECT_LT(dns_build_query(pkt, sizeof(pkt), 1, long_name, DNS_TYPE_A), 0, "name too long");

    /* and a buffer with no room at all */
    EXPECT_LT(dns_build_query(buf, 4, 1, "example.com", DNS_TYPE_A), 0, "");
    EXPECT_LT(dns_build_query(NULL, sizeof(buf), 1, "example.com", DNS_TYPE_A), 0, "");

    END_TEST;
}

static bool parse_answer(void) {
    BEGIN_TEST;

    /* one A record, its owner name written out in full */
    ASSERT_TRUE(pkt_question(0x4321, "example.com", DNS_FLAGS_RESPONSE, 1), "");
    pkt_name("example.com");
    pkt_rr_a(300, IPV4(93, 184, 216, 34));

    dns_record_t rec = {};
    ASSERT_EQ(NO_ERROR, dns_parse_response(pkt, pkt_len, 0x4321, "example.com", &rec), "");
    EXPECT_EQ(DNS_TYPE_A, rec.type, "");
    EXPECT_EQ(300u, rec.ttl, "");
    EXPECT_EQ(IPV4(93, 184, 216, 34), rec.addr, "");

    /* name matching ignores case and a trailing dot */
    EXPECT_EQ(NO_ERROR, dns_parse_response(pkt, pkt_len, 0x4321, "ExAmPlE.CoM", &rec), "");
    EXPECT_EQ(NO_ERROR, dns_parse_response(pkt, pkt_len, 0x4321, "example.com.", &rec), "");

    /* the compressed form of the same answer */
    ASSERT_TRUE(pkt_question(0x4321, "example.com", DNS_FLAGS_RESPONSE, 1), "");
    pkt_ptr(12);                /* the question name, right after the header */
    pkt_rr_a(60, IPV4(10, 20, 30, 40));

    memset(&rec, 0, sizeof(rec));
    ASSERT_EQ(NO_ERROR, dns_parse_response(pkt, pkt_len, 0x4321, "example.com", &rec), "");
    EXPECT_EQ(IPV4(10, 20, 30, 40), rec.addr, "");
    EXPECT_EQ(60u, rec.ttl, "");

    /* records for other names are skipped, whatever order they arrive in */
    ASSERT_TRUE(pkt_question(0x4321, "example.com", DNS_FLAGS_RESPONSE, 3), "");
    pkt_name("elsewhere.com");
    pkt_rr_a(60, IPV4(1, 1, 1, 1));
    pkt_name("example.com.evil.com");
    pkt_rr_a(60, IPV4(2, 2, 2, 2));
    pkt_ptr(12);
    pkt_rr_a(60, IPV4(3, 3, 3, 3));

    memset(&rec, 0, sizeof(rec));
    ASSERT_EQ(NO_ERROR, dns_parse_response(pkt, pkt_len, 0x4321, "example.com", &rec), "");
    EXPECT_EQ(IPV4(3, 3, 3, 3), rec.addr, "only the record for our name counts");

    END_TEST;
}

static bool parse_cname_chain(void) {
    BEGIN_TEST;

    /* www.example.com CNAME cdn.example.net CNAME edge.example.net A ... */
    ASSERT_TRUE(pkt_question(7, "www.example.com", DNS_FLAGS_RESPONSE, 3), "");

    pkt_ptr(12);
    size_t cdn_offset = pkt_rr_cname(100, "cdn.example.net");

    pkt_ptr((uint16_t)cdn_offset);
    size_t edge_offset = pkt_rr_cname(100, "edge.example.net");

    pkt_ptr((uint16_t)edge_offset);
    pkt_rr_a(50, IPV4(203, 0, 113, 7));

    dns_record_t rec = {};
    ASSERT_EQ(NO_ERROR, dns_parse_response(pkt, pkt_len, 7, "www.example.com", &rec), "");
    EXPECT_EQ(IPV4(203, 0, 113, 7), rec.addr, "");
    EXPECT_EQ(50u, rec.ttl, "");

    /* the same chain with the address record missing */
    ASSERT_TRUE(pkt_question(7, "www.example.com", DNS_FLAGS_RESPONSE, 1), "");
    pkt_ptr(12);
    pkt_rr_cname(100, "cdn.example.net");

    EXPECT_EQ(ERR_NOT_FOUND, dns_parse_response(pkt, pkt_len, 7, "www.example.com", &rec), "");

    END_TEST;
}

static bool parse_negative_answers(void) {
    BEGIN_TEST;

    dns_record_t rec = {};

    /* no such name */
    ASSERT_TRUE(pkt_question(9, "nope.example.com", DNS_FLAGS_RESPONSE | 3, 0), "");
    EXPECT_EQ(ERR_NOT_FOUND, dns_parse_response(pkt, pkt_len, 9, "nope.example.com", &rec), "");

    /* server failure */
    ASSERT_TRUE(pkt_question(9, "nope.example.com", DNS_FLAGS_RESPONSE | 2, 0), "");
    EXPECT_EQ(ERR_IO, dns_parse_response(pkt, pkt_len, 9, "nope.example.com", &rec), "");

    /* a successful response with no answers at all */
    ASSERT_TRUE(pkt_question(9, "nope.example.com", DNS_FLAGS_RESPONSE, 0), "");
    EXPECT_EQ(ERR_NOT_FOUND, dns_parse_response(pkt, pkt_len, 9, "nope.example.com", &rec), "");

    /* an answer count larger than the records present */
    ASSERT_TRUE(pkt_question(9, "example.com", DNS_FLAGS_RESPONSE, 4), "");
    pkt_name("other.com");
    pkt_rr_a(60, IPV4(1, 2, 3, 4));
    EXPECT_EQ(ERR_NOT_VALID, dns_parse_response(pkt, pkt_len, 9, "example.com", &rec), "");

    END_TEST;
}

static bool parse_rejects_hostile(void) {
    BEGIN_TEST;

    dns_record_t rec = {};

    /* a well formed answer, which the mutations below start from */
    ASSERT_TRUE(pkt_question(0x1111, "example.com", DNS_FLAGS_RESPONSE, 1), "");
    pkt_ptr(12);
    pkt_rr_a(60, IPV4(1, 2, 3, 4));
    const size_t good_len = pkt_len;
    ASSERT_EQ(NO_ERROR, dns_parse_response(pkt, good_len, 0x1111, "example.com", &rec), "");

    /* somebody else's transaction */
    EXPECT_EQ(ERR_NOT_VALID, dns_parse_response(pkt, good_len, 0x2222, "example.com", &rec),
              "wrong id");
    /* an answer to a question we did not ask */
    EXPECT_EQ(ERR_NOT_VALID, dns_parse_response(pkt, good_len, 0x1111, "example.org", &rec),
              "wrong name");
    /* every truncation of a valid message must be rejected, not misread */
    for (size_t len = 0; len < good_len; len++) {
        status_t err = dns_parse_response(pkt, len, 0x1111, "example.com", &rec);
        EXPECT_LT(err, 0, "a truncated message must not parse");
    }

    /* a query rather than a response */
    pkt[2] &= (uint8_t)~0x80;
    EXPECT_EQ(ERR_NOT_VALID, dns_parse_response(pkt, good_len, 0x1111, "example.com", &rec), "");
    pkt[2] |= 0x80;

    /* more than one question */
    pkt[5] = 2;
    EXPECT_EQ(ERR_NOT_VALID, dns_parse_response(pkt, good_len, 0x1111, "example.com", &rec), "");
    pkt[5] = 1;

    /* rdlength running past the end of the message */
    pkt[good_len - 5] = 0xff;
    EXPECT_EQ(ERR_NOT_VALID, dns_parse_response(pkt, good_len, 0x1111, "example.com", &rec), "");

    /* a compression pointer to itself, and one pointing forwards: both are
     * loops, and the walker must refuse rather than spin
     */
    ASSERT_TRUE(pkt_question(0x1111, "example.com", DNS_FLAGS_RESPONSE, 1), "");
    size_t rr_name = pkt_len;
    pkt_ptr((uint16_t)rr_name);
    pkt_rr_a(60, IPV4(1, 2, 3, 4));
    EXPECT_LT(dns_parse_response(pkt, pkt_len, 0x1111, "example.com", &rec), 0,
              "self referencing pointer");

    ASSERT_TRUE(pkt_question(0x1111, "example.com", DNS_FLAGS_RESPONSE, 1), "");
    pkt_ptr((uint16_t)(pkt_len + 8));
    pkt_rr_a(60, IPV4(1, 2, 3, 4));
    EXPECT_LT(dns_parse_response(pkt, pkt_len, 0x1111, "example.com", &rec), 0,
              "forward pointer");

    /* a question name that is one long chain of pointers back into itself */
    ASSERT_TRUE(pkt_question(0x1111, "example.com", DNS_FLAGS_RESPONSE, 1), "");
    pkt[12] = 0xc0;
    pkt[13] = 12;
    EXPECT_LT(dns_parse_response(pkt, pkt_len, 0x1111, "example.com", &rec), 0, "");

    /* a reserved label type */
    ASSERT_TRUE(pkt_question(0x1111, "example.com", DNS_FLAGS_RESPONSE, 1), "");
    pkt[12] = 0x80;
    EXPECT_LT(dns_parse_response(pkt, pkt_len, 0x1111, "example.com", &rec), 0, "");

    /* a label that runs off the end of the message */
    ASSERT_TRUE(pkt_question(0x1111, "example.com", DNS_FLAGS_RESPONSE, 1), "");
    pkt[12] = 63;
    EXPECT_LT(dns_parse_response(pkt, pkt_len, 0x1111, "example.com", &rec), 0, "");

    EXPECT_EQ(ERR_INVALID_ARGS, dns_parse_response(NULL, 10, 1, "a.com", &rec), "");
    EXPECT_EQ(ERR_INVALID_ARGS, dns_parse_response(pkt, good_len, 1, NULL, &rec), "");
    EXPECT_EQ(ERR_INVALID_ARGS, dns_parse_response(pkt, good_len, 1, "a.com", NULL), "");

    END_TEST;
}

/* --- the resolver, with this test acting as the name server --- */

#define DNS_TEST_ANSWER IPV4(93, 184, 216, 34)

static struct {
    uint queries;
    uint ignore;            /* ignore this many queries before answering */
    uint32_t ttl;
    ipv4_addr_t answer;
} test_server;

static uint8_t server_reply[512];

/* Runs on the netstack thread. Echoes the question back with one A record
 * appended, which is the shortest legal answer.
 */
static void dns_server_cb(void *data, size_t len, uint32_t srcaddr, uint16_t srcport, void *arg) {
    test_server.queries++;

    if (test_server.ignore > 0) {
        test_server.ignore--;
        return;
    }
    if (len < 12 || len + 16 > sizeof(server_reply)) {
        return;
    }

    memcpy(server_reply, data, len);
    server_reply[2] |= 0x80;    /* response */
    server_reply[3] = 0x80;     /* recursion available, no error */
    server_reply[6] = 0;
    server_reply[7] = 1;        /* one answer */

    size_t pos = len;
    server_reply[pos++] = 0xc0; /* the answer's name points at the question */
    server_reply[pos++] = 12;
    server_reply[pos++] = 0;
    server_reply[pos++] = DNS_TYPE_A;
    server_reply[pos++] = 0;
    server_reply[pos++] = DNS_CLASS_IN;

    uint32_t ttl = htonl(test_server.ttl);
    memcpy(server_reply + pos, &ttl, sizeof(ttl));
    pos += sizeof(ttl);

    server_reply[pos++] = 0;
    server_reply[pos++] = 4;
    memcpy(server_reply + pos, &test_server.answer, 4);
    pos += 4;

    udp_socket_t *sock;
    if (udp_open(srcaddr, 53, srcport, &sock) == NO_ERROR) {
        udp_send(server_reply, pos, sock);
        udp_close(sock);
    }
}

static bool resolve_end_to_end(void) {
    BEGIN_TEST;

    testnetif_get();
    testnetif_configure(NULL);

    const ipv4_addr_t saved_server = minip_get_dns_server();

    memset(&test_server, 0, sizeof(test_server));
    test_server.ttl = 60;
    test_server.answer = DNS_TEST_ANSWER;

    ASSERT_EQ(0, udp_listen(53, dns_server_cb, NULL), "");
    minip_set_dns_server(IPV4(10, 99, 0, 1));
    dns_cache_flush();

    ipv4_addr_t addr = IPV4_NONE;
    EXPECT_EQ(NO_ERROR, dns_resolve("example.com", &addr, 2000), "");
    EXPECT_EQ(DNS_TEST_ANSWER, addr, "");
    EXPECT_EQ(1u, test_server.queries, "");

    /* the answer is cached for its ttl, case insensitively */
    addr = IPV4_NONE;
    EXPECT_EQ(NO_ERROR, dns_resolve("EXAMPLE.com", &addr, 2000), "");
    EXPECT_EQ(DNS_TEST_ANSWER, addr, "");
    EXPECT_EQ(1u, test_server.queries, "a cached answer should not hit the server");

    /* and flushing the cache asks again */
    dns_cache_flush();
    EXPECT_EQ(NO_ERROR, dns_resolve("example.com", &addr, 2000), "");
    EXPECT_EQ(2u, test_server.queries, "");

    /* a lost query is retransmitted */
    dns_cache_flush();
    test_server.ignore = 1;
    addr = IPV4_NONE;
    EXPECT_EQ(NO_ERROR, dns_resolve("example.com", &addr, 4000), "");
    EXPECT_EQ(DNS_TEST_ANSWER, addr, "");
    EXPECT_EQ(4u, test_server.queries, "the resolver should have retried once");

    /* a server that never answers times out without hanging the stack */
    dns_cache_flush();
    test_server.ignore = 99;
    EXPECT_EQ(ERR_TIMED_OUT, dns_resolve("example.com", &addr, 200), "");

    /* names that cannot be asked about fail before any packet is sent */
    uint queries = test_server.queries;
    test_server.ignore = 0;
    EXPECT_LT(dns_resolve("a..b", &addr, 1000), 0, "");
    EXPECT_EQ(queries, test_server.queries, "a malformed name should not be sent");
    EXPECT_EQ(ERR_INVALID_ARGS, dns_resolve("", &addr, 1000), "");
    EXPECT_EQ(ERR_INVALID_ARGS, dns_resolve(NULL, &addr, 1000), "");

    /* minip_resolve short circuits a literal address */
    dns_cache_flush();
    queries = test_server.queries;
    addr = IPV4_NONE;
    EXPECT_EQ(NO_ERROR, minip_resolve("10.1.2.3", &addr), "");
    EXPECT_EQ(IPV4(10, 1, 2, 3), addr, "");
    EXPECT_EQ(queries, test_server.queries, "a literal address needs no server");

    /* and falls through to the resolver for anything else */
    addr = IPV4_NONE;
    EXPECT_EQ(NO_ERROR, minip_resolve("example.com", &addr), "");
    EXPECT_EQ(DNS_TEST_ANSWER, addr, "");

    /* with no server configured there is nobody to ask */
    minip_set_dns_server(IPV4_NONE);
    dns_cache_flush();
    EXPECT_EQ(ERR_NOT_CONFIGURED, dns_resolve("example.com", &addr, 1000), "");

    minip_set_dns_server(saved_server);
    dns_cache_flush();
    EXPECT_EQ(0, udp_listen(53, NULL, NULL), "");

    END_TEST;
}

BEGIN_TEST_CASE(dns_tests)
RUN_TEST(build_query)
RUN_TEST(parse_answer)
RUN_TEST(parse_cname_chain)
RUN_TEST(parse_negative_answers)
RUN_TEST(parse_rejects_hostile)
RUN_TEST(resolve_end_to_end)
END_TEST_CASE(dns_tests)
