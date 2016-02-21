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
#include "usb.hpp"
#include "prot/transport_usb_struct.h"

#include <err.h>
#include <debug.h>
#include <stdio.h>
#include <assert.h>
#include <trace.h>
#include <string.h>
#include <rand.h>
#include <target.h>
#include <compiler.h>
#include <platform.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <app/mocom/usb.h>

#include "mux.hpp"
#include "mocom_app.hpp"

#define LOCAL_TRACE 0
#define TRACE_PACKETS 0

namespace mocom {

#define NDUID_CHRLEN 40
#define NDUID_STRLEN (NDUID_CHRLEN + 1)

/* static configuration */
static ep_t inep;
static ep_t outep;
static uint interface_num;

usb_transport::usb_transport()
:   m_inep(inep),
    m_outep(outep),
    m_interface_num(interface_num)
{
}

usb_transport::~usb_transport()
{
    delete[] m_rx_buffer;
    delete[] m_tx_buffer;
}

static void usbll_dump_packet(const uint8_t *buf, size_t len, bool tx)
{
    const struct usbll_header *header = (const struct usbll_header *)buf;
    char traceheader[128];

#if LOCAL_TRACE
    //hexdump8_ex(buf, len, 0);
#endif

    snprintf(traceheader, sizeof(traceheader), "%s len %5d txid 0x%08x rxid 0x%08x command 0x%x: ",
            tx ? "TX" : "RX", (int)len, header->txid, header->rxid, header->command);

    switch (header->command) {
        case usbll_null:
            printf("%s null\n", traceheader);
            break;
        case usbll_rst:
            printf("%s rst\n", traceheader);
            break;
        case usbll_syn: {
            const struct usbll_syn_header *syn_header = (const struct usbll_syn_header *)(header + 1);
            char nduid[NDUID_STRLEN];
            memcpy(nduid, syn_header->nduid, NDUID_CHRLEN);
            nduid[NDUID_CHRLEN] = 0;

            if (len >= sizeof(struct usbll_header) + sizeof(struct usbll_syn_header))
                printf("%s syn, nduid '%s' mtu %u, heartbeat %u, timeout %u\n", 
                        traceheader, nduid, syn_header->mtu, syn_header->heartbeat_interval, syn_header->timeout);
            else
                printf("%s syn, nduid '%s' mtu %u\n", traceheader, nduid, syn_header->mtu);

            break;
        }
        case usbll_data:
            printf("%s data\n", traceheader);
            break;
        default:
            printf("%s unknown\n", traceheader);
            break;
    }
}

status_t usb_transport::usb_cb(usb_callback_op_t op, const union usb_callback_args *args)
{
    LTRACEF("op %u, args %p\n", op, args);

    if (op == USB_CB_ONLINE) {
        usbc_setup_endpoint(m_inep, USB_IN, 0x40);
        usbc_setup_endpoint(m_outep, USB_OUT, 0x40);

        m_online = true;
    } else if (op == USB_CB_OFFLINE) {
        m_online = false;
    }

    return NO_ERROR;
}

status_t usb_transport::ep_cb_rx(ep_t endpoint, usbc_transfer_t *t)
{
#if 0 && LOCAL_TRACE
    LTRACEF("ep %u transfer %p\n", endpoint, t);
    usbc_dump_transfer(t);

    if (t->result >= 0) {
        hexdump8(t->buf, t->bufpos);
    }
#endif

    m_rx_transfer_queued = false;

    the_app->signal_irq();

    return NO_ERROR;
}

status_t usb_transport::ep_cb_tx(ep_t endpoint, usbc_transfer_t *t)
{
    m_tx_transfer_queued = false;

    the_app->signal_irq();

    return NO_ERROR;
}

static void get_nduid(uint8_t *buf)
{
    for (int i = 0; i < NDUID_CHRLEN; i++) {
        buf[i] = '0' + i;
    }
}

bool usb_transport::prepare_tx_packet()
{
    DEBUG_ASSERT(m_tx_transfer_queued == false);

    struct usbll_header *header = (struct usbll_header *)m_tx_buffer;
    header->magic = USBLL_MAGIC;
    header->version = USBLL_VERSION;
    header->txid = m_txid;
    header->rxid = m_rxid;

    m_tx_transfer.buflen = 0;

    switch (m_state) {
        case STATE_LISTEN: {
            LTRACEF("STATE_LISTEN, constructing syn packet\n");
            // we need to send a syn packet
            header->command = usbll_syn;

            struct usbll_syn_header *syn = (struct usbll_syn_header *)(header + 1);
            get_nduid(syn->nduid);
            syn->mtu = 4096;
            syn->heartbeat_interval = 250;
            syn->timeout = 1000;
            syn->data_offset = sizeof(struct usbll_syn_header);
            syn->data_length = 0;

            m_tx_transfer.buflen = sizeof(*header) + sizeof(*syn);

            m_state = STATE_SYNSENT;
            return true;
        }
        case STATE_SYNSENT:
            if (current_time() - m_last_tx_time > 1000) {
                // retransmit SYN
                m_state = STATE_LISTEN;
            }
            break;
        case STATE_ESTABLISHED: {
            if (current_time() - m_last_tx_time > 250) { // XXX use negotiated numbers
                header->command = usbll_null;
                m_tx_transfer.buflen = sizeof(*header);
                return true;
            }

            // ask the mux layer for any data
            ssize_t len = m_mux->prepare_tx_packet((uint8_t *)(header + 1), USB_TRANSFER_LEN - sizeof(struct usbll_header));
            if (len > 0) {
                header->command = usbll_data;
                m_tx_transfer.buflen = sizeof(struct usbll_header) + len;
                // make sure we're not an even multiple of a usb packet size
                if ((m_tx_transfer.buflen % 64) == 0)
                    m_tx_transfer.buflen++;
                DEBUG_ASSERT(m_tx_transfer.buflen <= USB_TRANSFER_LEN);
                return true;
            }
            break;
        }
        default:
            //LTRACEF("other state, doing nothing (for now)\n");
            ;
    }

    return false;
}

bool usb_transport::handle_rx_packet(const uint8_t *buf, size_t len)
{
    DEBUG_ASSERT(buf);
    DEBUG_ASSERT(len <= USB_TRANSFER_LEN);

    LTRACEF("buf %p, len %zu\n", buf, len);

    const struct usbll_header *header = (const struct usbll_header *)buf;

    if (len < sizeof(struct usbll_header))
        return false;
    if (header->magic != USBLL_MAGIC)
        return false;
    if (header->version != USBLL_VERSION)
        return false;

    LTRACEF("state %u, command %u\n", m_state, header->command);
#if TRACE_PACKETS
    usbll_dump_packet(buf, len, false);
#endif

    switch (m_state) {
        case STATE_LISTEN:
        case STATE_SYNSENT:
            // they're synning us back
            if (header->command == usbll_syn) {
                m_rxid = header->txid;
                if (m_state == STATE_SYNSENT) {
                    LTRACEF("moving to ESTABLISHED state\n");
                    m_state = STATE_ESTABLISHED;
                    m_mux->set_online(true);
                }
            } else {
                LTRACEF("non SYN packet in LISTEN/SYNSENT state\n");
            }
            break;
        case STATE_ESTABLISHED:
            if (header->command == usbll_data) {
                // pass it up to the mux
                m_mux->process_rx_packet((const uint8_t * )(header + 1), len - sizeof(struct usbll_header));
            } else if (header->command == usbll_null) {
                // we don't care
            } else {
                LTRACEF("non DATA/NULL packet in ESTABLISHED state\n");
            }
    }

    return true;
}

lk_time_t usb_transport::do_work()
{
    LTRACE_ENTRY;

    // if we're offline, nothing to do
    if (m_online == false) {
        // nothing to do
        return 1000; // tell the main app to try again in 1000ms
    }

    // see if we're not receiving for some reason
    if (!m_rx_transfer_queued) {
        // potentially process data here
        if (m_rx_transfer.bufpos > 0)
            handle_rx_packet((const uint8_t *)m_rx_transfer.buf, m_rx_transfer.bufpos);

        m_rx_transfer.callback = &ep_cb_rx_static;
        m_rx_transfer.result = 0;
        m_rx_transfer.buf = m_rx_buffer;
        m_rx_transfer.buflen = USB_TRANSFER_LEN;
        m_rx_transfer.bufpos = 0;
        m_rx_transfer.extra = this;
        m_rx_transfer_queued = true;

        usbc_queue_rx(m_outep, &m_rx_transfer);
    }

    // check to see if we have some pending tx data
    if (!m_tx_transfer_queued) {
        if (prepare_tx_packet()) {
            // queue it
            m_tx_transfer.callback = &ep_cb_tx_static;
            m_tx_transfer.result = 0;
            m_tx_transfer.buf = m_tx_buffer;
            m_tx_transfer.bufpos = 0;
            m_tx_transfer.extra = this;
            m_tx_transfer_queued = true;
            m_last_tx_time = current_time();

#if TRACE_PACKETS
            usbll_dump_packet(m_tx_buffer, m_tx_transfer.buflen, true);
#endif

            usbc_queue_tx(m_inep, &m_tx_transfer);
        }
    }

    return INFINITE_TIME;
}

status_t usb_transport::init()
{
#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

    // build a descriptor for it
    uint8_t if_descriptor[] = {
        0x09,           /* length */
        INTERFACE,      /* type */
        (uint8_t)m_interface_num,/* interface num */
        0x00,           /* alternates */
        0x02,           /* endpoint count */
        0xff,           /* interface class */
        0x47,           /* interface subclass */
        0x11,           /* interface protocol */
        0x00,           /* string index */

        /* endpoint IN */
        0x07,           /* length */
        ENDPOINT,       /* type */
        (uint8_t)(m_inep | 0x80),  /* address: 1 IN */
        0x02,           /* type: bulk */
        W(64),          /* max packet size: 64 */
        00,             /* interval */

        /* endpoint OUT */
        0x07,           /* length */
        ENDPOINT,       /* type */
        (uint8_t)m_outep,       /* address: 1 OUT */
        0x02,           /* type: bulk */
        W(64),          /* max packet size: 64 */
        00,             /* interval */
    };

    LTRACEF("epin %u, epout %u\n", m_inep, m_outep);

    usb_append_interface_lowspeed(if_descriptor, sizeof(if_descriptor));
    usb_append_interface_highspeed(if_descriptor, sizeof(if_descriptor));

    usb_register_callback(usb_cb_static, this);

    // clear the state
    m_state = STATE_LISTEN;
    m_online = false;

    // allocate some buffers
    m_rx_buffer = new uint8_t[USB_TRANSFER_LEN];
    m_rx_transfer = { };
    m_rx_transfer_queued = false;
    m_tx_buffer = new uint8_t[USB_TRANSFER_LEN];
    m_tx_transfer = { };
    m_tx_transfer_queued = false;

    m_rxid = 0;
    m_txid = rand();
    m_last_tx_time = 0;

    return NO_ERROR;

#undef W
#undef W3
}

} // namespace mocom

// C api to configure the endpoints
status_t mocom_configure_usb_endpoints(uint _interface, ep_t _inep, ep_t _outep)
{
    LTRACEF("interface %u, inep %u, outep %u\n", _interface, _inep, _outep);

    mocom::inep = _inep;
    mocom::outep = _outep;
    mocom::interface_num = _interface;

    return NO_ERROR;
}

