/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
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

#include <lib/ndebug/ndebug.h>

#include <assert.h>
#include <dev/udc.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <err.h>
#include <kernel/event.h>
#include <lib/ndebug/shared_structs.h>
#include <string.h>
#include <trace.h>

#define LOCAL_TRACE 0

#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

#define IFACE_SUBCLASS_IDX (0x06)
#define IFACE_PROTOCOL_IDX (0x07)
#define IFACE_IN_ADDR_IDX  (0x0B)
#define IFACE_OUT_ADDR_IDX (0x12)

#define CH_TO_ADDR(CH) ((CH) + 1)
#define CHECK_CHANNEL(CH) \
        do { DEBUG_ASSERT((CH) < NDEBUG_CHANNEL_COUNT); } while (false);

#define SYS_EP_ADDR (CH_TO_ADDR(NDEBUG_CHANNEL_SYS))
#define USR_EP_ADDR (CH_TO_ADDR(NDEBUG_CHANNEL_USR))

#define NDEBUG_SUBCLASS (0x02)

#define NDEBUG_PROTOCOL_LK_SYSTEM (0x01)
#define NDEBUG_PROTOCOL_SERIAL_PIPE (0x02)

static usbc_transfer_t rx_transfer[NDEBUG_CHANNEL_COUNT];
static usbc_transfer_t tx_transfer[NDEBUG_CHANNEL_COUNT];

static event_t rx_event[NDEBUG_CHANNEL_COUNT];
static event_t tx_event[NDEBUG_CHANNEL_COUNT];

static event_t usb_online_event;

static const uint8_t bulk_pair_descriptor_template[] = {
    0x09,           /* length */
    INTERFACE,      /* type */
    0x00,           /* interface num - Unused, patched in by dev/usb */
    0x00,           /* alternates */
    0x02,           /* endpoint count */
    0xff,           /* interface class - User Deficned */
    0x00,           /* interface subclass - Patched by client */
    0x00,           /* interface protocol - Patched by client */
    0x00,           /* string index */

    /* endpoint 1 IN */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x80,           /* address - Patched by Client */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval - unused for bulk */

    /* endpoint 1 OUT */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x00,           /* address - Patched by client */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval - unused for bulk */
};

static void init_channel(uint8_t subclass, uint8_t protocol, uint8_t ep_addr)
{
    uint8_t desc[sizeof(bulk_pair_descriptor_template)];

    // Make a copy of the template descriptor and fill in the missing fields.
    memcpy(desc, bulk_pair_descriptor_template,
           sizeof(bulk_pair_descriptor_template));

    desc[IFACE_SUBCLASS_IDX] = subclass;
    desc[IFACE_PROTOCOL_IDX] = protocol;
    desc[IFACE_IN_ADDR_IDX] = ep_addr | 0x80;
    desc[IFACE_OUT_ADDR_IDX] = ep_addr;

    // Append the interfaces.
    usb_append_interface_lowspeed(desc, sizeof(desc));
    usb_append_interface_highspeed(desc, sizeof(desc));
}

static void setup_usb_endpoint(uint8_t ep_num)
{
    usbc_setup_endpoint(ep_num, USB_IN, 0x40, USB_BULK);
    usbc_setup_endpoint(ep_num, USB_OUT, 0x40, USB_BULK);
}

static status_t ndebug_register_cb(
    void *cookie,
    usb_callback_op_t op,
    const union usb_callback_args *args
)
{
    if (op == USB_CB_ONLINE) {
        for (channel_t ch = 0; ch < NDEBUG_CHANNEL_COUNT; ++ch) {
            setup_usb_endpoint(CH_TO_ADDR(ch));
        }
        event_signal(&usb_online_event, false);
    } else if (op == USB_CB_RESET || op == USB_CB_DISCONNECT ||
               op == USB_CB_OFFLINE) {
        event_unsignal(&usb_online_event);
    }
    return NO_ERROR;
}

void ndebug_init(void)
{
    for (channel_t ch = 0; ch < NDEBUG_CHANNEL_COUNT; ++ch) {
        event_init(&rx_event[ch], 0, EVENT_FLAG_AUTOUNSIGNAL);
        event_init(&tx_event[ch], 0, EVENT_FLAG_AUTOUNSIGNAL);
    }

    init_channel(NDEBUG_SUBCLASS, NDEBUG_PROTOCOL_LK_SYSTEM, SYS_EP_ADDR);
    init_channel(NDEBUG_SUBCLASS, NDEBUG_PROTOCOL_SERIAL_PIPE, USR_EP_ADDR);

    event_init(&usb_online_event, 0, 0);

    usb_register_callback(&ndebug_register_cb, NULL);
}

