/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "minip-internal.h"

#include <lk/trace.h>
#include <assert.h>
#include <lk/compiler.h>
#include <stdlib.h>
#include <lk/err.h>
#include <string.h>
#include <sys/types.h>
#include <lk/console_cmd.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <arch/ops.h>
#include <platform.h>
#include <arch/atomic.h>

#define LOCAL_TRACE 0

typedef struct tcp_header {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t length_flags;
    uint16_t win_size;
    uint16_t checksum;
    uint16_t urg_pointer;
} __PACKED tcp_header_t;

typedef struct tcp_mss_option {
    uint8_t kind; /* 0x2 */
    uint8_t len;  /* 0x4 */
    uint16_t mss;
} __PACKED tcp_mss_option_t;

typedef enum tcp_state {
    STATE_CLOSED,
    STATE_LISTEN,
    STATE_SYN_SENT,
    STATE_SYN_RCVD,
    STATE_ESTABLISHED,
    STATE_CLOSE_WAIT,
    STATE_LAST_ACK,
    STATE_CLOSING,
    STATE_FIN_WAIT_1,
    STATE_FIN_WAIT_2,
    STATE_TIME_WAIT
} tcp_state_t;

typedef enum tcp_flags {
    PKT_FIN = 1,
    PKT_SYN = 2,
    PKT_RST = 4,
    PKT_PSH = 8,
    PKT_ACK = 16,
    PKT_URG = 32
} tcp_flags_t;

static inline tcp_flags_t operator|(tcp_flags_t a, tcp_flags_t b) {
    return (tcp_flags_t)((unsigned)a | (unsigned)b);
}

typedef struct tcp_socket {
    struct list_node node;

    mutex_t lock;
    volatile int ref;

    tcp_state_t state;
    ipv4_addr_t local_ip;
    ipv4_addr_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    ipv4_route_t *route;

    uint32_t mss;

    /* rx */
    uint32_t rx_win_size;
    uint32_t rx_win_low;
    uint32_t rx_win_high;
    struct list_node rx_queue; // pktbufs sorted by p->seq, the in-order run at the head
    uint32_t rx_contig_bytes;  // unread in-order payload bytes at the head of rx_queue
    uint32_t rx_ooo_count;     // pktbufs queued beyond the in-order run
    event_t  rx_event;
    int      rx_full_mss_count; // number of packets we have received in a row with a full mss
    net_timer_t ack_delay_timer;

    /* tx */
    uint32_t tx_win_low;  // low side of the acked window
    uint32_t tx_win_high; // tx_win_low + their advertised window size
    uint32_t tx_highest_seq; // highest sequence we have txed them
    struct list_node tx_queue; // unacked/unsent payload pktbufs; p->seq (immutable) = first byte
    uint32_t tx_buf_top;  // one past the last queued sequence (valid while tx_queue is non-empty)
    uint32_t tx_pktbuf_count;
    event_t  tx_event;
    net_timer_t retransmit_timer;
    int      retransmit_count; // consecutive retransmits with no ack in between
    uint32_t dupack_count;   // consecutive duplicate acks from the peer

    /* congestion control (RFC 5681, with NewReno's recovery from 6582) */
    uint32_t cwnd;           // how much may be in flight, bytes
    uint32_t cwnd_extra;     // limited transmit's allowance on top of cwnd
    uint32_t ssthresh;       // above this, growth slows to congestion avoidance
    uint32_t recover;        // sequence that ends the current loss episode
    bool     in_recovery;    // a fast recovery episode is under way
    bool     fin_pending;    // tcp_close() has queued a FIN behind the tx data
    bool     fin_sent;       // the FIN has been transmitted at least once
    uint32_t fin_seq;        // the sequence the FIN occupies (valid if fin_sent)

    /* round trip estimate, all in milliseconds (RFC 6298) */
    uint32_t srtt;           // smoothed round trip time
    uint32_t rttvar;         // its mean deviation
    uint32_t rto;            // current retransmit timeout, backed off in place
    bool     rtt_valid;      // srtt/rttvar hold a real measurement
    bool     rtt_pending;    // a segment is being timed right now
    uint32_t rtt_seq;        // the sequence being timed; an ack past it ends it
    lk_time_t rtt_start;     // when that segment went out

    /* counters, read out through tcp_get_socket_stats() */
    uint32_t stat_retransmits;
    uint32_t stat_fast_retransmits;
    uint32_t stat_dupacks;

    /* listen accept */
    semaphore_t accept_sem;
    struct tcp_socket *accepted;

    net_timer_t time_wait_timer;

    /* connect waiting */
    event_t connect_event;
} tcp_socket_t;

#define DEFAULT_MSS (1460)
#define DEFAULT_RX_WINDOW_SIZE MIN(8192u, (uint32_t)PKTBUF_POOL_SIZE * PKTBUF_SIZE / 8)
#define DEFAULT_TX_BUFFER_SIZE (8192)

/* Received data is kept in the pktbufs it arrived in, so a peer sending
 * tiny or scattered segments could otherwise pin an outsized share of the
 * pktbuf pool: cap the out-of-order pktbufs a socket may hold and copy
 * small in-order segments into the tail of the previous buffer instead of
 * queueing another one. The cap scales off the configured pool size.
 */
#define TCP_RX_OOO_CAP        MAX(2, MIN(8, PKTBUF_POOL_SIZE / 32))
#define TCP_RX_COALESCE_SIZE  (128)

/* Sent data stays queued in pktbufs too (each holding one segment's payload
 * behind PKTBUF_MAX_HDR of headroom), bounded both by bytes and by a pktbuf
 * count scaled off the pool size.
 */
#define TCP_TX_MAX_PKTBUFS    MAX(2, MIN(8, PKTBUF_POOL_SIZE / 32))

/* Retransmit timeout bounds and the clock granularity the estimator
 * assumes, in milliseconds. RFC 6298 nominates a 1s floor; like most
 * implementations we use a much lower one, since the round trips this
 * stack sees are typically well under a millisecond. The ceiling and the
 * retry count together bound how long a dead peer is chased: from a
 * settled 200ms the backoff runs 200/400/800/1600/3200/6400, so a
 * connection is declared dead after about 12s.
 */
#define TCP_RTO_INITIAL (1000)
#define TCP_RTO_MIN (200)
#define TCP_RTO_MAX (16000)
#define TCP_RTO_GRANULARITY (10)
#define TCP_MAX_RETRANSMITS (6)

/* Duplicate acks that mean a segment was lost rather than merely delayed
 * (RFC 5681 3.2). Three is the long standing value: it is enough that
 * plain reordering of adjacent segments does not trip it.
 */
#define TCP_DUPACK_THRESHOLD (3)

/* An upper bound on the congestion window. Nothing here can usefully keep
 * more in flight than the peer's window and our own transmit queue allow,
 * so this only keeps an idle-but-growing window from running away.
 */
#define TCP_CWND_MAX (1u << 20)

/* The handshake gets its own retry count: it starts from the initial 1s
 * timeout with nothing measured yet, so 4 tries is already ~15 seconds.
 */
#define SYN_RETRANSMIT_RETRIES (4)
#define DELAYED_ACK_TIMEOUT (50)
#define TIME_WAIT_TIMEOUT (60000) // 1 minute

#define FORCE_TCP_CHECKSUM (false)

#define SEQUENCE_GTE(a, b) ((int32_t)((a) - (b)) >= 0)
#define SEQUENCE_LTE(a, b) ((int32_t)((a) - (b)) <= 0)
#define SEQUENCE_GT(a, b) ((int32_t)((a) - (b)) > 0)
#define SEQUENCE_LT(a, b) ((int32_t)((a) - (b)) < 0)

/* Recompute the retransmit timeout from the current estimate, which also
 * discards any backoff that had been applied to it.
 */
static void tcp_update_rto(tcp_socket_t *s) {
    uint32_t var = MAX((uint32_t)TCP_RTO_GRANULARITY, 4 * s->rttvar);
    uint32_t rto = s->srtt + var;

    s->rto = MAX((uint32_t)TCP_RTO_MIN, MIN(rto, (uint32_t)TCP_RTO_MAX));
}

/* Fold one round trip measurement into the estimate (RFC 6298 2.2/2.3) */
static void tcp_rtt_sample(tcp_socket_t *s, uint32_t rtt) {
    if (!s->rtt_valid) {
        s->srtt = rtt;
        s->rttvar = rtt / 2;
        s->rtt_valid = true;
    } else {
        uint32_t delta = (rtt > s->srtt) ? (rtt - s->srtt) : (s->srtt - rtt);
        s->rttvar = (3 * s->rttvar + delta) / 4;
        s->srtt = (7 * s->srtt + rtt) / 8;
    }

    LTRACEF("s %p, rtt %u -> srtt %u rttvar %u\n", s, rtt, s->srtt, s->rttvar);

    tcp_update_rto(s);
}

/* Start timing a segment, if one is not already being timed. */
static void tcp_rtt_start(tcp_socket_t *s, uint32_t sequence) {
    if (s->rtt_pending)
        return;

    s->rtt_pending = true;
    s->rtt_seq = sequence;
    s->rtt_start = current_time();
}

/* An ack arrived: if it covers the segment being timed, that is a sample.
 * Karn's algorithm keeps this honest -- a retransmit abandons the
 * measurement outright rather than risk attributing the peer's ack of the
 * original transmission to the retransmitted one.
 */
static void tcp_rtt_ack(tcp_socket_t *s, uint32_t sequence) {
    if (!s->rtt_pending || !SEQUENCE_GT(sequence, s->rtt_seq))
        return;

    s->rtt_pending = false;
    tcp_rtt_sample(s, (uint32_t)(current_time() - s->rtt_start));
}

static mutex_t tcp_socket_list_lock = MUTEX_INITIAL_VALUE(tcp_socket_list_lock);
static struct list_node tcp_socket_list = LIST_INITIAL_VALUE(tcp_socket_list);

static bool tcp_debug = false;

/* local routines */
static tcp_socket_t *lookup_socket(ipv4_addr_t remote_ip, ipv4_addr_t local_ip, uint16_t remote_port, uint16_t local_port);
static void add_socket_to_list(tcp_socket_t *s);
static void remove_socket_from_list(tcp_socket_t *s);
static tcp_socket_t *create_tcp_socket(void);
static status_t tcp_send(ipv4_addr_t dest_ip, uint16_t dest_port, ipv4_addr_t src_ip, uint16_t src_port,
                         const iovec_t *iov, size_t iov_cnt,
                         tcp_flags_t flags, const void *options, size_t options_length,
                         uint32_t ack, uint32_t sequence, uint16_t window_size);
static status_t tcp_socket_send(tcp_socket_t *s, const iovec_t *iov, size_t iov_cnt,
                                tcp_flags_t flags, const void *options, size_t options_length, uint32_t sequence);
