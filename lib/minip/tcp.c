/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "minip-internal.h"

#include <trace.h>
#include <assert.h>
#include <compiler.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <lib/console.h>
#include <lib/cbuf.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>

#define LOCAL_TRACE 0

typedef uint32_t ipv4_addr;

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

typedef struct tcp_pseudo_header {
    ipv4_addr source_addr;
    ipv4_addr dest_addr;
    uint8_t zero;
    uint8_t protocol;
    uint16_t tcp_length;
} __PACKED tcp_pseudo_header_t;

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

typedef struct tcp_socket {
    struct list_node node;

    mutex_t lock;

    tcp_state_t state;
    ipv4_addr local_ip;
    ipv4_addr remote_ip;
    uint16_t local_port;
    uint16_t remote_port;

    uint32_t mss;

    /* rx */
    uint32_t rx_win_size;
    uint32_t rx_win_low;
    uint32_t rx_win_high;
    uint8_t  *rx_buffer_raw;
    cbuf_t   rx_buffer;
    event_t  rx_event;
    net_timer_t ack_delay_timer;

    /* tx */
    uint32_t tx_win_low;  // low side of the acked window
    uint32_t tx_win_high; // tx_win_low + their advertised window size
    uint32_t tx_highest_seq; // highest sequence we have txed them
    uint8_t  *tx_buffer;  // our outgoing buffer
    uint32_t tx_buffer_size; // size of tx_buffer
    uint32_t tx_buffer_offset; // offset into the buffer to append new data to
    event_t  tx_event;
    net_timer_t retransmit_timer;

    /* listen accept */
    semaphore_t accept_sem;
    struct tcp_socket *accepted;
} tcp_socket_t;

#define DEFAULT_MSS (1024)
#define DEFAULT_RX_WINDOW_SIZE (4096)
#define DEFAULT_TX_BUFFER_SIZE (4096)

#define RETRANSMIT_TIMEOUT (50)

#define SEQUENCE_GTE(a, b) ((int32_t)((a) - (b)) >= 0)
#define SEQUENCE_LTE(a, b) ((int32_t)((a) - (b)) <= 0)
#define SEQUENCE_GT(a, b) ((int32_t)((a) - (b)) > 0)
#define SEQUENCE_LT(a, b) ((int32_t)((a) - (b)) < 0)

static struct list_node tcp_socket_list = LIST_INITIAL_VALUE(tcp_socket_list);

/* local routines */
static tcp_socket_t *lookup_socket(ipv4_addr remote_ip, ipv4_addr local_ip, uint16_t remote_port, uint16_t local_port);
static tcp_socket_t *create_tcp_socket(bool alloc_buffers);
static status_t tcp_send(ipv4_addr dest_ip, uint16_t dest_port, ipv4_addr src_ip, uint16_t src_port, const void *buf,
    size_t len, tcp_flags_t flags, const void *options, size_t options_length, uint32_t ack, uint32_t sequence, uint16_t window_size);
static status_t tcp_socket_send(tcp_socket_t *s, const void *data, size_t len, tcp_flags_t flags, const void *options, size_t options_length, uint32_t sequence);
static void handle_data(tcp_socket_t *s, const void *data, size_t len, uint32_t sequence);
static void send_ack(tcp_socket_t *s);
static void handle_ack(tcp_socket_t *s, uint32_t sequence, uint32_t win_size);
static void handle_retransmit_timeout(void *_s);
static void tcp_remote_close(tcp_socket_t *s);

static uint16_t cksum_pheader(const tcp_pseudo_header_t *pheader, const void *buf, size_t len)
{
    uint16_t checksum = ones_sum16(0, pheader, sizeof(*pheader));
    return ~ones_sum16(checksum, buf, len);
}