static status_t usb_xmit_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
    uint32_t channel = (uint32_t)t->extra;
    CHECK_CHANNEL(channel);

    event_signal(&tx_event[channel], false);
    return 0;
}

static status_t usb_recv_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
    uint32_t channel = (uint32_t)t->extra;
    CHECK_CHANNEL(channel);

    event_signal(&rx_event[channel], false);
    return 0;
}

ssize_t ndebug_usb_read(const channel_t ch, const size_t n,
                        const lk_time_t timeout, uint8_t *buf)
{
    CHECK_CHANNEL(ch);

    usbc_transfer_t *transfer = &rx_transfer[ch];

    transfer->callback = &usb_recv_cplt_cb;
    transfer->result = 0;
    transfer->buf = buf;
    transfer->buflen = n;
    transfer->bufpos = 0;
    transfer->extra = (void *)ch;

    usbc_queue_rx(CH_TO_ADDR(ch), transfer);
    status_t res = event_wait_timeout(&rx_event[ch], timeout);

    if (res != NO_ERROR) {
        return res;
    }

    if (transfer->result != NO_ERROR) {
        return transfer->result;
    }

    return transfer->bufpos;
}

ssize_t ndebug_usb_write(const channel_t ch, const size_t n,
                         const lk_time_t timeout, uint8_t *buf)
{
    CHECK_CHANNEL(ch);

    usbc_transfer_t *transfer = &tx_transfer[ch];

    transfer->callback = &usb_xmit_cplt_cb;
    transfer->result = 0;
    transfer->buf = buf;
    transfer->buflen = n;
    transfer->bufpos = 0;
    transfer->extra = (void *)ch;

    usbc_queue_tx(CH_TO_ADDR(ch), transfer);
    status_t res = event_wait_timeout(&tx_event[ch], timeout);

    if (res != NO_ERROR) {
        return res;
    }

    if (transfer->result != NO_ERROR) {
        return transfer->result;
    }

    return n;
}

static bool is_valid_connection_request(ssize_t n, const uint8_t *buf)
{
    LTRACEF("length: %ld, buf: 0x%p\n", n, buf);

    if (n < (ssize_t)sizeof(ndebug_ctrl_packet_t)) {
        dprintf(INFO, "Malformed Packet. Expected at least %u bytes, got %ld\n",
                sizeof(ndebug_ctrl_packet_t), n);
        return false;
    }

    ndebug_ctrl_packet_t *pkt = (ndebug_ctrl_packet_t *)buf;

    return pkt->magic == NDEBUG_CTRL_PACKET_MAGIC &&
           pkt->type == NDEBUG_CTRL_CMD_RESET;
}

status_t msg_host(
    const channel_t ch, const uint32_t message,
    const lk_time_t timeout, uint8_t *buf
)
{
    LTRACEF("message: %d\n", message);

    ndebug_ctrl_packet_t *pkt = (ndebug_ctrl_packet_t *)buf;
    pkt->magic = NDEBUG_CTRL_PACKET_MAGIC;
    pkt->type = message;

    ssize_t res =
        ndebug_usb_write(ch, sizeof(ndebug_ctrl_packet_t), timeout, buf);

    if (res == ERR_TIMED_OUT) {
        dprintf(INFO, "send message %d timed out\n", message);
    } else if (res < 0) {
        dprintf(INFO, "send message %d failed with error %ld\n", message, res);
    }
    return res;
}

status_t ndebug_await_connection(const channel_t ch, const lk_time_t timeout)
{
    LTRACEF("channel: %u, timeout: %u\n", ch, timeout);

    uint8_t buf[NDEBUG_MAX_PACKET_SIZE];

    status_t result = event_wait_timeout(&usb_online_event, timeout);
    if (result != NO_ERROR) {
        return result;
    }

    while (true) {
        ssize_t bytes =
            ndebug_usb_read(ch, NDEBUG_MAX_PACKET_SIZE, timeout, buf);

        if (bytes < 0) return bytes;
        if (bytes < (ssize_t)sizeof(ndebug_ctrl_packet_t)) continue;
        if (!is_valid_connection_request(bytes, buf)) {
            msg_host(ch, NDEBUG_CTRL_CMD_RESET, timeout, buf);
            continue;
        }

        if (msg_host(ch, NDEBUG_CTRL_CMD_ESTABLISHED, timeout, buf) < 0) {
            continue;
        }

        return NO_ERROR;
    }
}