static bool handle_data(tcp_socket_t *s, pktbuf_t *p, uint32_t sequence);
static void send_ack(tcp_socket_t *s);
static void handle_ack(tcp_socket_t *s, uint32_t sequence, uint32_t win_size, bool bare);
static ssize_t tcp_retransmit(tcp_socket_t *s);
static void tcp_fast_retransmit(tcp_socket_t *s);
static void tcp_cwnd_init(tcp_socket_t *s);
static void tcp_enter_recovery(tcp_socket_t *s);
static ssize_t tcp_write_pending_data(tcp_socket_t *s);
static uint16_t tcp_advertised_window(tcp_socket_t *s);
static status_t tcp_send_data_pktbuf(tcp_socket_t *s, pktbuf_t *p, uint32_t sequence);
static status_t tcp_send_queued_segment(tcp_socket_t *s, pktbuf_t *q, uint32_t plen);
static status_t tcp_send_segment_copy(tcp_socket_t *s, pktbuf_t *q, uint32_t start, uint32_t len);
static void handle_retransmit_timeout(void *_s);
static void tcp_send_syn(tcp_socket_t *s, bool with_ack);
static void handle_time_wait_timeout(void *_s);
static void handle_delayed_ack_timeout(void *_s);
static void tcp_remote_close(tcp_socket_t *s);
static void tcp_abort(tcp_socket_t *s);
static void tcp_wakeup_waiters(tcp_socket_t *s);
static void inc_socket_ref(tcp_socket_t *s);
static bool dec_socket_ref(tcp_socket_t *s);

/* Payload length of a queued tx pktbuf, derived from its neighbors: a
 * segment runs from its own (immutable) seq to the next segment's, the tail
 * to tx_buf_top. data/dlen of queued pktbufs carry no state between
 * transmits.
 */
static uint32_t tx_seg_len(tcp_socket_t *s, pktbuf_t *q) {
    pktbuf_t *next = list_next_type(&s->tx_queue, &q->list, pktbuf_t, list);
    return (next ? next->seq : s->tx_buf_top) - q->seq;
}

static uint32_t tx_buffered_bytes(tcp_socket_t *s) {
    return list_is_empty(&s->tx_queue) ? 0 : s->tx_buf_top - s->tx_win_low;
}

/* One past the last queued data byte, which is where a FIN sits. An empty
 * queue means everything written has been acked, so the top is tx_win_low.
 */
static uint32_t tx_data_top(tcp_socket_t *s) {
    return list_is_empty(&s->tx_queue) ? s->tx_win_low : s->tx_buf_top;
}

/* true once the peer has acked the FIN we sent */
static bool tcp_fin_acked(tcp_socket_t *s) {
    return s->fin_sent && SEQUENCE_GT(s->tx_win_low, s->fin_seq);
}

static uint32_t tx_seg_cap(tcp_socket_t *s) {
    return MIN(s->mss, (uint32_t)PKTBUF_MAX_DATA);
}

static bool tx_queue_has_room(tcp_socket_t *s) {
    if (tx_buffered_bytes(s) >= DEFAULT_TX_BUFFER_SIZE)
        return false;
    if (s->tx_pktbuf_count < (uint32_t)TCP_TX_MAX_PKTBUFS)
        return true;
    /* every pktbuf slot is used, but the tail segment may still take bytes */
    pktbuf_t *tail = list_peek_tail_type(&s->tx_queue, pktbuf_t, list);
    return tail != NULL && tx_seg_len(s, tail) < tx_seg_cap(s);
}

__NO_INLINE static void dump_tcp_header(const tcp_header_t *header) {
    printf("TCP: src_port %u, dest_port %u, seq %u, ack %u, win %u, flags %c%c%c%c%c%c\n",
           ntohs(header->source_port), ntohs(header->dest_port), ntohl(header->seq_num), ntohl(header->ack_num),
           ntohs(header->win_size),
           (ntohs(header->length_flags) & PKT_FIN) ? 'F' : ' ',
           (ntohs(header->length_flags) & PKT_SYN) ? 'S' : ' ',
           (ntohs(header->length_flags) & PKT_RST) ? 'R' : ' ',
           (ntohs(header->length_flags) & PKT_PSH) ? 'P' : ' ',
           (ntohs(header->length_flags) & PKT_ACK) ? 'A' : ' ',
           (ntohs(header->length_flags) & PKT_URG) ? 'U' : ' ');
}

static const char *tcp_state_to_string(tcp_state_t state) {
    switch (state) {
        default:
        case STATE_CLOSED:
            return "CLOSED";
        case STATE_LISTEN:
            return "LISTEN";
        case STATE_SYN_SENT:
            return "SYN_SENT";
        case STATE_SYN_RCVD:
            return "SYN_RCVD";
        case STATE_ESTABLISHED:
            return "ESTABLISHED";
        case STATE_CLOSE_WAIT:
            return "CLOSE_WAIT";
        case STATE_LAST_ACK:
            return "LAST_ACK";
        case STATE_CLOSING:
            return "CLOSING";
        case STATE_FIN_WAIT_1:
            return "FIN_WAIT_1";
        case STATE_FIN_WAIT_2:
            return "FIN_WAIT_2";
        case STATE_TIME_WAIT:
            return "TIME_WAIT";
    }
}

static void dump_socket(tcp_socket_t *s) {
    printf("socket %p: state %d (%s), local 0x%x:%hu, remote 0x%x:%hu, mss %u, ref %d\n",
           s, s->state, tcp_state_to_string(s->state),
           s->local_ip, s->local_port, s->remote_ip, s->remote_port, s->mss, s->ref);
    if (s->state == STATE_ESTABLISHED || s->state == STATE_CLOSE_WAIT) {
        printf("\trx: wsize %u wlo %u whi %u (%u) contig %u ooo %u\n",
               s->rx_win_size, s->rx_win_low, s->rx_win_high,
               s->rx_win_high - s->rx_win_low, s->rx_contig_bytes, s->rx_ooo_count);
        printf("\ttx: wlo %u whi %u (%u) highest_seq %u (%u) buffered %u pktbufs %u\n",
               s->tx_win_low, s->tx_win_high, s->tx_win_high - s->tx_win_low,
               s->tx_highest_seq, s->tx_highest_seq - s->tx_win_low,
               tx_buffered_bytes(s), s->tx_pktbuf_count);
    }
    printf("\trto: %u ms (srtt %u rttvar %u%s), retransmits %d, dupacks %u\n",
           s->rto, s->srtt, s->rttvar, s->rtt_valid ? "" : ", unmeasured",
           s->retransmit_count, s->dupack_count);
    printf("\tcwnd: %u (+%u) ssthresh %u%s\n",
           s->cwnd, s->cwnd_extra, s->ssthresh, s->in_recovery ? ", recovering" : "");
    printf("\tcounters: retransmit %u fast %u dupack %u\n",
           s->stat_retransmits, s->stat_fast_retransmits, s->stat_dupacks);
    if (s->fin_pending) {
        printf("\tfin: seq %u %s\n", s->fin_seq, s->fin_sent ? "sent" : "pending");
    }
}



static tcp_socket_t *lookup_socket(ipv4_addr_t remote_ip, ipv4_addr_t local_ip, uint16_t remote_port, uint16_t local_port) {
    LTRACEF_LEVEL(2, "remote ip 0x%x local ip 0x%x remote port %u local port %u\n", remote_ip, local_ip, remote_port, local_port);

    mutex_acquire(&tcp_socket_list_lock);

    /* XXX replace with something faster, like a hash table */
    tcp_socket_t *s = NULL;
    list_for_every_entry(&tcp_socket_list, s, tcp_socket_t, node) {
        if (s->state == STATE_CLOSED || s->state == STATE_LISTEN) {
            continue;
        } else {
            /* full check */
            if (s->remote_ip == remote_ip &&
                    s->local_ip == local_ip &&
                    s->remote_port == remote_port &&
                    s->local_port == local_port) {
                goto out;
            }
        }
    }

    /* walk the list again, looking only for listen matches */
    list_for_every_entry(&tcp_socket_list, s, tcp_socket_t, node) {
        if (s->state == STATE_LISTEN) {
            /* sockets in listen state only care about local port */
            if (s->local_port == local_port) {
                goto out;
            }
        }
    }

    /* fall through case returns null */
    s = NULL;

out:
    /* bump the ref before returning it */
    if (s)
        inc_socket_ref(s);

    mutex_release(&tcp_socket_list_lock);

    return s;
}

static void add_socket_to_list(tcp_socket_t *s) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(s->ref > 0); // we should have implicitly bumped the ref when creating the socket

    mutex_acquire(&tcp_socket_list_lock);

    list_add_head(&tcp_socket_list, &s->node);

    mutex_release(&tcp_socket_list_lock);
}

static void remove_socket_from_list(tcp_socket_t *s) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(s->ref > 0);

    mutex_acquire(&tcp_socket_list_lock);

    DEBUG_ASSERT(list_in_list(&s->node));
    list_delete(&s->node);

    mutex_release(&tcp_socket_list_lock);
}

/* Pick a local port for an outgoing connection from the dynamic range,
 * skipping any port a listed socket already uses (a collision with a
 * listener would make loopback connections match their own mirrored
 * 4-tuple). Returns 0 if the whole range is somehow in use.
 */
static uint16_t alloc_ephemeral_port(void) {
    static uint16_t next_ephemeral;

    mutex_acquire(&tcp_socket_list_lock);

    if (next_ephemeral == 0) {
        /* rand() returns the raw 32 bit LCG state, negative half the time */
        next_ephemeral = 49152 + ((unsigned int)rand() % 16384);
    }

    uint16_t port = 0;
    for (int tries = 0; tries < 16384; tries++) {
        uint16_t candidate = next_ephemeral;
        next_ephemeral = (next_ephemeral < 65535) ? next_ephemeral + 1 : 49152;

        bool in_use = false;
        tcp_socket_t *e;
        list_for_every_entry(&tcp_socket_list, e, tcp_socket_t, node) {
            if (e->local_port == candidate) {
                in_use = true;
                break;
            }
        }
        if (!in_use) {
            port = candidate;
            break;
        }
    }

    mutex_release(&tcp_socket_list_lock);
    return port;
}

static void inc_socket_ref(tcp_socket_t *s) {
    DEBUG_ASSERT(s);

    __UNUSED int oldval = atomic_add(&s->ref, 1);
    LTRACEF_LEVEL(2, "caller %p, thread %p, socket %p, ref now %d\n", __GET_CALLER(), get_current_thread(), s, oldval + 1);
    DEBUG_ASSERT(oldval > 0);
}