__NO_INLINE static void dump_tcp_header(const tcp_header_t *header)
{
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

static const char *tcp_state_to_string(tcp_state_t state)
{
    switch (state) {
        default:
        case STATE_CLOSED: return "CLOSED";
        case STATE_LISTEN: return "LISTEN";
        case STATE_SYN_SENT: return "SYN_SENT";
        case STATE_SYN_RCVD: return "SYN_RCVD";
        case STATE_ESTABLISHED: return "ESTABLISHED";
        case STATE_CLOSE_WAIT: return "CLOSE_WAIT";
        case STATE_LAST_ACK: return "LAST_ACK";
        case STATE_CLOSING: return "CLOSING";
        case STATE_FIN_WAIT_1: return "FIN_WAIT_1";
        case STATE_FIN_WAIT_2: return "FIN_WAIT_2";
        case STATE_TIME_WAIT: return "TIME_WAIT";
    }
}

void tcp_input(pktbuf_t *p, uint32_t src_ip, uint32_t dst_ip)
{
    LTRACEF("p %p (len %zu), src_ip 0x%x, dst_ip 0x%x\n", p, p->dlen, src_ip, dst_ip);

    tcp_header_t *header = (tcp_header_t *)p->data;

    /* reject if too small */
    if (p->dlen < sizeof(tcp_header_t))
        return;

    if (LOCAL_TRACE) {
        dump_tcp_header(header);
    }

    /* compute the actual header length (+ options) */
    size_t header_len = ((ntohs(header->length_flags) >> 12) & 0xf) * 4;
    if (p->dlen < header_len) {
        LTRACEF("REJECT: packet too large for buffer\n");
        return;
    }

    /* checksum */
    {
        tcp_pseudo_header_t pheader;

        // set up the pseudo header for checksum purposes
        pheader.source_addr = src_ip;
        pheader.dest_addr = dst_ip;
        pheader.zero = 0;
        pheader.protocol = IP_PROTO_TCP;
        pheader.tcp_length = htons(p->dlen);

        uint16_t checksum = cksum_pheader(&pheader, p->data, p->dlen);
        if(checksum != 0) {
            LTRACEF("REJECT: failed checksum\n");
            return;
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
    uint32_t highest_sequence = header->seq_num + ((data_len > 0) ? (data_len - 1) : 0);

    /* see if it matches a socket we have */
    tcp_socket_t *s = lookup_socket(src_ip, dst_ip, header->source_port, header->dest_port);
    if (!s) {
        /* send a RST packet */
        goto send_reset;
    }

    LTRACEF("got socket %p, state %d (%s)\n", s, s->state, tcp_state_to_string(s->state));

    /* remove the header */
    pktbuf_consume(p, header_len);

    mutex_acquire(&s->lock);

    /* check to see if they're resetting us */
    if (packet_flags & PKT_RST) {
        if (s->state != STATE_CLOSED && s->state != STATE_LISTEN) {
            tcp_remote_close(s);
        }
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
            tcp_socket_t *accept_socket = create_tcp_socket(true);
            if (!accept_socket)
                goto done;

            /* set it up */
            accept_socket->local_ip = minip_get_ipaddr();
            accept_socket->local_port = s->local_port;
            accept_socket->remote_ip = src_ip;
            accept_socket->remote_port = header->source_port;
            accept_socket->state = STATE_SYN_RCVD;

            mutex_acquire(&accept_socket->lock);

            list_add_head(&tcp_socket_list, &accept_socket->node);

            /* remember their sequence */
            accept_socket->rx_win_low = header->seq_num + 1;
            accept_socket->rx_win_high = accept_socket->rx_win_low + accept_socket->rx_win_size - 1;

            /* save this socket and wake anyone up that is waiting to accept */
            s->accepted = accept_socket;
            sem_post(&s->accept_sem, true);

            /* set up a mss option for sending back */
            tcp_mss_option_t mss_option;
            mss_option.kind = 0x2;
            mss_option.len = 0x4;
            mss_option.mss = ntohs(s->mss); // XXX make sure we fit in their mss

            /* send a response */
            tcp_socket_send(accept_socket, NULL, 0, PKT_ACK|PKT_SYN, &mss_option, sizeof(mss_option),
                accept_socket->tx_win_low);

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
                if (header->ack_num != s->tx_win_low + 1) {
                    goto send_reset;
                }

                s->tx_win_low++;
                s->tx_win_high = s->tx_win_low + header->win_size;
                s->tx_highest_seq = s->tx_win_low;

                s->state = STATE_ESTABLISHED;
            } else {
                goto send_reset;
            }

            break;

        case STATE_ESTABLISHED:
            if (packet_flags & PKT_ACK) {
                /* they're acking us */
                handle_ack(s, header->ack_num, header->win_size);
            }

            if (data_len > 0) {
                LTRACEF("new data, len %u\n", data_len);
                handle_data(s, p->data, p->dlen, header->seq_num);
            }

            if ((packet_flags & PKT_FIN) && SEQUENCE_GTE(s->rx_win_low, highest_sequence)) {
                // they're closing with us, and there's no outstanding data

                // FIN consumed a sequence
                s->rx_win_low++;

                // ack them and transition to new state
                send_ack(s);
                s->state = STATE_CLOSE_WAIT;

                // wake up any waiters
                event_signal(&s->rx_event, true);
            }
            break;

        case STATE_CLOSE_WAIT:
            if (packet_flags & PKT_FIN) {
                /* they must have missed our ack, ack them again */
                send_ack(s);
            }
            break;
        case STATE_SYN_SENT:
        case STATE_LAST_ACK:
        case STATE_CLOSING:
        case STATE_FIN_WAIT_1:
        case STATE_FIN_WAIT_2:
        case STATE_TIME_WAIT:
            PANIC_UNIMPLEMENTED;
    }

done:
    mutex_release(&s->lock);
    return;

send_reset:
    if (s)
        mutex_release(&s->lock);

    LTRACEF("SEND RST\n");
    if (!(packet_flags & PKT_RST)) {
        tcp_send(src_ip, header->source_port, dst_ip, header->dest_port,
            NULL, 0, PKT_RST, NULL, 0, 0, header->ack_num, 0);
    }
}

static void handle_data(tcp_socket_t *s, const void *data, size_t len, uint32_t sequence)
{
    LTRACEF("data %p, len %zu, sequence %u\n", data, len, sequence);

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));
    DEBUG_ASSERT(data);
    DEBUG_ASSERT(len > 0);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    /* see if it matches our current window */
    uint32_t sequence_top = sequence + len - 1;
    if (SEQUENCE_LTE(sequence, s->rx_win_low) && SEQUENCE_GTE(sequence_top, s->rx_win_low)) {
        /* it intersects the bottom of our window, so it's in order */

        /* copy the data we need to our cbuf */
        size_t offset = sequence - s->rx_win_low;
        size_t copy_len = MIN(s->rx_win_high - s->rx_win_low, len - offset);

        DEBUG_ASSERT(offset < len);

        LTRACEF("copying from offset %zu, len %zu\n", offset, copy_len);

        s->rx_win_low += copy_len;

        cbuf_write(&s->rx_buffer, (uint8_t *)data + offset, copy_len, false);
        event_signal(&s->rx_event, true);

        // XXX set up a delayed ack here when we have timers
        send_ack(s);
    } else {
        // either out of order or completely out of our window, drop
        // duplicately ack the last thing we really got
        send_ack(s);
    }
}

