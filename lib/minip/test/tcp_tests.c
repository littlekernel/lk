/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* TCP unit tests. The happy paths run over plain loopback (127.0.0.1) so
 * they work on NIC-less targets; the fault runs push both endpoints of a
 * connection through the testnetif's drop/duplicate/reorder pipeline, and
 * the injection tests feed hand-built frames straight into the stack.
 * Nothing here waits for a configured interface.
 */

#include <lib/unittest.h>
#include <lib/minip.h>
#include <lib/pktbuf.h>
#include <kernel/thread.h>
#include <platform.h>
#include <lk/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../minip-internal.h"
#include "testnetif.h"

/* one listen port per test; TIME_WAIT sockets from earlier tests linger */
#define TCP_TEST_PORT_BASE 31000

#define TRANSFER_CHUNK 1000

/* deterministic pattern both ends can generate */
static void fill_pattern(uint8_t *buf, size_t len, uint32_t seed) {
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(seed + i * 7 + (i >> 8));
    }
}

static bool check_pattern(const uint8_t *buf, size_t len, uint32_t seed) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != (uint8_t)(seed + i * 7 + (i >> 8))) {
            return false;
        }
    }
    return true;
}

static ssize_t tcp_read_full(tcp_socket_t *s, uint8_t *buf, size_t len) {
    size_t pos = 0;
    while (pos < len) {
        ssize_t ret = tcp_read(s, buf + pos, len - pos);
        if (ret < 0) {
            return ret;
        }
        pos += ret;
    }
    return pos;
}

/* accepts one connection and echoes until the peer closes */
struct echo_server {
    tcp_socket_t *listener;
    volatile bool accepted;
    volatile bool saw_close;
    thread_t *thread;
};

static int echo_server_worker(void *arg) {
    struct echo_server *srv = (struct echo_server *)arg;

    tcp_socket_t *conn = NULL;
    status_t err = tcp_accept_timeout(srv->listener, &conn, 10000);
    if (err < 0) {
        return err;
    }
    srv->accepted = true;

    uint8_t *buf = malloc(TRANSFER_CHUNK);
    if (!buf) {
        tcp_close(conn);
        return ERR_NO_MEMORY;
    }

    for (;;) {
        ssize_t len = tcp_read(conn, buf, TRANSFER_CHUNK);
        if (len < 0) {
            srv->saw_close = true;
            break;
        }
        if (tcp_write(conn, buf, len) < 0) {
            srv->saw_close = true;
            break;
        }
    }

    free(buf);
    tcp_close(conn);
    return 0;
}