static bool dec_socket_ref(tcp_socket_t *s) {
    DEBUG_ASSERT(s);

    int oldval = atomic_add(&s->ref, -1);
    LTRACEF_LEVEL(2, "caller %p, thread %p, socket %p, ref now %d\n", __GET_CALLER(), get_current_thread(), s, oldval - 1);

    if (oldval == 1) {
        LTRACEF("destroying socket\n");
        if (s->route) {
            ipv4_dec_route_ref(s->route);
            s->route = NULL;
        }
        event_destroy(&s->tx_event);
        event_destroy(&s->rx_event);
        event_destroy(&s->connect_event);

        pktbuf_t *q;
        while ((q = list_remove_head_type(&s->rx_queue, pktbuf_t, list)) != NULL) {
            pktbuf_free(q, true);
        }
        while ((q = list_remove_head_type(&s->tx_queue, pktbuf_t, list)) != NULL) {
            pktbuf_free(q, true);
        }

        free(s);
    }
    return (oldval == 1);
}

/* Extract the MSS option from a SYN's option list; 0 if absent or
 * malformed. The option bytes are still in network order (only the fixed
 * header is swapped in place).
 */
static uint16_t tcp_parse_mss_option(const tcp_header_t *header, size_t header_len) {
    const uint8_t *opt = (const uint8_t *)(header + 1);
    const uint8_t *end = (const uint8_t *)header + header_len;

    while (opt < end) {
        if (opt[0] == 0x0) { /* end of option list */
            return 0;
        }
        if (opt[0] == 0x1) { /* nop */
            opt++;
            continue;
        }
        if (opt + 1 >= end || opt[1] < 2 || opt + opt[1] > end) {
            return 0; /* malformed */
        }
        if (opt[0] == 0x2) { /* mss */
            if (opt[1] != 4) {
                return 0;
            }
            return (uint16_t)((opt[2] << 8) | opt[3]);
        }
        opt += opt[1];
    }
    return 0;
}

/* Clamp the socket's segment size to what the peer advertised */
static void tcp_apply_peer_mss(tcp_socket_t *s, uint16_t peer_mss) {
    if (peer_mss >= 64) {
        s->mss = MIN(s->mss, (uint32_t)peer_mss);
    }
}

static void tcp_timer_set(tcp_socket_t *s, net_timer_t *timer, net_timer_callback_t cb, lk_time_t delay) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(timer);

    if (net_timer_set(timer, cb, s, delay))
        inc_socket_ref(s);
}

static void tcp_timer_cancel(tcp_socket_t *s, net_timer_t *timer) {

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(timer);

    if (net_timer_cancel(timer))
        dec_socket_ref(s);
}

/* Process one TCP segment. Returns true if ownership of p was taken (the
 * payload was queued on a socket); the caller frees it otherwise.
 */
bool tcp_input(netif_t *netif, pktbuf_t *p, uint32_t src_ip, uint32_t dst_ip) {
    if (unlikely(tcp_debug))
        TRACEF("p %p (len %u), src_ip 0x%x, dst_ip 0x%x\n", p, p->dlen, src_ip, dst_ip);

    bool consumed = false;

    tcp_header_t *header = (tcp_header_t *)p->data;

    /* reject if too small */
    if (p->dlen < sizeof(tcp_header_t))
        return false;

    if (unlikely(tcp_debug) || LOCAL_TRACE) {
        dump_tcp_header(header);
    }

    /* compute the actual header length (+ options) */
    size_t header_len = ((ntohs(header->length_flags) >> 12) & 0xf) * 4;
    if (p->dlen < header_len) {
        TRACEF("REJECT: packet too large for buffer\n");
        return false;
    }

    /* checksum */
    if (FORCE_TCP_CHECKSUM || (p->flags & PKTBUF_FLAG_CKSUM_TCP_GOOD) == 0) {
        ipv4_pseudo_header_t pheader;

        // set up the pseudo header for checksum purposes
        pheader.source_addr = src_ip;
        pheader.dest_addr = dst_ip;
        pheader.zero = 0;
        pheader.protocol = IP_PROTO_TCP;
        pheader.tcp_length = htons(p->dlen);

        uint16_t checksum = cksum_pheader(&pheader, p->data, p->dlen);
        if (checksum != 0) {
            TRACEF("REJECT: failed checksum, header says 0x%x, we got 0x%x\n", header->checksum, checksum);
            return false;
        }
    }

    /* byte swap header in place */
    header->source_port = ntohs(header->source_port);
    header->dest_port = ntohs(header->dest_port);
    header->seq_num = ntohl(header->seq_num);
    header->ack_num = ntohl(header->ack_num);
    header->length_flags = ntohs(header->length_flags);
    header->win_size = ntohs(header->win_size);
    header->urg_pointer = ntohs(header->urg_pointer);

    /* get some data from the packet */
    uint8_t packet_flags = header->length_flags & 0x3f;
    size_t data_len = p->dlen - header_len;

    /* a segment carrying nothing but an acknowledgement */
    const bool bare_ack = (data_len == 0 && (packet_flags & (PKT_SYN|PKT_FIN)) == 0);

    /* see if it matches a socket we have */
    tcp_socket_t *s = lookup_socket(src_ip, dst_ip, header->source_port, header->dest_port);
    if (!s) {
        /* send a RST packet */
        goto send_reset;
    }

    if (unlikely(tcp_debug))
        TRACEF("got socket %p, state %d (%s), ref %d\n", s, s->state, tcp_state_to_string(s->state), s->ref);

    /* remove the header */
    pktbuf_consume(p, header_len);

    mutex_acquire(&s->lock);

    /* check to see if they're resetting us */
    if (packet_flags & PKT_RST) {
        tcp_abort(s);
        goto done;
    }

    switch (s->state) {
        case STATE_CLOSED:
            /* socket closed, send RST */
            goto send_reset;

        /* passive connect states */
        case STATE_LISTEN: {
            /* we're in listen and they want to talk to us */
            if (!(packet_flags & PKT_SYN)) {
                /* not a SYN, send RST */
                goto send_reset;
            }

            /* see if we have a slot to accept */
            if (s->accepted != NULL)
                goto done;

            /* make a new accept socket */
            tcp_socket_t *accept_socket = create_tcp_socket();
            if (!accept_socket)
                goto done;

            /* set it up */
            accept_socket->local_ip = netif->ipv4_addr;
            accept_socket->local_port = s->local_port;
            accept_socket->remote_ip = src_ip;
            accept_socket->remote_port = header->source_port;
            accept_socket->state = STATE_SYN_RCVD;

            /* honor the mss option on their SYN */
            tcp_apply_peer_mss(accept_socket, tcp_parse_mss_option(header, header_len));

            /* look up and cache the route for the accepted socket */
            ipv4_route_t *route = ipv4_search_route(src_ip);
            if (!route) {
                /* never made it onto the list; drop the create ref */
                dec_socket_ref(accept_socket);
                goto done;
            }
            accept_socket->route = route;

            mutex_acquire(&accept_socket->lock);

            add_socket_to_list(accept_socket);

            /* remember their sequence */
            accept_socket->rx_win_low = header->seq_num + 1;
            accept_socket->rx_win_high = accept_socket->rx_win_low + accept_socket->rx_win_size - 1;

            /* save this socket and wake anyone up that is waiting to accept */
            s->accepted = accept_socket;
            sem_post(&s->accept_sem, true);

            /* send a SYN|ACK; the SYN consumes a sequence */
            accept_socket->tx_win_low++;
            accept_socket->tx_highest_seq = accept_socket->tx_win_low;
            tcp_rtt_start(accept_socket, accept_socket->tx_win_low - 1);
            tcp_send_syn(accept_socket, true);

            /* retransmit it until they ack */
            tcp_timer_set(accept_socket, &accept_socket->retransmit_timer,
                          &handle_retransmit_timeout, accept_socket->rto);

            mutex_release(&accept_socket->lock);
            break;
        }
        case STATE_SYN_RCVD:
            if (packet_flags & PKT_SYN) {
                /* they must have not seen our ack of their original syn, retransmit */
                // XXX implement
                goto send_reset;
            }

            /* if they ack our SYN, we can move on to ESTABLISHED */
            if (packet_flags & PKT_ACK) {
                if (header->ack_num != s->tx_win_low) {
                    goto send_reset;
                }

                s->tx_win_high = s->tx_win_low + header->win_size;
                s->tx_highest_seq = s->tx_win_low;
                tcp_cwnd_init(s);

                tcp_rtt_ack(s, header->ack_num);
                tcp_timer_cancel(s, &s->retransmit_timer);
                s->retransmit_count = 0;
                s->state = STATE_ESTABLISHED;

                /* wake any writer waiting out the handshake */
                event_signal(&s->tx_event, true);
            } else {
                goto send_reset;
            }

            break;

        /* active connection state */
        case STATE_SYN_SENT:
            if ((packet_flags & PKT_SYN) == 0) {
                // we got data on the packet without a syn, reset
                goto send_reset;
            }

            if ((packet_flags & PKT_ACK) == 0) {
                // simultaneous SYN/ACK
                // TODO: handle
                goto send_reset;
            }

            LTRACEF("ack num %d tx win_low %d\n", header->ack_num, s->tx_win_low);

            if (header->ack_num != s->tx_win_low + 1) {
                // they didn't ack our syn
                goto send_reset;
            }

            /* honor the mss option on their SYN|ACK */
            tcp_apply_peer_mss(s, tcp_parse_mss_option(header, header_len));

            // remember their sequence
            s->rx_win_low = header->seq_num + 1;
            s->rx_win_high = s->rx_win_low + s->rx_win_size - 1;

            s->tx_win_low++;
            s->tx_win_high = s->tx_win_low + header->win_size;
            s->tx_highest_seq = s->tx_win_low;
            tcp_cwnd_init(s);

            tcp_rtt_ack(s, header->ack_num);
            tcp_timer_cancel(s, &s->retransmit_timer);
            s->retransmit_count = 0;
            s->state = STATE_ESTABLISHED;

            send_ack(s);

            event_signal(&s->connect_event, true);

            break;

        /* established state */

        case STATE_ESTABLISHED:
            if (packet_flags & PKT_ACK) {
                /* they're acking us */
                handle_ack(s, header->ack_num, header->win_size, bare_ack);
            }

            if (data_len > 0) {
                LTRACEF("new data, len %zu\n", data_len);
                consumed = handle_data(s, p, header->seq_num);
            }

            if ((packet_flags & PKT_FIN) && s->rx_win_low == (uint32_t)(header->seq_num + data_len)) {
                /* they're closing with us, and there's no outstanding data */

                /* FIN consumed a sequence */
                s->rx_win_low++;

                /* ack them and transition to new state */
                send_ack(s);
                s->state = STATE_CLOSE_WAIT;

                /* wake up any read waiters */
                event_signal(&s->rx_event, true);
            }
            break;

        case STATE_CLOSE_WAIT:
            if (packet_flags & PKT_ACK) {
                /* they're acking us */
                handle_ack(s, header->ack_num, header->win_size, bare_ack);
            }
            if (packet_flags & PKT_FIN) {
                /* they must have missed our ack, ack them again */
                send_ack(s);
            }
            break;
        case STATE_LAST_ACK:
            if (packet_flags & PKT_FIN) {
                /* they missed our ack of their FIN, ack it again */
                send_ack(s);
            }
            if (packet_flags & PKT_ACK) {
                handle_ack(s, header->ack_num, header->win_size, bare_ack);
            }
            if (tcp_fin_acked(s)) {
                /* tcp_close() was already called on us, so the abort also
                 * takes the socket off the list and drops its reference */
                tcp_abort(s);
            }
            break;
        case STATE_FIN_WAIT_1:
            if (packet_flags & PKT_ACK) {
                handle_ack(s, header->ack_num, header->win_size, bare_ack);
            }
            if (tcp_fin_acked(s)) {
                s->state = STATE_FIN_WAIT_2;
                /* drop into fin_wait_2 state logic, in case they were FINning us too */
                goto fin_wait_2;
            }
            if (packet_flags & PKT_FIN) {
                /* simultaneous close. they finned us without acking our fin */
                s->rx_win_low++;
                send_ack(s);
                s->state = STATE_CLOSING;
            }
            break;
        case STATE_FIN_WAIT_2:
fin_wait_2:
            if (packet_flags & PKT_FIN) {
                /* they're FINning us, ack them */
                s->rx_win_low++;
                send_ack(s);
                s->state = STATE_TIME_WAIT;

                /* set timed wait timer */
                tcp_timer_set(s, &s->time_wait_timer, &handle_time_wait_timeout, TIME_WAIT_TIMEOUT);
            }
            break;
        case STATE_CLOSING:
            if (packet_flags & PKT_ACK) {
                handle_ack(s, header->ack_num, header->win_size, bare_ack);
            }
            if (tcp_fin_acked(s)) {
                s->state = STATE_TIME_WAIT;

                /* set timed wait timer */
                tcp_timer_set(s, &s->time_wait_timer, &handle_time_wait_timeout, TIME_WAIT_TIMEOUT);
            }
            break;
        case STATE_TIME_WAIT:
            /* a retransmitted FIN means our last ack never arrived: send it
             * again and start the wait over */
            if (packet_flags & PKT_FIN) {
                send_ack(s);
                tcp_timer_set(s, &s->time_wait_timer, &handle_time_wait_timeout, TIME_WAIT_TIMEOUT);
            }
            break;
    }

done:
    mutex_release(&s->lock);
    dec_socket_ref(s);
    return consumed;

send_reset:
    if (s) {
        mutex_release(&s->lock);
        dec_socket_ref(s);
    }

    LTRACEF("SEND RST\n");
    if (!(packet_flags & PKT_RST)) {
        tcp_send(src_ip, header->source_port, dst_ip, header->dest_port,
                 NULL, 0, PKT_RST, NULL, 0, 0, header->ack_num, 0);
    }
    return false;
}