static status_t tcp_socket_send(tcp_socket_t *s, const void *data, size_t len, tcp_flags_t flags, 
    const void *options, size_t options_length, uint32_t sequence)
{
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));
    DEBUG_ASSERT(len == 0 || data);
    DEBUG_ASSERT(options_length == 0 || options);
    DEBUG_ASSERT((options_length % 4) == 0);

    // calculate the new right edge of the rx window
    uint32_t rx_win_high = s->rx_win_low + s->rx_win_size - cbuf_space_used(&s->rx_buffer) - 1;

    LTRACEF("rx_win_low %u rx_win_size %u read_buf_len %d, new win high %u\n",
        s->rx_win_low, s->rx_win_size, cbuf_space_used(&s->rx_buffer), rx_win_high);

    uint16_t win_size;
    if (SEQUENCE_GTE(rx_win_high, s->rx_win_high)) {
        s->rx_win_high = rx_win_high;
        win_size = rx_win_high - s->rx_win_low;
    } else {
        // the window size has shrunk, but we can't move the
        // right edge of the window backwards
        win_size = s->rx_win_high - s->rx_win_low;
    }

#if 0
    // we are piggybacking a pending ACK, so clear the delayed ACK timer
    if (flags & PKT_ACK) {
        if (cancel_net_timer(&s->ack_delay_timer) == 0)
            dec_socket_ref(s);
    }
