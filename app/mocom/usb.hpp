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
#pragma once

#include <dev/usbc.h>
#include <dev/usb.h>
#include <kernel/spinlock.h>
#include "transport.hpp"
#include <lib/cpputils/nocopy.hpp>

namespace mocom {

class usb_transport : public transport {
public:
    usb_transport();
    virtual ~usb_transport();

    virtual status_t init() override;
    virtual lk_time_t do_work() override;

private:
    // in/out endpoints
    ep_t m_inep;
    ep_t m_outep;
    uint m_interface_num;

    enum {
        STATE_LISTEN,
        STATE_SYNSENT,
        STATE_ESTABLISHED,
    } m_state;
    volatile bool m_online;

    // transfer lock
    spin_lock_t lock;

    static const size_t USB_TRANSFER_LEN = 4096;

    // rx transfers
    bool            m_rx_transfer_queued;
    usbc_transfer_t m_rx_transfer;
    uint8_t        *m_rx_buffer = nullptr;

    // tx transfers
    bool            m_tx_transfer_queued;
    usbc_transfer_t m_tx_transfer;
    uint8_t        *m_tx_buffer = nullptr;

    uint32_t m_txid;
    uint32_t m_rxid;
    lk_time_t m_last_tx_time;

    // usb layer callbacks
    status_t usb_cb(usb_callback_op_t op, const union usb_callback_args *args);
    status_t ep_cb_rx(ep_t endpoint, usbc_transfer_t *t);
    status_t ep_cb_tx(ep_t endpoint, usbc_transfer_t *t);

    static status_t usb_cb_static(void *cookie, usb_callback_op_t op, const union usb_callback_args *args);
    static status_t ep_cb_rx_static(ep_t endpoint, usbc_transfer_t *t);
    static status_t ep_cb_tx_static(ep_t endpoint, usbc_transfer_t *t);

    // internal routines
    bool prepare_tx_packet();
    bool handle_rx_packet(const uint8_t *buf, size_t len);

    // no copy
    usb_transport(const usb_transport &);
    usb_transport& operator=(const usb_transport &);
};

inline status_t usb_transport::usb_cb_static(void *cookie, usb_callback_op_t op, const union usb_callback_args *args) {
    usb_transport *t = static_cast<usb_transport *>(cookie);

    return t->usb_cb(op, args);
}

inline status_t usb_transport::ep_cb_rx_static(ep_t endpoint, usbc_transfer_t *t)
{
    usb_transport *_t = static_cast<usb_transport *>(t->extra);
    return _t->ep_cb_rx(endpoint, t);
}

inline status_t usb_transport::ep_cb_tx_static(ep_t endpoint, usbc_transfer_t *t)
{
    usb_transport *_t = static_cast<usb_transport *>(t->extra);
    return _t->ep_cb_tx(endpoint, t);
}

inline transport *create_usb_transport()
{
    usb_transport *u = new usb_transport();
    return u;
}

}