/* Queue an in-window segment on the socket's receive queue, which is kept
 * sorted by p->seq with the contiguous in-order run at the head. p->data and
 * p->dlen describe the payload. Returns true if the pktbuf was queued
 * (ownership transferred); false if the caller should free it.
 */
static bool handle_data(tcp_socket_t *s, pktbuf_t *p, uint32_t sequence) {
    if (unlikely(tcp_debug))
        TRACEF("s %p, p %p, len %u, sequence %u\n", s, p, p->dlen, sequence);

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));
    DEBUG_ASSERT(p);
    DEBUG_ASSERT(p->dlen > 0);

    /* trim the head of the segment to the bottom of our window */
    if (SEQUENCE_LT(sequence, s->rx_win_low)) {
        uint32_t dup = s->rx_win_low - sequence;
        if (dup >= p->dlen) {
            /* complete duplicate; re-ack what we already have */
            send_ack(s);
            return false;
        }
        pktbuf_consume(p, dup);
        sequence = s->rx_win_low;
    }

    /* trim the tail to the right (inclusive) edge of the advertised window */
    uint32_t seq_end = sequence + p->dlen; /* one past the last byte */
    if (SEQUENCE_GT(seq_end, s->rx_win_high + 1)) {
        uint32_t excess = seq_end - (s->rx_win_high + 1);
        if (excess >= p->dlen) {
            /* entirely beyond the window */
            send_ack(s);
            return false;
        }
        pktbuf_consume_tail(p, excess);
        seq_end = sequence + p->dlen;
    }

    /* find the insertion point: succ is the first queued pktbuf starting
     * after us (NULL for the tail), prev the entry before that slot */
    pktbuf_t *succ = NULL;
    pktbuf_t *e;
    list_for_every_entry(&s->rx_queue, e, pktbuf_t, list) {
        if (SEQUENCE_GT(e->seq, sequence)) {
            succ = e;
            break;
        }
    }
    pktbuf_t *prev = succ ? list_prev_type(&s->rx_queue, &succ->list, pktbuf_t, list)
                          : list_peek_tail_type(&s->rx_queue, pktbuf_t, list);

    /* trim our head against overlap with prev's queued bytes */
    if (prev) {
        uint32_t prev_end = prev->seq + prev->dlen;
        if (SEQUENCE_GT(prev_end, sequence)) {
            uint32_t dup = prev_end - sequence;
            if (dup >= p->dlen) {
                /* prev already covers all of it */
                send_ack(s);
                return false;
            }
            pktbuf_consume(p, dup);
            sequence = prev_end;
        }
    }

    bool in_order = (sequence == s->rx_win_low);

    /* small in-order-adjacent segments are copied into the tail of prev
     * rather than costing a pool buffer each */
    bool coalesce = (prev != NULL && prev->seq + prev->dlen == sequence &&
                     p->dlen < TCP_RX_COALESCE_SIZE && pktbuf_avail_tail(prev) >= p->dlen);

    /* out-of-order islands are capped; drop before touching queue state */
    if (!coalesce && !in_order && s->rx_ooo_count >= (uint32_t)TCP_RX_OOO_CAP) {
        send_ack(s);
        return false;
    }

    /* our copy of any overlapped bytes is the one we keep: drop or trim
     * queued successors we cover. everything past the insertion point is
     * beyond the in-order run, so these are all out-of-order islands. */
    while (succ != NULL && SEQUENCE_LT(succ->seq, seq_end)) {
        pktbuf_t *next = list_next_type(&s->rx_queue, &succ->list, pktbuf_t, list);
        uint32_t succ_end = succ->seq + succ->dlen;
        if (SEQUENCE_LTE(succ_end, seq_end)) {
            list_delete(&succ->list);
            pktbuf_free(succ, true);
            DEBUG_ASSERT(s->rx_ooo_count > 0);
            s->rx_ooo_count--;
        } else {
            pktbuf_consume(succ, seq_end - succ->seq);
            succ->seq = seq_end;
            break;
        }
        succ = next;
    }

    bool queued;
    if (coalesce) {
        pktbuf_append_data(prev, p->data, p->dlen);
        queued = false;
    } else {
        p->seq = sequence;
        if (succ) {
            list_add_before(&succ->list, &p->list);
        } else {
            list_add_tail(&s->rx_queue, &p->list);
        }
        queued = true;
    }

    if (in_order) {
        /* the in-order run grew; absorb any islands it now reaches */
        uint32_t old_win_low = s->rx_win_low;
        s->rx_win_low = seq_end;

        pktbuf_t *n = succ; /* first entry past the new data, however stored */
        while (n != NULL && n->seq == s->rx_win_low) {
            s->rx_win_low += n->dlen;
            DEBUG_ASSERT(s->rx_ooo_count > 0);
            s->rx_ooo_count--;
            n = list_next_type(&s->rx_queue, &n->list, pktbuf_t, list);
        }

        uint32_t added = s->rx_win_low - old_win_low;
        s->rx_contig_bytes += added;
        event_signal(&s->rx_event, true);

        /* keep a counter if they've been sending a full mss */
        if (added >= s->mss) {
            s->rx_full_mss_count++;
        } else {
            s->rx_full_mss_count = 0;
        }

        /* immediately ack if we're more than halfway into our buffer or they've sent 2 or more full packets */
        if (s->rx_full_mss_count >= 2 ||
                (int)(s->rx_win_low + s->rx_win_size - s->rx_win_high) > (int)s->rx_win_size / 2) {
            send_ack(s);
            s->rx_full_mss_count = 0;
        } else {
            tcp_timer_set(s, &s->ack_delay_timer, &handle_delayed_ack_timeout, DELAYED_ACK_TIMEOUT);
        }
    } else {
        if (queued) {
            s->rx_ooo_count++;
        }
        /* duplicate-ack the gap so the peer knows what we are missing */
        send_ack(s);
    }

    return queued;
}

static status_t tcp_socket_send(tcp_socket_t *s, const iovec_t *iov, size_t iov_cnt,
                                tcp_flags_t flags, const void *options, size_t options_length, uint32_t sequence) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));
    DEBUG_ASSERT(iov_cnt == 0 || iov);
    DEBUG_ASSERT(options_length == 0 || options);
    DEBUG_ASSERT((options_length % 4) == 0);

    uint16_t win_size = tcp_advertised_window(s);

    // we are piggybacking a pending ACK, so clear the delayed ACK timer
    if (flags & PKT_ACK) {
        tcp_timer_cancel(s, &s->ack_delay_timer);
    }

    status_t err = tcp_send(s->remote_ip, s->remote_port, s->local_ip, s->local_port, iov, iov_cnt, flags,
                            options, options_length, (flags & PKT_ACK) ? s->rx_win_low : 0, sequence, win_size);

    return err;
}

/* (re)send our SYN (active open) or SYN|ACK (passive open) with the mss
 * option attached. For the passive case tx_win_low has already consumed the
 * SYN's sequence, so back up by one.
 */
static void tcp_send_syn(tcp_socket_t *s, bool with_ack) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    tcp_mss_option_t mss_option;
    mss_option.kind = 0x2;
    mss_option.len = 0x4;
    mss_option.mss = htons(s->mss);

    if (with_ack) {
        tcp_socket_send(s, NULL, 0, PKT_ACK|PKT_SYN, &mss_option, sizeof(mss_option),
                        s->tx_win_low - 1);
    } else {
        tcp_send(s->remote_ip, s->remote_port, s->local_ip, s->local_port, NULL, 0, PKT_SYN,
                 &mss_option, sizeof(mss_option), 0, s->tx_win_low, s->rx_win_size);
    }
}

static void send_ack(tcp_socket_t *s) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    /* a bare ack is meaningful in every state with a synchronized sequence
     * space; before that there is nothing to acknowledge with */
    switch (s->state) {
        case STATE_CLOSED:
        case STATE_LISTEN:
        case STATE_SYN_SENT:
        case STATE_SYN_RCVD:
            return;
        default:
            break;
    }

    /* acks carry SND.NXT, which sits past a FIN we have already sent */
    tcp_socket_send(s, NULL, 0, PKT_ACK, NULL, 0, s->tx_highest_seq);
}