#endif

    status_t err = tcp_send(s->remote_ip, s->remote_port, s->local_ip, s->local_port, data, len, flags,
            options, options_length, (flags & PKT_ACK) ? s->rx_win_low : 0, sequence, win_size);

    return err;
}

static void send_ack(tcp_socket_t *s)
{
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    if (s->state != STATE_ESTABLISHED && s->state != STATE_CLOSE_WAIT)
        return;

    tcp_socket_send(s, NULL, 0, PKT_ACK, NULL, 0, s->tx_win_low);
}

static status_t tcp_send(ipv4_addr dest_ip, uint16_t dest_port, ipv4_addr src_ip, uint16_t src_port, const void *buf,
    size_t len, tcp_flags_t flags, const void *options, size_t options_length, uint32_t ack, uint32_t sequence, uint16_t window_size)
{
    DEBUG_ASSERT(len == 0 || buf);
    DEBUG_ASSERT(options_length == 0 || options);
    DEBUG_ASSERT((options_length % 4) == 0);

    pktbuf_t *p = pktbuf_alloc();
    if (!p)
        return ERR_NO_MEMORY;

    tcp_header_t *header = pktbuf_prepend(p, sizeof(tcp_header_t) + options_length);
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
    if (len > 0)
        pktbuf_append_data(p, buf, len);

    /* compute the checksum */
    tcp_pseudo_header_t pheader;
    pheader.source_addr = src_ip;
    pheader.dest_addr = dest_ip;
    pheader.zero = 0;
    pheader.protocol = IP_PROTO_TCP;
    pheader.tcp_length = htons(p->dlen);

    header->checksum = cksum_pheader(&pheader, p->data, p->dlen);

    if (LOCAL_TRACE) {
        printf("sending ");
        dump_tcp_header(header);
    }

    /* send it */
    return minip_ipv4_send(p, dest_ip, IP_PROTO_TCP);
}

static void handle_ack(tcp_socket_t *s, uint32_t sequence, uint32_t win_size)
{
    LTRACEF("socket %p ack sequence %u, win_size %u\n", s, sequence, win_size);

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    LTRACEF("s %p, tx_win_low %u tx_win_high %u tx_highest_seq %u bufsize %zu offset %zu\n",
            s, s->tx_win_low, s->tx_win_high, s->tx_highest_seq, s->tx_buffer_size, s->tx_buffer_offset);
    if (SEQUENCE_LTE(sequence, s->tx_win_low)) {
        /* they're acking stuff we've already received an ack for */
        return;
    } else if (SEQUENCE_GT(sequence, s->tx_highest_seq)) {
        /* they're acking stuff we haven't sent */
        return;
    } else {
        /* their ack is somewhere in our window */
        uint32_t acked_len;

        acked_len = (sequence - s->tx_win_low);

        LTRACEF("acked len %u\n", acked_len);

        DEBUG_ASSERT(acked_len <= s->tx_buffer_size);
        DEBUG_ASSERT(acked_len <= s->tx_buffer_offset);

        memmove(s->tx_buffer, s->tx_buffer + acked_len, s->tx_buffer_offset - acked_len);

        s->tx_buffer_offset -= acked_len;
        s->tx_win_low += acked_len;
        s->tx_win_high = s->tx_win_low + win_size;

        /* cancel or reset our retransmit timer */
        if (s->tx_win_low == s->tx_highest_seq) {
            net_timer_cancel(&s->retransmit_timer);
        } else {
            net_timer_set(&s->retransmit_timer, &handle_retransmit_timeout, s, RETRANSMIT_TIMEOUT);
        }

        /* we have opened the transmit buffer */
        event_signal(&s->tx_event, true);
    }
}

