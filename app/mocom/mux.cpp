/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include "mux.hpp"
#include "prot/mux.h"

#include <err.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <trace.h>
#include <string.h>
#include <rand.h>
#include <target.h>
#include <compiler.h>
#include <platform.h>
#include <lib/cksum.h>

#include "channel.hpp"

#define LOCAL_TRACE 0
#define TRACE_PACKETS 0

namespace mocom {

static const char *control_op_to_string(uint32_t op)
{
    switch (op) {
        case PMUX_CONTROL_NONE:
            return "none";
        case PMUX_CONTROL_CHANNEL_CLOSED:
            return "closed";
        case PMUX_CONTROL_OPEN:
            return "open";
        case PMUX_CONTROL_OPEN_COMMAND:
            return "openc";
        case PMUX_CONTROL_CLOSE:
            return "close";
        default:
            return "unknown";
    }
}

#define PMUX_RX 0
#define PMUX_TX 1

static void dump_packet(const void *buf, size_t len, int txrx)
{
#define TXRX ((txrx == PMUX_TX) ? "TX" : "RX")

    if (!buf || len == 0) {
//      printf("%s NULL\n", TXRX);
        return;
    }

    struct pmux_header *header = (struct pmux_header *)buf;

    if ((header->flags & PMUX_FLAG_ACK)) {
        printf("%s ACK  c %-6d seq %-8d len %-8d tlen %-8d\n", 
                TXRX, header->channel, header->sequence, header->payload_len, header->total_len);
    } else {
        if (header->channel == PMUX_CHANNEL_CONTROL) {
            struct pmux_control_header *control = (struct pmux_control_header *)header->data;
            printf("%s DATA c %-6d seq %-8d len %-8d tlen %-8d CONTROL op %d %6s c %-6d len %-6d\n", TXRX, 
                header->channel, header->sequence, header->payload_len, header->total_len, 
                control->op, control_op_to_string(control->op), control->channel, control->len);
        } else {
            printf("%s DATA c %-6d seq %-8d len %-8d tlen %-8d crc %08x\n", TXRX, 
                header->channel, header->sequence, header->payload_len, header->total_len, header->crc);
        }
    }
#undef TXRX
}

void mux::init()
{
    // create a default control channel
    control_channel *cc = new control_channel(*this, PMUX_CHANNEL_CONTROL);
    add_channel(cc);
}

channel *mux::find_channel(uint32_t num)
{
    channel *c;
    list_for_every_entry(&m_channel_list, c, channel, m_list_node) {
        if (c->get_num() == num)
            return c;
    }

    return nullptr;
}

bool mux::add_channel(channel *c)
{
    if (find_channel(c->get_num()))
        return false;

    list_add_head(&m_channel_list, &c->m_list_node);
    return true;
}

void mux::remove_channel(channel *c)
{
    list_delete(&c->m_list_node);
}

void mux::set_online(bool online)
{
    LTRACEF("online %u\n", online);
}

void mux::process_rx_packet(const uint8_t *buf, size_t len)
{
    LTRACEF("buf %p, len %zu\n", buf, len);

    const struct pmux_header *header = (const struct pmux_header *)buf;

    if (len < sizeof(struct pmux_header))
        return;
    if (header->magic != PMUX_MAGIC)
        return;
    if (header->version != PMUX_VERSION)
        return;

#if TRACE_PACKETS
    dump_packet(buf, len, PMUX_RX);
#endif

    channel *c = find_channel(header->channel);
    if (c) {
        if (header->flags & PMUX_FLAG_ACK) {
            // this is an ack for us
            if (header->sequence > c->m_acked_tx_sequence && header->sequence <= c->m_tx_sequence) {
                c->m_acked_tx_sequence = header->sequence;
                if (c->m_acked_tx_sequence == c->m_tx_sequence) {
                    // we are completely acked, so we can kill the tx queue
                    c->tx_complete();
                }
            } else {
                // ack out of sequence
            }
        } else {
            // we have received data
            if (header->sequence == (c->m_rx_sequence + 1)) {
                // in sequence from the other side
                c->process_rx_packet((uint8_t *)(header + 1), len - sizeof(*header));
                c->m_rx_sequence = header->sequence;
                c->m_pending_ack_sequence = header->sequence;
                c->m_pending_ack = true;
            } else if (header->sequence <= c->m_rx_sequence) {
                // they are retransmitting for some reason, re-ack it
                c->m_pending_ack_sequence = c->m_rx_sequence;
                c->m_pending_ack = true;
            }
        }
    } else {
        TRACEF("packet for non open channel %u\n", header->channel);
        // XXX send CONTROL_CLOSED
        PANIC_UNIMPLEMENTED;
    }
}

ssize_t mux::prepare_tx_packet(uint8_t *buf, size_t len)
{
    //LTRACEF("buf %p, len %zu\n", buf, len);

    // see if any channels need an ack
    channel *c;
    list_for_every_entry(&m_channel_list, c, channel, m_list_node) {
        if (c->m_pending_ack) {
            c->m_pending_ack = false;

            struct pmux_header *header = (struct pmux_header *)buf;

            header->magic = PMUX_MAGIC;
            header->version = PMUX_VERSION;
            header->flags = PMUX_FLAG_ACK;
            header->channel = c->get_num();
            header->sequence = c->m_pending_ack_sequence;
            header->payload_len = 0;
            header->total_len = sizeof(struct pmux_header);

#if TRACE_PACKETS
            dump_packet(buf, header->total_len, true);
#endif

            LTRACEF("acking channel %u\n", c->get_num());
            return header->total_len;
        }
    }

    // XXX handle retransmits


    // see if any channels have data to send
    list_for_every_entry(&m_channel_list, c, channel, m_list_node) {
        if (c->m_tx_buf && c->m_acked_tx_sequence == c->m_tx_sequence) {
            // we have pending data and the other side has fully acked us
            struct pmux_header *header = (struct pmux_header *)buf;

            size_t data_len = MIN(c->m_tx_len, len - sizeof(struct pmux_header));

            header->magic = PMUX_MAGIC;
            header->version = PMUX_VERSION;
            header->flags = 0;
            header->channel = c->get_num();
            header->sequence = ++c->m_tx_sequence;
            header->payload_len = data_len;
            header->total_len = sizeof(struct pmux_header) + data_len;
            memcpy(header->data, c->m_tx_buf, data_len);
            header->crc = adler32(1, header->data, header->payload_len);

            // XXX for now can only consume the entire queued tx len
            DEBUG_ASSERT(data_len == c->m_tx_len);

#if TRACE_PACKETS
            dump_packet(buf, header->total_len, true);
#endif

            LTRACEF("sending %u bytes on channel %u\n", header->payload_len, c->get_num());
            return header->total_len;
        }
    }

    return -1;
}

};