static status_t tcp_send(ipv4_addr_t dest_ip, uint16_t dest_port, ipv4_addr_t src_ip, uint16_t src_port,
                         const iovec_t *iov, size_t iov_cnt,
                         tcp_flags_t flags, const void *options, size_t options_length, uint32_t ack, uint32_t sequence, uint16_t window_size) {
    DEBUG_ASSERT(iov_cnt == 0 || iov);
    DEBUG_ASSERT(options_length == 0 || options);
    DEBUG_ASSERT((options_length % 4) == 0);

    pktbuf_t *p = pktbuf_alloc();
    if (!p)
        return ERR_NO_MEMORY;

    tcp_header_t *header = (tcp_header_t *)pktbuf_prepend(p, sizeof(tcp_header_t) + options_length);
    DEBUG_ASSERT(header);

    /* fill in the header */
    header->source_port = htons(src_port);
    header->dest_port = htons(dest_port);
    header->seq_num = htonl(sequence);
    header->ack_num = htonl(ack);
    header->length_flags = htons(((sizeof(tcp_header_t) + options_length) / 4) << 12 | flags);
    header->win_size = htons(window_size);
    header->checksum = 0;
    header->urg_pointer = 0;
    if (options)
        memcpy(header + 1, options, options_length);

    /* append the data */
    if (iov) {
        for (size_t i = 0; i < iov_cnt; i++) {
            if (iov[i].iov_len > 0) {
                DEBUG_ASSERT(iov[i].iov_base);
                pktbuf_append_data(p, iov[i].iov_base, iov[i].iov_len);
            }
        }
    }

    /* compute the checksum */
    /* XXX get the tx ckecksum capability from the nic */
    if (FORCE_TCP_CHECKSUM || true) {
        ipv4_pseudo_header_t pheader;
        pheader.source_addr = src_ip;
        pheader.dest_addr = dest_ip;
        pheader.zero = 0;
        pheader.protocol = IP_PROTO_TCP;
        pheader.tcp_length = htons(p->dlen);

        header->checksum = cksum_pheader(&pheader, p->data, p->dlen);
    }

    if (LOCAL_TRACE) {
        printf("sending ");
        dump_tcp_header(header);
    }

    status_t err = minip_ipv4_send(p, dest_ip, IP_PROTO_TCP);

    return err;
}

/* Calculate the window size to advertise from the unread in-order byte
 * count, never moving the right edge of the window backwards.
 */
static uint16_t tcp_advertised_window(tcp_socket_t *s) {
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    uint32_t rx_win_high = s->rx_win_low + s->rx_win_size - s->rx_contig_bytes - 1;

    LTRACEF("rx_win_low %u rx_win_size %u unread %u, new win high %u\n",
            s->rx_win_low, s->rx_win_size, s->rx_contig_bytes, rx_win_high);

    if (SEQUENCE_GTE(rx_win_high, s->rx_win_high)) {
        s->rx_win_high = rx_win_high;
        return rx_win_high - s->rx_win_low;
    } else {
        // the window size has shrunk, but we can't move the
        // right edge of the window backwards
        return s->rx_win_high - s->rx_win_low;
    }
}

/* Wrap the payload described by p->data/p->dlen in a data (ACK|PSH) header
 * and hand it to the ip layer, which consumes one reference on p whatever
 * the outcome.
 */
static status_t tcp_send_data_pktbuf(tcp_socket_t *s, pktbuf_t *p, uint32_t sequence) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    uint16_t win_size = tcp_advertised_window(s);

    /* data segments carry an ack: clear any pending delayed ack */
    tcp_timer_cancel(s, &s->ack_delay_timer);

    tcp_header_t *header = (tcp_header_t *)pktbuf_prepend(p, sizeof(tcp_header_t));
    DEBUG_ASSERT(header);

    header->source_port = htons(s->local_port);
    header->dest_port = htons(s->remote_port);
    header->seq_num = htonl(sequence);
    header->ack_num = htonl(s->rx_win_low);
    header->length_flags = htons((sizeof(tcp_header_t) / 4) << 12 | (PKT_ACK | PKT_PSH));
    header->win_size = htons(win_size);
    header->checksum = 0;
    header->urg_pointer = 0;

    ipv4_pseudo_header_t pheader;
    pheader.source_addr = s->local_ip;
    pheader.dest_addr = s->remote_ip;
    pheader.zero = 0;
    pheader.protocol = IP_PROTO_TCP;
    pheader.tcp_length = htons(p->dlen);
    header->checksum = cksum_pheader(&pheader, p->data, p->dlen);

    if (LOCAL_TRACE) {
        printf("sending ");
        dump_tcp_header(header);
    }

    return minip_ipv4_send(p, s->remote_ip, IP_PROTO_TCP);
}

/* Transmit the first plen bytes of a queued segment zero copy: point
 * data/dlen at the payload (their values between transmits are
 * meaningless), keep the queue's reference and give one to the tx path.
 * Only valid while no other reference is outstanding.
 */
static status_t tcp_send_queued_segment(tcp_socket_t *s, pktbuf_t *q, uint32_t plen) {
    DEBUG_ASSERT(q->ref == 1);
    DEBUG_ASSERT(plen > 0 && plen <= tx_seg_len(s, q));

    q->data = q->buffer + PKTBUF_MAX_HDR;
    q->dlen = plen;
    pktbuf_ref(q);
    return tcp_send_data_pktbuf(s, q, q->seq);
}

/* Copy a byte range of a queued segment into a fresh pktbuf and send that
 * instead: for ranges not starting at the segment head, for a window that
 * doesn't cover the whole segment, and for segments whose buffer is still
 * referenced by an earlier transmit still in flight.
 */
static status_t tcp_send_segment_copy(tcp_socket_t *s, pktbuf_t *q, uint32_t start, uint32_t len) {
    DEBUG_ASSERT(SEQUENCE_GTE(start, q->seq));
    DEBUG_ASSERT(len > 0);
    DEBUG_ASSERT(SEQUENCE_LTE(start + len, q->seq + tx_seg_len(s, q)));

    pktbuf_t *cp = pktbuf_alloc();
    if (!cp)
        return ERR_NO_MEMORY;

    pktbuf_append_data(cp, q->buffer + PKTBUF_MAX_HDR + (start - q->seq), len);
    return tcp_send_data_pktbuf(s, cp, start);
}

/* Bytes we have sent that the peer has not acknowledged yet. */
static uint32_t tcp_flight_size(tcp_socket_t *s) {
    return s->tx_highest_seq - s->tx_win_low;
}

/* The initial window, sized off the segment size (RFC 3390). It is only
 * known once the handshake has settled the mss.
 */
static void tcp_cwnd_init(tcp_socket_t *s) {
    if (s->mss > 2190) {
        s->cwnd = 2 * s->mss;
    } else if (s->mss > 1095) {
        s->cwnd = 3 * s->mss;
    } else {
        s->cwnd = 4 * s->mss;
    }
    s->cwnd_extra = 0;
    /* start with no threshold, so the connection opens up in slow start
     * until something actually goes wrong */
    s->ssthresh = TCP_CWND_MAX;
    s->in_recovery = false;
}

/* Open the window for data the peer acknowledged (RFC 5681 3.1). */
static void tcp_cwnd_grow(tcp_socket_t *s, uint32_t acked) {
    if (s->cwnd < s->ssthresh) {
        /* slow start: a segment's worth for every segment that arrived */
        s->cwnd += MIN(acked, s->mss);
    } else {
        /* congestion avoidance: about one segment per round trip */
        s->cwnd += MAX(s->mss * s->mss / s->cwnd, 1u);
    }

    s->cwnd = MIN(s->cwnd, (uint32_t)TCP_CWND_MAX);
}

/* Halve the window and resend, on the evidence of duplicate acks alone.
 * 'recover' remembers everything already in flight: acks below it belong
 * to this same loss episode, so a second hole in the same window does not
 * halve the window a second time (RFC 6582).
 */
static void tcp_enter_recovery(tcp_socket_t *s) {
    if (s->in_recovery || !SEQUENCE_GT(s->tx_win_low, s->recover))
        return;

    s->ssthresh = MAX(tcp_flight_size(s) / 2, 2 * s->mss);
    s->cwnd = s->ssthresh + TCP_DUPACK_THRESHOLD * s->mss;
    s->cwnd_extra = 0;
    /* the highest sequence transmitted, so an ack of tx_highest_seq --
     * which acknowledges the byte before it -- reads as a full one */
    s->recover = s->tx_highest_seq - 1;
    s->in_recovery = true;

    LTRACEF("s %p, entering recovery: ssthresh %u cwnd %u recover %u\n",
            s, s->ssthresh, s->cwnd, s->recover);

    tcp_fast_retransmit(s);
}

/* Everything in flight when the loss was detected is acked: deflate the
 * window back to what congestion avoidance should be running with.
 */
static void tcp_exit_recovery(tcp_socket_t *s) {
    s->cwnd = MIN(s->ssthresh, MAX(tcp_flight_size(s), s->mss) + s->mss);
    s->in_recovery = false;

    LTRACEF("s %p, leaving recovery: cwnd %u\n", s, s->cwnd);
}

/* A timeout is the strongest evidence of congestion there is: drop to one
 * segment and start over (RFC 5681 3.1).
 */
static void tcp_cwnd_timeout(tcp_socket_t *s) {
    s->ssthresh = MAX(tcp_flight_size(s) / 2, 2 * s->mss);
    s->cwnd = s->mss;
    s->cwnd_extra = 0;
    s->dupack_count = 0;
    s->in_recovery = false;
}

/* Process an incoming acknowledgement. 'bare' says the segment carried
 * nothing else -- no payload, no SYN or FIN -- which is what makes an ack
 * of nothing new a duplicate rather than just a window update.
 */