static ssize_t tcp_write_pending_data(tcp_socket_t *s)
{
    LTRACEF("s %p, tx_win_low %u tx_win_high %u tx_highest_seq %u bufsize %zu offset %zu\n",
            s, s->tx_win_low, s->tx_win_high, s->tx_highest_seq, s->tx_buffer_size, s->tx_buffer_offset);

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));
    DEBUG_ASSERT(s->tx_buffer_size > 0);
    DEBUG_ASSERT(s->tx_buffer_offset <= s->tx_buffer_size);

    /* do we have any new data to send? */
    uint32_t outstanding = (s->tx_highest_seq - s->tx_win_low);
    uint32_t pending = s->tx_buffer_offset - outstanding;
    LTRACEF("outstanding %u, pending %u\n", outstanding, pending);

    /* send packets that cover the pending area of the window */
    uint32_t offset = 0;
    while (offset < pending) {
        uint32_t tosend = MIN(s->mss, pending - offset);

        tcp_socket_send(s, s->tx_buffer + outstanding + offset, tosend, PKT_ACK|PKT_PSH, NULL, 0, s->tx_highest_seq);
        s->tx_highest_seq += tosend;
        offset += tosend;
    }

    /* reset the retransmit timer if we sent anything */
    if (offset > 0) {
        net_timer_set(&s->retransmit_timer, &handle_retransmit_timeout, s, RETRANSMIT_TIMEOUT);
    }

    return offset;
}

static ssize_t tcp_retransmit(tcp_socket_t *s)
{
    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    if (s->state != STATE_ESTABLISHED && s->state != STATE_FIN_WAIT_1)
        return 0;

    /* how much data have we sent but not gotten an ack for? */
    uint32_t outstanding = (s->tx_highest_seq - s->tx_win_low);
    if (outstanding == 0)
        return 0;

    uint32_t tosend = MIN(s->mss, outstanding);

    LTRACEF("s %p, tosend %u seq %u\n", s, tosend, s->tx_win_low);
    tcp_socket_send(s, s->tx_buffer, tosend, PKT_ACK|PKT_PSH, NULL, 0, s->tx_win_low);

    return tosend;
}

static void handle_retransmit_timeout(void *_s)
{
    tcp_socket_t *s = _s;

    LTRACEF("s %p\n", s);

    DEBUG_ASSERT(s);

    mutex_acquire(&s->lock);

    if (s->state == STATE_CLOSED)
        goto done;

    tcp_retransmit(s);

    net_timer_set(&s->retransmit_timer, &handle_retransmit_timeout, s, RETRANSMIT_TIMEOUT);

done:
    mutex_release(&s->lock);
}

static void tcp_remote_close(tcp_socket_t *s)
{
    LTRACEF("s %p\n", s);

    DEBUG_ASSERT(s);
    DEBUG_ASSERT(is_mutex_held(&s->lock));

    if (s->state == STATE_CLOSED)
        return;

    s->state = STATE_CLOSED;

    list_delete(&s->node);
    net_timer_cancel(&s->retransmit_timer);

    // wake up any waiters
    event_signal(&s->rx_event, true);
    event_signal(&s->tx_event, true);
}


static tcp_socket_t *lookup_socket(ipv4_addr remote_ip, ipv4_addr local_ip, uint16_t remote_port, uint16_t local_port)
{
    LTRACEF("remote ip 0x%x local ip 0x%x remote port %u local port %u\n", remote_ip, local_ip, remote_port, local_port);

    /* XXX replace with something faster, like a hash table */
    tcp_socket_t *s;
    list_for_every_entry(&tcp_socket_list, s, tcp_socket_t, node) {
        if (s->state == STATE_LISTEN) {
            /* sockets in listen state only care about local port */
            if (s->local_port == local_port) {
                return s;
            }
        } else {
            /* full check */
            if (s->remote_ip == remote_ip &&
                s->local_ip == local_ip &&
                s->remote_port == remote_port &&
                s->local_port == local_port) {

                return s;
            }
        }
    }

    return NULL;
}