static bool echo_server_start(struct echo_server *srv, uint16_t port) {
    memset(srv, 0, sizeof(*srv));
    if (tcp_open_listen(&srv->listener, port) < 0) {
        return false;
    }
    srv->thread = thread_create("tcp test echo", &echo_server_worker, srv,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(srv->thread);
    return true;
}

static status_t echo_server_finish(struct echo_server *srv) {
    int retcode = -1;
    status_t err = thread_join(srv->thread, &retcode, 10000);
    tcp_close(srv->listener);
    if (err < 0) {
        return err;
    }
    return retcode;
}

/* write a chunk, read the echo back, compare; repeat to total bytes */
static bool echo_transfer(tcp_socket_t *sock, size_t total, size_t chunk, uint32_t seed) {
    uint8_t *txbuf = malloc(chunk);
    uint8_t *rxbuf = malloc(chunk);
    if (!txbuf || !rxbuf) {
        free(txbuf);
        free(rxbuf);
        return false;
    }

    bool ok = true;
    size_t pos = 0;
    while (pos < total && ok) {
        size_t n = MIN(chunk, total - pos);
        fill_pattern(txbuf, n, seed + pos);

        if (tcp_write(sock, txbuf, n) != (ssize_t)n) {
            ok = false;
            break;
        }
        if (tcp_read_full(sock, rxbuf, n) != (ssize_t)n) {
            ok = false;
            break;
        }
        if (!check_pattern(rxbuf, n, seed + pos)) {
            ok = false;
            break;
        }
        pos += n;
    }

    free(txbuf);
    free(rxbuf);
    return ok;
}

static bool loopback_echo_large(void) {
    BEGIN_TEST;

    struct echo_server srv;
    ASSERT_TRUE(echo_server_start(&srv, TCP_TEST_PORT_BASE + 0), "");

    tcp_socket_t *sock = NULL;
    ASSERT_EQ(NO_ERROR, tcp_connect(&sock, IPV4(127, 0, 0, 1), TCP_TEST_PORT_BASE + 0), "");

    /* bidirectional transfer spanning many segments and window updates */
    EXPECT_TRUE(echo_transfer(sock, 64 * 1024, TRANSFER_CHUNK, 0x1234), "echoed data should match");

    EXPECT_EQ(NO_ERROR, tcp_close(sock), "");
    EXPECT_EQ(NO_ERROR, echo_server_finish(&srv), "");
    EXPECT_TRUE(srv.saw_close, "server should see the close");

    END_TEST;
}

static bool loopback_small_writes(void) {
    BEGIN_TEST;

    struct echo_server srv;
    ASSERT_TRUE(echo_server_start(&srv, TCP_TEST_PORT_BASE + 1), "");

    tcp_socket_t *sock = NULL;
    ASSERT_EQ(NO_ERROR, tcp_connect(&sock, IPV4(127, 0, 0, 1), TCP_TEST_PORT_BASE + 1), "");

    /* a stream of tiny writes exercises tail coalescing on both queues */
    static uint8_t pattern[800];
    fill_pattern(pattern, sizeof(pattern), 0x77);
    for (size_t pos = 0; pos < sizeof(pattern); pos += 8) {
        ASSERT_EQ(8, tcp_write(sock, pattern + pos, 8), "");
    }

    static uint8_t rxbuf[800];
    ASSERT_EQ((ssize_t)sizeof(rxbuf), tcp_read_full(sock, rxbuf, sizeof(rxbuf)), "");
    EXPECT_BYTES_EQ(pattern, rxbuf, sizeof(pattern), "echoed data should match");

    EXPECT_EQ(NO_ERROR, tcp_close(sock), "");
    EXPECT_EQ(NO_ERROR, echo_server_finish(&srv), "");

    END_TEST;
}

/* the far side closes first: reads drain, then report the close */
static int close_server_worker(void *arg) {
    tcp_socket_t *listener = (tcp_socket_t *)arg;

    tcp_socket_t *conn = NULL;
    status_t err = tcp_accept_timeout(listener, &conn, 10000);
    if (err < 0) {
        return err;
    }

    static const char msg[] = "parting message";
    tcp_write(conn, msg, sizeof(msg));
    tcp_close(conn);
    return 0;
}

static bool remote_close(void) {
    BEGIN_TEST;

    tcp_socket_t *listener = NULL;
    ASSERT_EQ(NO_ERROR, tcp_open_listen(&listener, TCP_TEST_PORT_BASE + 2), "");

    thread_t *t = thread_create("tcp test closer", &close_server_worker, listener,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);

    tcp_socket_t *sock = NULL;
    ASSERT_EQ(NO_ERROR, tcp_connect(&sock, IPV4(127, 0, 0, 1), TCP_TEST_PORT_BASE + 2), "");

    /* data written before their FIN still arrives... */
    uint8_t buf[32];
    ASSERT_EQ((ssize_t)sizeof("parting message"), tcp_read_full(sock, buf, sizeof("parting message")), "");
    EXPECT_BYTES_EQ((const uint8_t *)"parting message", buf, sizeof("parting message"), "");

    /* ...and the next read reports the close */
    EXPECT_EQ(ERR_CHANNEL_CLOSED, tcp_read(sock, buf, sizeof(buf)), "");

    EXPECT_EQ(NO_ERROR, tcp_close(sock), "");

    int retcode = -1;
    EXPECT_EQ(NO_ERROR, thread_join(t, &retcode, 10000), "");
    EXPECT_EQ(0, retcode, "");
    tcp_close(listener);

    END_TEST;
}

static bool accept_timeout(void) {
    BEGIN_TEST;

    tcp_socket_t *listener = NULL;
    ASSERT_EQ(NO_ERROR, tcp_open_listen(&listener, TCP_TEST_PORT_BASE + 3), "");

    tcp_socket_t *conn = NULL;
    lk_time_t start = current_time();
    EXPECT_EQ(ERR_TIMED_OUT, tcp_accept_timeout(listener, &conn, 50), "");
    EXPECT_GE(current_time() - start, 50U, "");

    EXPECT_EQ(NO_ERROR, tcp_close(listener), "");

    END_TEST;
}

static bool duplicate_listen(void) {
    BEGIN_TEST;

    tcp_socket_t *a = NULL;
    tcp_socket_t *b = NULL;
    ASSERT_EQ(NO_ERROR, tcp_open_listen(&a, TCP_TEST_PORT_BASE + 4), "");
    EXPECT_EQ(ERR_ALREADY_EXISTS, tcp_open_listen(&b, TCP_TEST_PORT_BASE + 4), "");
    EXPECT_EQ(NO_ERROR, tcp_close(a), "");

    /* the port is free again once the listener is gone */
    ASSERT_EQ(NO_ERROR, tcp_open_listen(&b, TCP_TEST_PORT_BASE + 4), "");
    EXPECT_EQ(NO_ERROR, tcp_close(b), "");

    END_TEST;
}

/* --- fault runs over the testnetif ------------------------------------- */

/* connect to our own address on the testnetif, with a helper thread since
 * tcp_connect blocks */
struct connect_result {
    tcp_socket_t *sock;
    uint16_t port;
    status_t err;
};

static int connect_worker(void *arg) {
    struct connect_result *res = (struct connect_result *)arg;
    res->err = tcp_connect(&res->sock, IPV4(10, 99, 0, 1), res->port);
    return 0;
}

static bool syn_retransmit(void) {
    BEGIN_TEST;

    testnetif_get();

    struct echo_server srv;
    ASSERT_TRUE(echo_server_start(&srv, TCP_TEST_PORT_BASE + 5), "");

    /* black-hole the interface so the first SYN vanishes, then heal it and
     * let the retransmit complete the handshake */
    testnetif_faults_t faults = { .drop_every = 1 };
    testnetif_configure(&faults);

    struct connect_result res = { .port = TCP_TEST_PORT_BASE + 5, .err = 1 };
    thread_t *t = thread_create("tcp test connect", &connect_worker, &res,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);

    thread_sleep(100);
    EXPECT_GE(testnetif_dropped_count(), 1U, "the SYN should have been swallowed");
    testnetif_configure(NULL);

    ASSERT_EQ(NO_ERROR, thread_join(t, NULL, 15000), "");
    ASSERT_EQ(NO_ERROR, res.err, "connect should succeed via SYN retransmit");

    EXPECT_TRUE(echo_transfer(res.sock, 2 * TRANSFER_CHUNK, TRANSFER_CHUNK, 0x51), "");

    EXPECT_EQ(NO_ERROR, tcp_close(res.sock), "");
    EXPECT_EQ(NO_ERROR, echo_server_finish(&srv), "");

    END_TEST;
}

/* establish cleanly, run the transfer with a fault pipeline, heal before
 * closing so the (unretransmitted) FIN handshake stays reliable */
static bool transfer_with_faults(const testnetif_faults_t *faults, size_t total, uint16_t port) {
    testnetif_get();

    struct echo_server srv;
    if (!echo_server_start(&srv, port)) {
        return false;
    }

    struct connect_result res = { .port = port, .err = 1 };
    thread_t *t = thread_create("tcp test connect", &connect_worker, &res,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);
    if (thread_join(t, NULL, 15000) < 0 || res.err < 0) {
        echo_server_finish(&srv);
        return false;
    }

    testnetif_configure(faults);
    bool ok = echo_transfer(res.sock, total, TRANSFER_CHUNK, 0xfa);
    testnetif_configure(NULL);

    ok = (tcp_close(res.sock) == NO_ERROR) && ok;
    ok = (echo_server_finish(&srv) == NO_ERROR) && ok;
    return ok;
}

static bool data_retransmit(void) {
    BEGIN_TEST;

    /* every 5th frame (data and acks alike) vanishes; the retransmit timer
     * has to recover each loss */
    const testnetif_faults_t faults = { .drop_every = 5 };
    EXPECT_TRUE(transfer_with_faults(&faults, 4 * TRANSFER_CHUNK, TCP_TEST_PORT_BASE + 6),
                "transfer should survive dropped frames");

    END_TEST;
}

static bool reordered_segments(void) {
    BEGIN_TEST;

    /* pairwise-swapped delivery: segments arrive out of order, exercising
     * the receive queue's island insert and merge paths */
    const testnetif_faults_t faults = { .reorder_pairs = true };
    EXPECT_TRUE(transfer_with_faults(&faults, 4 * TRANSFER_CHUNK, TCP_TEST_PORT_BASE + 7),
                "transfer should survive reordered frames");

    END_TEST;
}

static bool duplicated_segments(void) {
    BEGIN_TEST;

    /* every 3rd frame is delivered twice: pure duplicates must be trimmed
     * away and dup-acked, never delivered twice to the reader */
    const testnetif_faults_t faults = { .dup_every = 3 };
    EXPECT_TRUE(transfer_with_faults(&faults, 4 * TRANSFER_CHUNK, TCP_TEST_PORT_BASE + 8),
                "transfer should survive duplicated frames");

    END_TEST;
}

/* a server that reads to EOF into one buffer, so a test can check that the
 * whole stream survived the peer's close */
struct sink_server {
    tcp_socket_t *listener;
    uint8_t *buf;
    size_t buflen;
    volatile size_t received;
    volatile status_t err;
    thread_t *thread;
};

static int sink_server_worker(void *arg) {
    struct sink_server *srv = (struct sink_server *)arg;

    tcp_socket_t *conn = NULL;
    status_t err = tcp_accept_timeout(srv->listener, &conn, 10000);
    if (err < 0) {
        srv->err = err;
        return err;
    }

    while (srv->received < srv->buflen) {
        ssize_t len = tcp_read(conn, srv->buf + srv->received, srv->buflen - srv->received);
        if (len < 0) {
            break;
        }
        srv->received += len;
    }

    srv->err = NO_ERROR;
    tcp_close(conn);
    return 0;
}

static bool close_with_unacked_data(void) {
    BEGIN_TEST;

    testnetif_get();

    const uint16_t port = TCP_TEST_PORT_BASE + 9;
    const size_t total = 8 * TRANSFER_CHUNK;

    uint8_t *expected = (uint8_t *)malloc(total);
    ASSERT_NONNULL(expected, "");

    struct sink_server srv;
    memset(&srv, 0, sizeof(srv));
    srv.buf = (uint8_t *)malloc(total);
    srv.buflen = total;
    if (!srv.buf) {
        free(expected);
        ASSERT_NONNULL(srv.buf, "");
    }

    for (size_t pos = 0; pos < total; pos += TRANSFER_CHUNK) {
        fill_pattern(expected + pos, TRANSFER_CHUNK, 0xc1 + pos);
    }

    ASSERT_EQ(NO_ERROR, tcp_open_listen(&srv.listener, port), "");
    srv.thread = thread_create("tcp test sink", &sink_server_worker, &srv,
                               DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(srv.thread);

    struct connect_result res = { .port = port, .err = 1 };
    thread_t *t = thread_create("tcp test connect", &connect_worker, &res,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);
    ASSERT_EQ(NO_ERROR, thread_join(t, NULL, 15000), "");
    ASSERT_EQ(NO_ERROR, res.err, "");

    /* swap delivery order so the FIN can overtake data still in flight: a
     * FIN sequenced anywhere but after the last byte would truncate the
     * stream, and one the peer discards has to be retransmitted */
    const testnetif_faults_t faults = { .reorder_pairs = true };
    testnetif_configure(&faults);

    for (size_t pos = 0; pos < total; pos += TRANSFER_CHUNK) {
        ASSERT_EQ((ssize_t)TRANSFER_CHUNK, tcp_write(res.sock, expected + pos, TRANSFER_CHUNK), "");
    }

    /* close with data still unacknowledged */
    EXPECT_EQ(NO_ERROR, tcp_close(res.sock), "");

    EXPECT_EQ(NO_ERROR, thread_join(srv.thread, NULL, 15000), "");
    testnetif_configure(NULL);

    EXPECT_EQ(total, srv.received, "the whole stream should arrive before the close");
    EXPECT_BYTES_EQ(expected, srv.buf, total, "");

    tcp_close(srv.listener);
    free(expected);
    free(srv.buf);

    END_TEST;
}

/* Lose one data segment out of a burst and let the duplicate acks the
 * rest provoke drive the recovery, rather than the retransmit timer.
 */
static bool fast_retransmit(void) {
    BEGIN_TEST;

    testnetif_get();

    const uint16_t port = TCP_TEST_PORT_BASE + 10;
    /* enough segments behind the hole for three duplicate acks */
    const size_t total = 8 * TRANSFER_CHUNK;

    uint8_t *expected = (uint8_t *)malloc(total);
    ASSERT_NONNULL(expected, "");

    struct sink_server srv;
    memset(&srv, 0, sizeof(srv));
    srv.buf = (uint8_t *)malloc(total);
    srv.buflen = total;
    if (!srv.buf) {
        free(expected);
        ASSERT_NONNULL(srv.buf, "");
    }
    fill_pattern(expected, total, 0x2b);

    ASSERT_EQ(NO_ERROR, tcp_open_listen(&srv.listener, port), "");
    srv.thread = thread_create("tcp test sink", &sink_server_worker, &srv,
                               DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(srv.thread);

    struct connect_result res = { .port = port, .err = 1 };
    thread_t *t = thread_create("tcp test connect", &connect_worker, &res,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);
    ASSERT_EQ(NO_ERROR, thread_join(t, NULL, 15000), "");
    ASSERT_EQ(NO_ERROR, res.err, "");

    /* let the handshake's delayed ack drain so the next sizable frame on
     * the wire is our first data segment */
    thread_sleep(2 * 50);

    /* swallow it; everything behind it still arrives, out of order */
    const testnetif_faults_t faults = { .drop_once_min_len = 512 };
    testnetif_configure(&faults);

    ASSERT_EQ((ssize_t)total, tcp_write(res.sock, expected, total), "");

    EXPECT_EQ(NO_ERROR, thread_join(srv.thread, NULL, 15000), "");

    tcp_socket_stats_t stats;
    tcp_get_socket_stats(res.sock, &stats);

    EXPECT_EQ(1U, testnetif_dropped_count(), "exactly one segment should be lost");
    EXPECT_GE(stats.dupacks, (uint32_t)3, "the segments behind the hole should dup-ack");
    EXPECT_EQ(1U, stats.fast_retransmits, "the hole should be filled on the third dupack");
    EXPECT_EQ(0U, stats.retransmits, "the retransmit timer should not have been needed");

    testnetif_configure(NULL);

    EXPECT_EQ(total, srv.received, "the whole stream should arrive");
    EXPECT_BYTES_EQ(expected, srv.buf, total, "");

    EXPECT_EQ(NO_ERROR, tcp_close(res.sock), "");
    tcp_close(srv.listener);
    free(expected);
    free(srv.buf);

    END_TEST;
}

static bool wait_for_bytes(struct sink_server *srv, size_t n, lk_time_t timeout) {
    for (lk_time_t waited = 0; waited <= timeout; waited += 10) {
        if (srv->received >= n) {
            return true;
        }
        thread_sleep(10);
    }
    return false;
}

/* Watch the congestion window itself rather than throughput: both ends of
 * this connection are in one kernel with no path between them, so a
 * window that opens and collapses correctly looks exactly like one that
 * does nothing at all from the outside.
 */
static bool congestion_window(void) {
    BEGIN_TEST;

    testnetif_get();
    testnetif_configure(NULL);

    const uint16_t port = TCP_TEST_PORT_BASE + 11;
    const size_t clean = 6 * TRANSFER_CHUNK;
    const size_t lossy = 8 * TRANSFER_CHUNK;
    const size_t total = clean + lossy;

    uint8_t *expected = (uint8_t *)malloc(total);
    ASSERT_NONNULL(expected, "");

    struct sink_server srv;
    memset(&srv, 0, sizeof(srv));
    srv.buf = (uint8_t *)malloc(total);
    srv.buflen = total;
    if (!srv.buf) {
        free(expected);
        ASSERT_NONNULL(srv.buf, "");
    }
    fill_pattern(expected, total, 0x9d);

    ASSERT_EQ(NO_ERROR, tcp_open_listen(&srv.listener, port), "");
    srv.thread = thread_create("tcp test sink", &sink_server_worker, &srv,
                               DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(srv.thread);

    struct connect_result res = { .port = port, .err = 1 };
    thread_t *t = thread_create("tcp test connect", &connect_worker, &res,
                                DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(t);
    ASSERT_EQ(NO_ERROR, thread_join(t, NULL, 15000), "");
    ASSERT_EQ(NO_ERROR, res.err, "");

    /* a fresh connection starts at the initial window (RFC 3390) with no
     * threshold, so it is free to open up in slow start */
    tcp_socket_stats_t fresh;
    tcp_get_socket_stats(res.sock, &fresh);
    EXPECT_GE(fresh.cwnd, 2 * fresh.mss, "initial window should be a few segments");
    EXPECT_LE(fresh.cwnd, 4 * fresh.mss, "initial window should be a few segments");
    EXPECT_GT(fresh.ssthresh, 64u * 1024, "nothing has gone wrong yet");

    /* a clean transfer: slow start should open the window */
    ASSERT_EQ((ssize_t)clean, tcp_write(res.sock, expected, clean), "");
    EXPECT_TRUE(wait_for_bytes(&srv, clean, 10000), "");
    thread_sleep(2 * 50); /* let the last acks land */

    tcp_socket_stats_t opened;
    tcp_get_socket_stats(res.sock, &opened);
    EXPECT_GT(opened.cwnd, fresh.cwnd, "slow start should have opened the window");
    EXPECT_EQ(fresh.ssthresh, opened.ssthresh, "no loss, so no threshold yet");

    /* now lose a segment: the recovery should halve the window */
    const testnetif_faults_t faults = { .drop_once_min_len = 512 };
    testnetif_configure(&faults);

    ASSERT_EQ((ssize_t)lossy, tcp_write(res.sock, expected + clean, lossy), "");
    EXPECT_TRUE(wait_for_bytes(&srv, total, 10000), "");
    thread_sleep(2 * 50);

    tcp_socket_stats_t after;
    tcp_get_socket_stats(res.sock, &after);
    EXPECT_EQ(1U, after.fast_retransmits, "the loss should be recovered from dupacks");
    EXPECT_EQ(0U, after.retransmits, "and not from the retransmit timer");
    EXPECT_LT(after.ssthresh, opened.cwnd, "the loss should have set a threshold below it");
    EXPECT_GE(after.ssthresh, 2 * after.mss, "but never below two segments");
    EXPECT_LE(after.cwnd, opened.cwnd, "and the window should have come back down");

    EXPECT_EQ(NO_ERROR, thread_join(srv.thread, NULL, 15000), "");
    testnetif_configure(NULL);

    EXPECT_EQ(total, srv.received, "the whole stream should arrive");
    EXPECT_BYTES_EQ(expected, srv.buf, total, "");

    EXPECT_EQ(NO_ERROR, tcp_close(res.sock), "");
    tcp_close(srv.listener);
    free(expected);
    free(srv.buf);

    END_TEST;
}

/* --- raw frame injection ----------------------------------------------- */

/* the wire header layout (the stack's own struct is private to tcp.cpp) */
typedef struct tcp_header_inject {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t length_flags;
    uint16_t win_size;
    uint16_t checksum;
    uint16_t urg_pointer;
} __PACKED tcp_header_inject_t;

struct inject_frame {
    struct eth_hdr eth;
    struct ipv4_hdr ip;
    tcp_header_inject_t tcp;
} __PACKED;

static void build_tcp_frame(struct inject_frame *f, ipv4_addr_t src_ip, uint16_t src_port,
                            uint16_t dst_port, uint16_t flags, uint32_t seq) {
    netif_t *n = testnetif_get();

    memset(f, 0, sizeof(*f));
    mac_addr_copy(f->eth.dst_mac, n->mac_address);
    mac_addr_copy(f->eth.src_mac, testnetif_peer_mac());
    f->eth.type = htons(ETH_TYPE_IPV4);

    f->ip.ver_ihl = 0x45;
    f->ip.len = htons(sizeof(f->ip) + sizeof(f->tcp));
    f->ip.flags_frags = 0x40;
    f->ip.ttl = 64;
    f->ip.proto = IP_PROTO_TCP;
    f->ip.src_addr = src_ip;
    f->ip.dst_addr = n->ipv4_addr;
    f->ip.chksum = ~ones_sum16(0, &f->ip, sizeof(f->ip));

    f->tcp.source_port = htons(src_port);
    f->tcp.dest_port = htons(dst_port);
    f->tcp.seq_num = htonl(seq);
    f->tcp.length_flags = htons((uint16_t)((sizeof(f->tcp) / 4) << 12 | flags));
    f->tcp.win_size = htons(4096);

    ipv4_pseudo_header_t pheader = {
        .source_addr = f->ip.src_addr,
        .dest_addr = f->ip.dst_addr,
        .zero = 0,
        .protocol = IP_PROTO_TCP,
        .tcp_length = htons(sizeof(f->tcp)),
    };
    f->tcp.checksum = cksum_pheader(&pheader, &f->tcp, sizeof(f->tcp));
}

/* scan the capture ring for a TCP frame to dst_mac with the given flags */
static bool wait_for_tcp_frame(const uint8_t *dst_mac, uint16_t flags, lk_time_t timeout) {
    for (lk_time_t waited = 0; waited <= timeout; waited += 10) {
        for (uint idx = 0; idx < TESTNETIF_CAPTURE_FRAMES; idx++) {
            uint8_t frame[TESTNETIF_CAPTURE_BYTES];
            size_t len = testnetif_capture_get(idx, frame, sizeof(frame));
            if (len < sizeof(struct inject_frame)) {
                continue;
            }
            const struct inject_frame *f = (const struct inject_frame *)frame;
            if (memcmp(f->eth.dst_mac, dst_mac, 6) != 0 ||
                    ntohs(f->eth.type) != ETH_TYPE_IPV4 ||
                    f->ip.proto != IP_PROTO_TCP) {
                continue;
            }
            if ((ntohs(f->tcp.length_flags) & 0x3f) == flags) {
                return true;
            }
        }
        thread_sleep(10);
    }
    return false;
}

static bool rst_on_closed_port(void) {
    BEGIN_TEST;

    netif_t *n = testnetif_get();
    (void)n;
    testnetif_configure(NULL);

    /* seed the fake peer's mac so the RST doesn't detour through ARP */
    const ipv4_addr_t peer_ip = IPV4(10, 99, 0, 99);
    arp_cache_update(peer_ip, testnetif_peer_mac());

    /* a SYN to a port nothing listens on must draw an RST */
    struct inject_frame f;
    build_tcp_frame(&f, peer_ip, 4321, TCP_TEST_PORT_BASE + 99, 0x02 /* SYN */, 1000);
    ASSERT_EQ(NO_ERROR, testnetif_inject(&f, sizeof(f)), "");

    EXPECT_TRUE(wait_for_tcp_frame(testnetif_peer_mac(), 0x04 /* RST */, 2000),
                "a RST should have been transmitted");

    END_TEST;
}

static bool ipv4_input_validation(void) {
    BEGIN_TEST;

    testnetif_get();

    pktbuf_stats_t before;
    pktbuf_get_stats(&before);

    /* hostile inputs: none may crash the stack or leak a buffer */
    struct inject_frame f;

    /* truncated: an ipv4 header that doesn't fit the frame */
    build_tcp_frame(&f, IPV4(10, 99, 0, 98), 1, 2, 0x02, 1);
    ASSERT_EQ(NO_ERROR, testnetif_inject(&f, sizeof(f.eth) + 4), "");

    /* wrong IP version */
    build_tcp_frame(&f, IPV4(10, 99, 0, 98), 1, 2, 0x02, 1);
    f.ip.ver_ihl = 0x65;
    ASSERT_EQ(NO_ERROR, testnetif_inject(&f, sizeof(f)), "");

    /* corrupted header checksum */
    build_tcp_frame(&f, IPV4(10, 99, 0, 98), 1, 2, 0x02, 1);
    f.ip.chksum ^= 0xffff;
    ASSERT_EQ(NO_ERROR, testnetif_inject(&f, sizeof(f)), "");

    /* ip length larger than the buffer that arrived */
    build_tcp_frame(&f, IPV4(10, 99, 0, 98), 1, 2, 0x02, 1);
    f.ip.len = htons(1400);
    f.ip.chksum = 0;
    f.ip.chksum = ~ones_sum16(0, &f.ip, sizeof(f.ip));
    ASSERT_EQ(NO_ERROR, testnetif_inject(&f, sizeof(f)), "");

    /* corrupted TCP checksum on an otherwise valid frame */
    build_tcp_frame(&f, IPV4(10, 99, 0, 98), 1, 2, 0x02, 1);
    f.tcp.checksum ^= 0xffff;
    ASSERT_EQ(NO_ERROR, testnetif_inject(&f, sizeof(f)), "");

    /* give the worker a moment to chew through them */
    thread_sleep(50);

    /* the stack is still alive: a loopback connection works */
    struct echo_server srv;
    ASSERT_TRUE(echo_server_start(&srv, TCP_TEST_PORT_BASE + 9), "");
    tcp_socket_t *sock = NULL;
    ASSERT_EQ(NO_ERROR, tcp_connect(&sock, IPV4(127, 0, 0, 1), TCP_TEST_PORT_BASE + 9), "");
    EXPECT_TRUE(echo_transfer(sock, TRANSFER_CHUNK, TRANSFER_CHUNK, 0xab), "");
    EXPECT_EQ(NO_ERROR, tcp_close(sock), "");
    EXPECT_EQ(NO_ERROR, echo_server_finish(&srv), "");

    /* every injected frame was freed */
    for (int i = 0; i < 300; i++) {
        pktbuf_stats_t after;
        pktbuf_get_stats(&after);
        if (after.bufs_free >= before.bufs_free) {
            break;
        }
        thread_sleep(10);
    }
    pktbuf_stats_t after;
    pktbuf_get_stats(&after);
    EXPECT_GE(after.bufs_free, before.bufs_free, "no pool buffers may leak");

    END_TEST;
}

BEGIN_TEST_CASE(tcp_tests)
RUN_TEST(loopback_echo_large)
RUN_TEST(loopback_small_writes)
RUN_TEST(remote_close)
RUN_TEST(accept_timeout)
RUN_TEST(duplicate_listen)
RUN_TEST(syn_retransmit)
RUN_TEST(data_retransmit)
RUN_TEST(reordered_segments)
RUN_TEST(duplicated_segments)
RUN_TEST(close_with_unacked_data)
RUN_TEST(fast_retransmit)
RUN_TEST(congestion_window)
RUN_TEST(rst_on_closed_port)
RUN_TEST(ipv4_input_validation)
END_TEST_CASE(tcp_tests)