static void handle_ack(tcp_socket_t *s, uint32_t sequence, uint32_t win_size, bool bare) {
    LTRACEF("socket %p ack sequence %u, win_size %u\n", s, sequence, win_size);

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    LTRACEF("s %p, tx_win_low %u tx_win_high %u tx_highest_seq %u buffered %u\n",
            s, s->tx_win_low, s->tx_win_high, s->tx_highest_seq, tx_buffered_bytes(s));
    if (SEQUENCE_LTE(sequence, s->tx_win_low)) {
        /* Nothing new is acknowledged, but the window they advertise
         * still counts. A receiver whose reader has caught up has no
         * other way to tell us it can take more, and without this the
         * send window keeps whatever size our last acked segment happened
         * to name -- for the rest of the connection, if the peer never
         * acks anything again. Only ever move the right edge forward, so
         * an old segment arriving late cannot pull it back.
         */
        bool window_opened = false;
        if (sequence == s->tx_win_low && SEQUENCE_GT(sequence + win_size, s->tx_win_high)) {
            s->tx_win_high = sequence + win_size;
            window_opened = true;
        }

        /* They're acking stuff we've already received an ack for. An
         * empty segment repeating the ack we already have, while data of
         * ours is still outstanding, is the peer saying it has a hole.
         *
         * RFC 5681 also asks that the advertised window be unchanged, to
         * tell a duplicate ack apart from a window update. That test is
         * not usable here: the acks a receiver sends from its out of order
         * path carry a window that moves as its own reader drains, so
         * requiring it to hold still loses the duplicates precisely when
         * the far end is busiest. The ack number alone is a good enough
         * signal -- it stands still only while the receiver is missing
         * something, and data of ours being outstanding says that
         * something is ours.
         */
        if (bare && sequence == s->tx_win_low && s->tx_highest_seq != s->tx_win_low) {
            s->stat_dupacks++;
            s->dupack_count++;

            if (s->in_recovery) {
                /* mid recovery each duplicate ack says one more segment
                 * has left the network, so one more may enter it */
                s->cwnd += s->mss;
                tcp_write_pending_data(s);
            } else if (s->dupack_count < TCP_DUPACK_THRESHOLD) {
                /* limited transmit (RFC 3042): each of the first two lets
                 * one new segment out. It keeps the ack clock running, and
                 * with a small window it is what produces the third
                 * duplicate ack at all -- three segments in flight cannot
                 * otherwise generate one */
                s->cwnd_extra = s->dupack_count * s->mss;
                tcp_write_pending_data(s);
            } else if (s->dupack_count == TCP_DUPACK_THRESHOLD) {
                tcp_enter_recovery(s);
            }
        }

        /* room we did not have a moment ago */
        if (window_opened) {
            tcp_write_pending_data(s);
        }
        return;
    } else if (SEQUENCE_GT(sequence, s->tx_highest_seq)) {
        /* they're acking stuff we haven't sent */
        return;
    }

    /* their ack is somewhere in our window */
    LTRACEF("acked len %u\n", sequence - s->tx_win_low);

    /* forward progress: take a round trip sample if one was being timed,
     * and drop whatever backoff the timeout had accumulated */
    tcp_rtt_ack(s, sequence);
    s->retransmit_count = 0;
    s->dupack_count = 0;
    s->cwnd_extra = 0;
    tcp_update_rto(s);

    const uint32_t acked = sequence - s->tx_win_low;

    s->tx_win_low = sequence;
    s->tx_win_high = sequence + win_size;

    /* free fully acked segments; a partially acked head stays whole (its
     * seq is immutable) and a retransmit just resends the acked prefix
     * too, which the peer discards */
    pktbuf_t *q;
    while ((q = list_peek_head_type(&s->tx_queue, pktbuf_t, list)) != NULL) {
        if (SEQUENCE_GT(q->seq + tx_seg_len(s, q), sequence))
            break;
        list_delete(&q->list);
        DEBUG_ASSERT(s->tx_pktbuf_count > 0);
        s->tx_pktbuf_count--;
        pktbuf_free(q, true);
    }

    if (s->in_recovery) {
        if (SEQUENCE_GT(sequence, s->recover)) {
            /* the whole window that was outstanding when we lost a segment
             * has been acked; the episode is over */
            tcp_exit_recovery(s);
        } else {
            /* a partial ack uncovers the next hole: resend it immediately
             * rather than waiting for three more duplicate acks, and
             * account for the segment that just left the network */
            s->cwnd = (s->cwnd > acked) ? s->cwnd - acked : 0;
            if (acked >= s->mss) {
                s->cwnd += s->mss;
            }
            /* never deflate below a segment, or nothing can be sent at all
             * until a timeout puts the window back */
            s->cwnd = MAX(s->cwnd, s->mss);
            if (tcp_retransmit(s) > 0) {
                s->stat_fast_retransmits++;
            }
        }
    } else {
        tcp_cwnd_grow(s, acked);
    }

    /* cancel or reset our retransmit timer */
    if (s->tx_win_low == s->tx_highest_seq) {
        tcp_timer_cancel(s, &s->retransmit_timer);
    } else {
        tcp_timer_set(s, &s->retransmit_timer, &handle_retransmit_timeout, s->rto);
    }

    /* we have opened the transmit buffer */
    event_signal(&s->tx_event, true);

    /* send any pending data that can now fit in the window */
    tcp_write_pending_data(s);
}

static ssize_t tcp_write_pending_data(tcp_socket_t *s) {
    LTRACEF("s %p, tx_win_low %u tx_win_high %u tx_highest_seq %u buffered %u\n",
            s, s->tx_win_low, s->tx_win_high, s->tx_highest_seq, tx_buffered_bytes(s));

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    uint32_t next = s->tx_highest_seq;

    /* find the segment holding the first unsent byte */
    pktbuf_t *q = NULL;
    pktbuf_t *e;
    list_for_every_entry(&s->tx_queue, e, pktbuf_t, list) {
        if (SEQUENCE_LT(next, e->seq + tx_seg_len(s, e))) {
            q = e;
            break;
        }
    }

    /* the peer's window says what it can hold; the congestion window says
     * what the path between us can. Send no further than either. */
    const uint32_t cwnd_edge = s->tx_win_low + s->cwnd + s->cwnd_extra;
    const uint32_t send_edge = SEQUENCE_LT(cwnd_edge, s->tx_win_high) ? cwnd_edge : s->tx_win_high;

    while (q != NULL && SEQUENCE_LT(next, s->tx_buf_top)) {
        int32_t allowed = (int32_t)(send_edge - next);
        if (allowed <= 0)
            break;

        uint32_t plen = tx_seg_len(s, q);
        DEBUG_ASSERT(plen > 0);

        if (next == q->seq && plen <= (uint32_t)allowed && q->ref == 1) {
            /* the whole segment fits the window: send the queued pktbuf */
            if (tcp_send_queued_segment(s, q, plen) < 0)
                break;
            next += plen;
        } else {
            /* partial: window edge, resuming mid-segment, or the buffer is
             * still referenced by a transmit in flight */
            uint32_t seg = MIN(q->seq + plen - next, (uint32_t)allowed);
            if (tcp_send_segment_copy(s, q, next, seg) < 0)
                break;
            next += seg;
        }

        if (SEQUENCE_GTE(next, q->seq + plen))
            q = list_next_type(&s->tx_queue, &q->list, pktbuf_t, list);
    }

    /* a queued FIN follows the last data byte and consumes a sequence of
     * its own; it goes out once everything ahead of it has been sent */
    if (s->fin_pending && !s->fin_sent && next == tx_data_top(s)) {
        if (tcp_socket_send(s, NULL, 0, PKT_ACK|PKT_FIN, NULL, 0, next) >= 0) {
            s->fin_seq = next;
            s->fin_sent = true;
            next++;
        }
    }

    ssize_t sent = next - s->tx_highest_seq;

    /* everything below tx_highest_seq is being sent for the first time, so
     * it is a legitimate thing to measure the round trip with */
    if (sent > 0) {
        tcp_rtt_start(s, s->tx_highest_seq);
    }
    s->tx_highest_seq = next;

    /* keep the retransmit timer running while anything is still owed */
    if (sent > 0 || (s->fin_pending && !s->fin_sent)) {
        tcp_timer_set(s, &s->retransmit_timer, &handle_retransmit_timeout, s->rto);
    }

    return sent;
}

static ssize_t tcp_retransmit(tcp_socket_t *s) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    switch (s->state) {
        case STATE_ESTABLISHED:
        case STATE_CLOSE_WAIT:
        case STATE_FIN_WAIT_1:
        case STATE_CLOSING:
        case STATE_LAST_ACK:
            break;
        default:
            return 0;
    }

    /* how much of our sequence space have we sent but not gotten an ack for? */
    if (s->tx_highest_seq == s->tx_win_low)
        return 0;

    /* resend the oldest unacked data segment, if there is still one */
    pktbuf_t *q = list_peek_head_type(&s->tx_queue, pktbuf_t, list);
    if (q && SEQUENCE_LT(q->seq, s->tx_highest_seq)) {
        /* only as far as the segment has actually been transmitted */
        uint32_t seg = MIN(tx_seg_len(s, q), s->tx_highest_seq - q->seq);

        LTRACEF("s %p, seg %u seq %u\n", s, seg, q->seq);

        if (q->ref == 1) {
            tcp_send_queued_segment(s, q, seg);
        } else {
            tcp_send_segment_copy(s, q, q->seq, seg);
        }

        return seg;
    }

    /* all the data is acked, so what is outstanding is our FIN */
    if (s->fin_sent && !tcp_fin_acked(s)) {
        LTRACEF("s %p, retransmitting fin at seq %u\n", s, s->fin_seq);
        tcp_socket_send(s, NULL, 0, PKT_ACK|PKT_FIN, NULL, 0, s->fin_seq);
        return 1;
    }

    return 0;
}

/* Resend the oldest unacked segment now, without waiting for the timer:
 * duplicate acks say the peer is missing it while still receiving what
 * came after, which is loss rather than reordering. The timeout is not
 * backed off -- nothing timed out -- but the round trip in progress
 * becomes ambiguous just as it would on a timeout.
 */
static void tcp_fast_retransmit(tcp_socket_t *s) {
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    LTRACEF("s %p, seq %u\n", s, s->tx_win_low);

    s->rtt_pending = false;

    if (tcp_retransmit(s) > 0) {
        s->stat_fast_retransmits++;
        tcp_timer_set(s, &s->retransmit_timer, &handle_retransmit_timeout, s->rto);
    }
}

static void handle_retransmit_timeout(void *_s) {
    tcp_socket_t *s = (tcp_socket_t *)_s;

    LTRACEF("s %p\n", s);

    DEBUG_ASSERT(s);

    mutex_acquire(&s->lock);

    /* Karn's algorithm: whatever we resend below, the round trip it
     * belongs to is now ambiguous, so abandon the measurement */
    s->rtt_pending = false;

    /* back the timeout off for the next attempt */
    s->rto = MIN(s->rto * 2, (uint32_t)TCP_RTO_MAX);

    switch (s->state) {
        case STATE_SYN_SENT:
        case STATE_SYN_RCVD:
            /* our SYN or SYN|ACK went unacked */
            if (++s->retransmit_count >= SYN_RETRANSMIT_RETRIES) {
                /* give up establishing the connection */
                LTRACEF("s %p, giving up on connection establishment\n", s);
                s->state = STATE_CLOSED;
                tcp_wakeup_waiters(s);
            } else {
                tcp_send_syn(s, (s->state == STATE_SYN_RCVD));
                tcp_timer_set(s, &s->retransmit_timer, &handle_retransmit_timeout, s->rto);
            }
            break;
        default:
            tcp_cwnd_timeout(s);

            if (++s->retransmit_count >= TCP_MAX_RETRANSMITS) {
                /* the peer has stopped answering entirely */
                LTRACEF("s %p, giving up after %d retransmits\n", s, s->retransmit_count);
                tcp_abort(s);
                break;
            }

            if (tcp_retransmit(s) > 0) {
                s->stat_retransmits++;
                tcp_timer_set(s, &s->retransmit_timer, &handle_retransmit_timeout, s->rto);
            } else {
                /* nothing to resend; a FIN that failed to allocate a pktbuf
                 * gets another try here, and rearms the timer itself */
                tcp_write_pending_data(s);
            }
            break;
    }

    mutex_release(&s->lock);
    dec_socket_ref(s);
}