static tcp_socket_t *create_tcp_socket(bool alloc_buffers)
{
    tcp_socket_t *s;

    s = calloc(1, sizeof(tcp_socket_t));
    if (!s)
        return NULL;

    mutex_init(&s->lock);

    s->state = STATE_CLOSED;
    s->rx_win_size = DEFAULT_RX_WINDOW_SIZE;
    event_init(&s->rx_event, false, 0);

    s->mss = DEFAULT_MSS;

    s->tx_win_low = rand();
    s->tx_win_high = s->tx_win_low;
    s->tx_highest_seq = s->tx_win_low;
    event_init(&s->tx_event, true, 0);

    if (alloc_buffers) {
        // XXX check for error
        s->rx_buffer_raw = malloc(s->rx_win_size);
        cbuf_initialize_etc(&s->rx_buffer, s->rx_win_size, s->rx_buffer_raw);

        s->tx_buffer_size = DEFAULT_TX_BUFFER_SIZE;
        s->tx_buffer = malloc(s->tx_buffer_size);
    }

    sem_init(&s->accept_sem, 0);

    return s;
}

/* user api */

status_t tcp_open_listen(tcp_socket_t **handle, uint16_t port)
{
    tcp_socket_t *s;

    if (!handle)
        return ERR_INVALID_ARGS;

    s = create_tcp_socket(false);
    if (!s)
        return ERR_NO_MEMORY;

    // XXX see if there's another listen socket already on this port

    s->local_port = port;

    /* go to listen state */
    s->state = STATE_LISTEN;

    /* add it to the socket list */
    list_add_head(&tcp_socket_list, &s->node);

    *handle = s;

    return NO_ERROR;
}

status_t tcp_accept(tcp_socket_t *listen_socket, tcp_socket_t **accept_socket)
{
    if (!listen_socket || !accept_socket)
        return ERR_INVALID_ARGS;

    tcp_socket_t *s = listen_socket;

    /* block to accept a socket */
    sem_wait(&s->accept_sem);

    mutex_acquire(&s->lock);

    /* we got here, grab the accepted socket and return */
    DEBUG_ASSERT(s->accepted);
    *accept_socket = s->accepted;
    s->accepted = NULL;

    mutex_release(&s->lock);

    return NO_ERROR;
}

ssize_t tcp_read(tcp_socket_t *socket, void *buf, size_t len)
{
    if (!socket)
        return ERR_INVALID_ARGS;
    if (len == 0)
        return 0;
    if (!buf)
        return ERR_INVALID_ARGS;

    tcp_socket_t *s = socket;

    ssize_t ret = 0;
retry:
    /* block on available data */
    event_wait(&s->rx_event);

    mutex_acquire(&s->lock);

    /* try to read some data from the receive buffer, even if we're closed */
    ret = cbuf_read(&s->rx_buffer, buf, len, false);
    if (ret == 0) {
        /* check to see if we've closed */
        if (s->state != STATE_ESTABLISHED) {
            ret = ERR_CHANNEL_CLOSED;
            goto out;
        }

        /* we must have raced with another thread then */
        mutex_release(&s->lock);
        goto retry;
    }

    /* we've read something, make sure the other end knows that our window is opening */
    uint32_t new_rx_win_size = s->rx_win_size - cbuf_space_used(&s->rx_buffer);

    /* if we've opened it enough, send an ack */
    if (new_rx_win_size >= s->mss && s->rx_win_high - s->rx_win_low < s->mss)
        send_ack(s);

out:
    mutex_release(&s->lock);

    return ret;
}