static void handle_delayed_ack_timeout(void *_s) {
    tcp_socket_t *s = (tcp_socket_t *)_s;

    LTRACEF("s %p\n", s);

    DEBUG_ASSERT(s);

    mutex_acquire(&s->lock);
    send_ack(s);
    mutex_release(&s->lock);
    dec_socket_ref(s);
}

static void handle_time_wait_timeout(void *_s) {
    tcp_socket_t *s = (tcp_socket_t *)_s;

    LTRACEF("s %p\n", s);

    DEBUG_ASSERT(s);

    mutex_acquire(&s->lock);

    DEBUG_ASSERT(s->state == STATE_TIME_WAIT);

    /* remove us from the list and drop the last ref */
    remove_socket_from_list(s);
    dec_socket_ref(s);

    mutex_release(&s->lock);
    dec_socket_ref(s);
}

static void tcp_wakeup_waiters(tcp_socket_t *s) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    // wake up any waiters
    event_signal(&s->rx_event, true);
    event_signal(&s->tx_event, true);
    event_signal(&s->connect_event, true);
}

static void tcp_remote_close(tcp_socket_t *s) {
    LTRACEF("s %p, ref %d\n", s, s->ref);

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));
    DEBUG_ASSERT(s->ref > 0);

    if (s->state == STATE_CLOSED)
        return;

    s->state = STATE_CLOSED;

    tcp_timer_cancel(s, &s->retransmit_timer);
    tcp_timer_cancel(s, &s->ack_delay_timer);

    tcp_wakeup_waiters(s);
}

/* Tear a connection down from the stack side, for a reset or a peer that
 * has stopped answering. States past tcp_close() have no owner left to
 * come back and free the socket, so they also give up their list entry and
 * the reference that came with it.
 */
static void tcp_abort(tcp_socket_t *s) {
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    if (s->state == STATE_CLOSED || s->state == STATE_LISTEN)
        return;

    bool unowned;
    switch (s->state) {
        case STATE_FIN_WAIT_1:
        case STATE_FIN_WAIT_2:
        case STATE_CLOSING:
        case STATE_LAST_ACK:
        case STATE_TIME_WAIT:
            unowned = true;
            break;
        default:
            unowned = false;
            break;
    }

    tcp_remote_close(s);

    if (unowned) {
        tcp_timer_cancel(s, &s->time_wait_timer);
        remove_socket_from_list(s);
        dec_socket_ref(s);
    }
}

static tcp_socket_t *create_tcp_socket(void) {
    tcp_socket_t *s;

    s = (tcp_socket_t *)calloc(1, sizeof(tcp_socket_t));
    if (!s)
        return NULL;

    mutex_init(&s->lock);
    s->ref = 1; // start with the ref already bumped

    s->state = STATE_CLOSED;
    s->rx_win_size = DEFAULT_RX_WINDOW_SIZE;
    list_initialize(&s->rx_queue);
    event_init(&s->rx_event, false, 0);

    s->mss = DEFAULT_MSS;
    s->rto = TCP_RTO_INITIAL;

    s->tx_win_low = rand();
    s->tx_win_high = s->tx_win_low;
    s->tx_highest_seq = s->tx_win_low;
    /* NewReno's recovery point starts at the initial send sequence, so the
     * first loss on the connection counts as a new episode */
    s->recover = s->tx_win_low;
    tcp_cwnd_init(s);
    list_initialize(&s->tx_queue);
    event_init(&s->tx_event, true, 0);

    sem_init(&s->accept_sem, 0);
    event_init(&s->connect_event, false, 0);

    return s;
}

/* user api */
status_t tcp_connect(tcp_socket_t **handle, uint32_t addr, uint16_t port) {
    tcp_socket_t *s;

    if (!handle)
        return ERR_INVALID_ARGS;

    s = create_tcp_socket();
    if (!s)
        return ERR_NO_MEMORY;

    // XXX add some entropy to try to better randomize things
    lk_bigtime_t t = current_time_hires();
    rand_add_entropy(&t, sizeof(t));

    // look up route to set local address
    ipv4_route_t *route = ipv4_search_route(addr);
    if (!route) {
        /* drop the create ref; the socket was never added to the list */
        dec_socket_ref(s);
        return ERR_NO_ROUTE;
    }
    netif_t *netif = route->interface;
    s->route = route;

    // set up the socket for outgoing connections
    s->local_ip = netif->ipv4_addr;
    s->local_port = alloc_ephemeral_port();
    if (s->local_port == 0) {
        dec_socket_ref(s);
        return ERR_NO_RESOURCES;
    }
    s->remote_ip = addr;
    s->remote_port = port;

    if (LOCAL_TRACE) {
        dump_socket(s);
    }

    // send a SYN packet
    mutex_acquire(&s->lock);

    s->state = STATE_SYN_SENT;
    add_socket_to_list(s);

    /* the handshake doubles as the first round trip measurement */
    tcp_rtt_start(s, s->tx_win_low);
    tcp_send_syn(s, false);

    /* retransmit the SYN until they answer; gives up and wakes us on exhaustion */
    tcp_timer_set(s, &s->retransmit_timer, &handle_retransmit_timeout, s->rto);

    mutex_release(&s->lock);

    // block until the handshake concludes one way or the other
    event_wait(&s->connect_event);

    status_t err = NO_ERROR;
    mutex_acquire(&s->lock);
    if (s->state != STATE_ESTABLISHED) {
        err = ERR_CHANNEL_CLOSED;
    }
    mutex_release(&s->lock);

    *handle = s;

    return err;
}

status_t tcp_open_listen(tcp_socket_t **handle, uint16_t port) {
    tcp_socket_t *s;

    if (!handle)
        return ERR_INVALID_ARGS;

    s = create_tcp_socket();
    if (!s)
        return ERR_NO_MEMORY;

    s->local_port = port;

    /* go to listen state */
    s->state = STATE_LISTEN;

    /* check for an existing listener on this port and insert atomically */
    mutex_acquire(&tcp_socket_list_lock);
    tcp_socket_t *e;
    list_for_every_entry(&tcp_socket_list, e, tcp_socket_t, node) {
        if (e->state == STATE_LISTEN && e->local_port == port) {
            mutex_release(&tcp_socket_list_lock);
            s->state = STATE_CLOSED;
            dec_socket_ref(s);
            return ERR_ALREADY_EXISTS;
        }
    }
    list_add_head(&tcp_socket_list, &s->node);
    mutex_release(&tcp_socket_list_lock);

    *handle = s;

    return NO_ERROR;
}

status_t tcp_accept_timeout(tcp_socket_t *listen_socket, tcp_socket_t **accept_socket, lk_time_t timeout) {
    if (!listen_socket || !accept_socket)
        return ERR_INVALID_ARGS;

    tcp_socket_t *s = listen_socket;
    inc_socket_ref(s);

    /* block to accept a socket for an amount of time */
    if (sem_timedwait(&s->accept_sem, timeout) == ERR_TIMED_OUT) {
        dec_socket_ref(s);
        return ERR_TIMED_OUT;
    }

    mutex_acquire(&s->lock);

    /* we got here, grab the accepted socket and return */
    DEBUG_ASSERT(s->accepted);
    *accept_socket = s->accepted;
    s->accepted = NULL;

    mutex_release(&s->lock);
    dec_socket_ref(s);

    return NO_ERROR;
}

ssize_t tcp_read(tcp_socket_t *socket, void *buf, size_t len) {
    LTRACEF("socket %p, buf %p, len %zu\n", socket, buf, len);
    if (!socket)
        return ERR_INVALID_ARGS;
    if (len == 0)
        return 0;
    if (!buf)
        return ERR_INVALID_ARGS;

    tcp_socket_t *s = socket;
    inc_socket_ref(s);

    ssize_t ret = 0;
    size_t pos = 0;
    uint32_t new_rx_win_size;
    uint32_t advertised;
retry:
    /* block on available data */
    event_wait(&s->rx_event);

    mutex_acquire(&s->lock);

    /* read out of the in-order run at the head of the queue, even if we're closed */
    if (s->rx_contig_bytes == 0) {
        /* check to see if we've closed */
        if (s->state != STATE_ESTABLISHED) {
            ret = ERR_CHANNEL_CLOSED;
            goto out;
        }

        /* we must have raced with another thread */
        event_unsignal(&s->rx_event);
        mutex_release(&s->lock);
        goto retry;
    }

    while (pos < len && s->rx_contig_bytes > 0) {
        pktbuf_t *q = list_peek_head_type(&s->rx_queue, pktbuf_t, list);
        DEBUG_ASSERT(q);

        /* the head pktbuf sits entirely inside the in-order run */
        size_t tocopy = MIN(len - pos, (size_t)q->dlen);
        DEBUG_ASSERT(tocopy <= s->rx_contig_bytes);

        memcpy((uint8_t *)buf + pos, q->data, tocopy);
        pktbuf_consume(q, tocopy);
        q->seq += tocopy;
        pos += tocopy;
        s->rx_contig_bytes -= tocopy;

        if (q->dlen == 0) {
            list_delete(&q->list);
            pktbuf_free(q, true);
        }
    }
    ret = pos;

    /* if we've used up the last in-order byte, unsignal the read event */
    if (s->state == STATE_ESTABLISHED && s->rx_contig_bytes == 0) {
        event_unsignal(&s->rx_event);
    }

    /* Reading has opened the window; tell them so if it is worth a segment
     * of its own. RFC 1122 4.2.3.3: an update is worth sending once the
     * window has grown by two segments or half the buffer beyond what they
     * last heard, and it is needed outright when the window had closed
     * below a segment, since they cannot send at all until it moves.
     *
     * Waiting only for the second case is not enough. Acks are the only
     * other thing that carries a window, so a sender that stops writing
     * while our reader is still draining keeps whatever window our last
     * ack happened to name -- and resumes into a window several segments
     * too small. Duplicate acks cannot be raised from a burst that small,
     * so a loss there costs a full retransmit timeout.
     */
    advertised = s->rx_win_high - s->rx_win_low;
    new_rx_win_size = s->rx_win_size - s->rx_contig_bytes;

    if ((advertised < s->mss && new_rx_win_size >= s->mss) ||
            new_rx_win_size >= advertised + MIN(2 * s->mss, s->rx_win_size / 2)) {
        send_ack(s);
    }

out:
    mutex_release(&s->lock);
    dec_socket_ref(s);

    return ret;
}

ssize_t tcp_write(tcp_socket_t *socket, const void *buf, size_t len) {
    LTRACEF("socket %p, buf %p, len %zu\n", socket, buf, len);
    if (!socket)
        return ERR_INVALID_ARGS;
    if (len == 0)
        return 0;
    if (!buf)
        return ERR_INVALID_ARGS;

    tcp_socket_t *s = socket;
    inc_socket_ref(s);

    ssize_t ret = (ssize_t)len;
    pktbuf_t *spare = NULL;
    size_t off = 0;
    while (off < len) {
        LTRACEF("off %zu, len %zu\n", off, len);

        /* wait for the tx queue to open up */
        event_wait(&s->tx_event);

        mutex_acquire(&s->lock);

        /* an accepted socket is handed out while the handshake may still
         * be completing; wait for it rather than failing the write */
        if (s->state == STATE_SYN_RCVD) {
            event_unsignal(&s->tx_event);
            mutex_release(&s->lock);
            continue;
        }

        /* check to see if we've closed */
        if (s->state != STATE_ESTABLISHED && s->state != STATE_CLOSE_WAIT) {
            mutex_release(&s->lock);
            ret = ERR_CHANNEL_CLOSED;
            break;
        }

        uint32_t seg_cap = tx_seg_cap(s);
        uint32_t buffered = tx_buffered_bytes(s);
        size_t byte_room = (buffered < DEFAULT_TX_BUFFER_SIZE) ? DEFAULT_TX_BUFFER_SIZE - buffered : 0;
        size_t copied = 0;

        /* top up the tail segment first; its pktbuf may be in flight, but
         * bytes past the segment end are outside any transmitted frame */
        pktbuf_t *tail = list_peek_tail_type(&s->tx_queue, pktbuf_t, list);
        if (tail != NULL && byte_room > 0) {
            uint32_t plen = tx_seg_len(s, tail);
            if (plen < seg_cap) {
                size_t tocopy = MIN(MIN((size_t)(seg_cap - plen), len - off), byte_room);
                memcpy(tail->buffer + PKTBUF_MAX_HDR + plen, (const uint8_t *)buf + off, tocopy);
                s->tx_buf_top += tocopy;
                off += tocopy;
                copied += tocopy;
                byte_room -= tocopy;
            }
        }

        /* start a new segment if there's still data and a pktbuf slot */
        if (off < len && byte_room > 0 && s->tx_pktbuf_count < (uint32_t)TCP_TX_MAX_PKTBUFS) {
            if (spare == NULL) {
                /* get a buffer without holding the socket lock: the
                 * blocking allocator is serviced by frees that may need it
                 * (and the stack thread takes s->lock in tcp_input) */
                mutex_release(&s->lock);
                spare = pktbuf_alloc_timeout(INFINITE_TIME);
                if (spare == NULL) {
                    ret = ERR_NO_MEMORY;
                    break;
                }
                /* revalidate everything */
                continue;
            }

            size_t tocopy = MIN(MIN((size_t)seg_cap, len - off), byte_room);

            pktbuf_reset(spare, PKTBUF_MAX_HDR);
            memcpy(spare->buffer + PKTBUF_MAX_HDR, (const uint8_t *)buf + off, tocopy);

            if (list_is_empty(&s->tx_queue)) {
                DEBUG_ASSERT(s->tx_win_low == s->tx_highest_seq);
                s->tx_buf_top = s->tx_win_low;
            }
            spare->seq = s->tx_buf_top;
            list_add_tail(&s->tx_queue, &spare->list);
            s->tx_pktbuf_count++;
            s->tx_buf_top += tocopy;
            spare = NULL;

            off += tocopy;
            copied += tocopy;
        }

        /* if the queue is full, sleep until acks drain it */
        if (!tx_queue_has_room(s)) {
            event_unsignal(&s->tx_event);
        }

        if (copied > 0) {
            /* send as much data as we can */
            tcp_write_pending_data(s);
        }

        mutex_release(&s->lock);
    }

    if (spare != NULL) {
        pktbuf_free(spare, true);
    }
    dec_socket_ref(s);
    return ret;
}

status_t tcp_close(tcp_socket_t *socket) {
    if (!socket)
        return ERR_INVALID_ARGS;

    tcp_socket_t *s = socket;

    inc_socket_ref(s);
    mutex_acquire(&s->lock);

    LTRACEF("socket %p, state %d (%s), ref %d\n", s, s->state, tcp_state_to_string(s->state), s->ref);

    status_t err;
    switch (s->state) {
        case STATE_CLOSED:
        case STATE_LISTEN:
            /* we can directly remove this socket */
            remove_socket_from_list(s);

            /* drop any timers that may be pending on this */
            tcp_timer_cancel(s, &s->ack_delay_timer);
            tcp_timer_cancel(s, &s->retransmit_timer);

            s->state = STATE_CLOSED;

            /* drop the extra ref that was held when the socket was created */
            dec_socket_ref(s);
            break;
        case STATE_SYN_RCVD:
        case STATE_ESTABLISHED:
            s->state = STATE_FIN_WAIT_1;

            /* the FIN belongs after everything already written, and is
             * retransmitted like data until they ack it */
            s->fin_pending = true;
            tcp_write_pending_data(s);

            /* stick around and wait for them to FIN us */
            break;
        case STATE_CLOSE_WAIT:
            s->state = STATE_LAST_ACK;

            s->fin_pending = true;
            tcp_write_pending_data(s);
            break;
        case STATE_SYN_SENT:
        case STATE_FIN_WAIT_1:
        case STATE_FIN_WAIT_2:
        case STATE_CLOSING:
        case STATE_TIME_WAIT:
        case STATE_LAST_ACK:
            /* these states are all post tcp_close(), so it's invalid to call it here */
            err = ERR_CHANNEL_CLOSED;
            goto out;
        default:
            PANIC_UNIMPLEMENTED;
    }

    /* make sure anyone blocked on this wakes up */
    tcp_wakeup_waiters(s);

    mutex_release(&s->lock);

    err = NO_ERROR;

out:
    /* if this was the last ref, it should destroy the socket */
    dec_socket_ref(s);

    return err;
}

void tcp_get_socket_stats(tcp_socket_t *s, tcp_socket_stats_t *out) {
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(out);

    mutex_acquire(&s->lock);
    out->retransmits = s->stat_retransmits;
    out->fast_retransmits = s->stat_fast_retransmits;
    out->dupacks = s->stat_dupacks;
    out->rto = s->rto;
    out->srtt = s->srtt;
    out->cwnd = s->cwnd;
    out->ssthresh = s->ssthresh;
    out->mss = s->mss;
    mutex_release(&s->lock);
}

/* debug stuff */
int cmd_tcp(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
notenoughargs:
        printf("ERROR not enough arguments\n");
usage:
        printf("usage: %s sockets\n", argv[0].str);
        printf("usage: %s listenclose <port>\n", argv[0].str);
        printf("usage: %s listen <port>\n", argv[0].str);
        printf("usage: %s connect <addr> <port> [message]\n", argv[0].str);
        printf("usage: %s debug\n", argv[0].str);
        return ERR_INVALID_ARGS;
    }

    if (!strcmp(argv[1].str, "sockets")) {

        mutex_acquire(&tcp_socket_list_lock);
        tcp_socket_t *s = NULL;
        list_for_every_entry(&tcp_socket_list, s, tcp_socket_t, node) {
            dump_socket(s);
        }
        mutex_release(&tcp_socket_list_lock);
    } else if (!strcmp(argv[1].str, "listenclose")) {
        /* listen for a connection, accept it, then immediately close it */
        if (argc < 3) goto notenoughargs;

        tcp_socket_t *handle = NULL;

        status_t err = tcp_open_listen(&handle, argv[2].u);
        printf("tcp_open_listen returns %d, handle %p\n", err, handle);

        tcp_socket_t *accepted;
        err = tcp_accept(handle, &accepted);
        printf("tcp_accept returns returns %d, handle %p\n", err, accepted);

        err = tcp_close(accepted);
        printf("tcp_close returns %d\n", err);

        err = tcp_close(handle);
        printf("tcp_close returns %d\n", err);
    } else if (!strcmp(argv[1].str, "listen")) {
        if (argc < 3) goto notenoughargs;

        tcp_socket_t *handle = NULL;

        status_t err = tcp_open_listen(&handle, argv[2].u);
        printf("tcp_open_listen returns %d, handle %p\n", err, handle);

        tcp_socket_t *accepted;
        err = tcp_accept(handle, &accepted);
        printf("tcp_accept returns returns %d, handle %p\n", err, accepted);

        for (;;) {
            uint8_t buf[512];

            ssize_t err_len = tcp_read(accepted, buf, sizeof(buf));
            printf("tcp_read returns %zd\n", err_len);
            if (err_len < 0)
                break;
            if (err_len > 0) {
                hexdump8(buf, err_len);
            }

            err_len = tcp_write(accepted, buf, err_len);
            printf("tcp_write returns %zd\n", err_len);
            if (err_len < 0)
                break;
        }

        err = tcp_close(accepted);
        printf("tcp_close returns %d\n", err);

        err = tcp_close(handle);
        printf("tcp_close returns %d\n", err);
    } else if (!strcmp(argv[1].str, "connect")) {
        if (argc < 4) goto notenoughargs;

        ipv4_addr_t addr;
        status_t resolve_err = minip_resolve(argv[2].str, &addr);
        if (resolve_err < 0) {
            printf("failed to resolve '%s': %d\n", argv[2].str, resolve_err);
            return resolve_err;
        }
        const char *message = (argc >= 5) ? argv[4].str : "hello from lk\n";

        tcp_socket_t *handle = NULL;
        status_t err = tcp_connect(&handle, addr, argv[3].u);
        printf("tcp_connect returns %d, handle %p\n", err, handle);
        if (err < 0) {
            if (handle)
                tcp_close(handle);
            return err;
        }

        ssize_t err_len = tcp_write(handle, message, strlen(message));
        printf("tcp_write returns %zd\n", err_len);

        uint8_t buf[128];
        err_len = tcp_read(handle, buf, sizeof(buf));
        printf("tcp_read returns %zd\n", err_len);
        if (err_len > 0) {
            hexdump8(buf, err_len);
        }

        err = tcp_close(handle);
        printf("tcp_close returns %d\n", err);
    } else if (!strcmp(argv[1].str, "debug")) {
        tcp_debug = !tcp_debug;
        printf("tcp debug now %u\n", tcp_debug);
    } else {
        printf("ERROR unknown command\n");
        goto usage;
    }

    return NO_ERROR;
}