ssize_t tcp_write(tcp_socket_t *socket, const void *buf, size_t len)
{
    if (!socket)
        return ERR_INVALID_ARGS;
    if (len == 0)
        return 0;
    if (!buf)
        return ERR_INVALID_ARGS;

    tcp_socket_t *s = socket;

    size_t off = 0;
    while (off < len) {
        LTRACEF("off %u, len %u\n", off, len);

        /* wait for the tx buffer to open up */
        event_wait(&s->tx_event);
        LTRACEF("after event_wait\n");

        mutex_acquire(&s->lock);

        /* check to see if we've closed */
        if (s->state != STATE_ESTABLISHED && s->state != STATE_CLOSE_WAIT) {
            mutex_release(&s->lock);
            return ERR_CHANNEL_CLOSED;
        }

        DEBUG_ASSERT(s->tx_buffer_size > 0);
        DEBUG_ASSERT(s->tx_buffer_offset <= s->tx_buffer_size);

        /* figure out how much data to copy in */
        size_t to_copy = MIN(s->tx_buffer_size - s->tx_buffer_offset, len - off);
        if (to_copy == 0) {
            mutex_release(&s->lock);
            continue;
        }

        memcpy(s->tx_buffer + s->tx_buffer_offset, (uint8_t *)buf + off, to_copy);
        s->tx_buffer_offset += to_copy;

        /* if this has completely filled it, unsignal the event */
        DEBUG_ASSERT(s->tx_buffer_offset <= s->tx_buffer_size);
        if (s->tx_buffer_offset == s->tx_buffer_size) {
            event_unsignal(&s->tx_event);
        }

        /* send as much data as we can */
        tcp_write_pending_data(s);

        off += to_copy;

        mutex_release(&s->lock);
    }

    return len;
}

status_t tcp_close(tcp_socket_t *socket)
{
    if (!socket)
        return ERR_INVALID_ARGS;

    tcp_socket_t *s = socket;

    mutex_acquire(&s->lock);

    status_t err;
    switch (s->state) {
        case STATE_CLOSED:
        case STATE_LISTEN:
            break;
        case STATE_ESTABLISHED:
            tcp_socket_send(s, NULL, 0, PKT_ACK|PKT_FIN, NULL, 0, s->tx_win_low);
            s->state = STATE_FIN_WAIT_1;
            // XXX should really stick around and wait for them to FIN us
            break;
        case STATE_CLOSE_WAIT:
            tcp_socket_send(s, NULL, 0, PKT_ACK|PKT_FIN, NULL, 0, s->tx_win_low);
            s->state = STATE_LAST_ACK;
            // XXX should wait for them to ack
            break;
        default:
            PANIC_UNIMPLEMENTED;
    }

    /* we can kill it */
    if (list_in_list(&s->node))
        list_delete(&s->node);

    net_timer_cancel(&s->retransmit_timer);
    event_destroy(&s->tx_event);
    event_destroy(&s->rx_event);

    free(s->rx_buffer_raw);
    free(s->tx_buffer);

    mutex_release(&s->lock);

    free(s);
    err = NO_ERROR;

    return err;
}

/* debug stuff */
static int cmd_tcp(int argc, const cmd_args *argv)
{
    status_t err;

    if (argc < 2) {
notenoughargs:
        printf("ERROR not enough arguments\n");
usage:
        printf("usage: %s listen <port>\n", argv[0].str);
        return ERR_INVALID_ARGS;
    }

    if (!strcmp(argv[1].str, "listen")) {
        if (argc < 3) goto notenoughargs;

        tcp_socket_t *handle;

        err = tcp_open_listen(&handle, argv[2].u);
        printf("tcp_open_listen returns %d, handle %p\n", err, handle);

        tcp_socket_t *accepted;
        err = tcp_accept(handle, &accepted);
        printf("tcp_accept returns returns %d, handle %p\n", err, accepted);

        for (;;) {
            uint8_t buf[512];

            ssize_t err = tcp_read(accepted, buf, sizeof(buf));
            printf("tcp_read returns %ld\n", err);
            if (err < 0)
                break;
            if (err > 0) {
                //hexdump8(buf, err);
            }

            err = tcp_write(accepted, buf, err);
            printf("tcp_write returns %ld\n", err);
            if (err < 0)
                break;
        }

        err = tcp_close(handle);
        printf("tcp_close returns %d\n", err);
    } else {
        printf("ERROR unknown command\n");
        goto usage;
    }

    return NO_ERROR;
}

STATIC_COMMAND_START
STATIC_COMMAND("tcp", "tcp commands", &cmd_tcp)
STATIC_COMMAND_END(tcp);


// vim: set ts=4 sw=4 expandtab:
