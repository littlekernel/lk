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

#include <dev/usb/class/cdcserial.h>

#include <assert.h>
#include <dev/udc.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <dev/usbc.h>
#include <err.h>
#include <kernel/event.h>
#include <sys/types.h>
#include <trace.h>

#define LOCAL_TRACE 0

#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

#define CDC_REQ_SEND_CMD               0x00
#define CDC_REQ_GET_RESP               0x01
#define CDC_REQ_SET_COMM_FEATURE       0x02
#define CDC_REQ_GET_COMM_FEATURE       0x03
#define CDC_REQ_CLEAR_COMM_FEATURE     0x04
#define CDC_REQ_SET_LINE_CODING        0x20
#define CDC_REQ_GET_LINE_CODING        0x21
#define CDC_REQ_SET_CONTROL_LINE_STATE 0x22
#define CDC_REQ_SEND_BREAK             0x23

#define EP0_MTU (64)
#define MAX_USB_ENDPOINT_PAIRS (16)

// NB: These must be kept in sync with the if_descriptors below.
#define CTRL_IN_EP_ADDR_OFFSET (0x0B)
#define DATA_IN_EP_ADDR_OFFSET (0x0B)
#define DATA_OUT_EP_ADDR_OFFSET (0x12)
#define LEADER_EP_NUMBER_OFFSET (0x1C)
#define FOLLOWER_EP_NUMBER_OFFSET (0x1D)

static event_t txevt = EVENT_INITIAL_VALUE(txevt, 0, EVENT_FLAG_AUTOUNSIGNAL);
static event_t rxevt = EVENT_INITIAL_VALUE(rxevt, 0, EVENT_FLAG_AUTOUNSIGNAL);

static volatile bool usb_online = false;

// A bitfield corresponding to the registered endpoints. When we get a
// USB_ONLINE event, these are the endpoints that we need to setup.
static volatile uint16_t registered_bulk_eps_in;
static volatile uint16_t registered_bulk_eps_out;
static volatile uint16_t registered_intr_eps_in;
static volatile uint16_t registered_intr_eps_out;

static uint8_t ctrl_if_descriptor[] = {
    0x09,           /* length */
    INTERFACE,      /* type */
    0x00,           /* interface num */
    0x00,           /* alternates */
    0x01,           /* endpoint count */
    0x02,           /* interface class */
    0x02,           /* interface subclass */
    0x01,           /* interface protocol */
    0x00,           /* string index */

    /* endpoint 1 IN */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x80,           /* address: 1 IN */
    0x03,           /* type: bulk */
    W(8),           /* max packet size: 8 */
    0xFF,           /* interval */

    /*Call Management Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x01,   /* bDescriptorSubtype: Call Management Func Desc */
    0x00,   /* bmCapabilities: D0+D1 */
    0x00,   /* bDataInterface: 1 */

    /*ACM Functional Descriptor*/
    0x04,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,   /* bmCapabilities */

    /* Union Functional Descriptor */
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x06,   /* bDescriptorSubtype: Union func desc */
    0x00,   /* bLeaderIngerface0: Communication class interface */
    0x00,   /* bFollowerInterface0: Data Class Interface */
};

static uint8_t data_if_descriptor[] = {
    /*Data class interface descriptor*/
    0x09,       /* bLength: Endpoint Descriptor size */
    INTERFACE,  /* bDescriptorType: */
    0x01,       /* bInterfaceNumber: Number of Interface */
    0x00,       /* bAlternateSetting: Alternate setting */
    0x02,       /* bNumEndpoints: Two endpoints used */
    0x0A,       /* bInterfaceClass: CDC */
    0x00,       /* bInterfaceSubClass: */
    0x00,       /* bInterfaceProtocol: */
    0x00,       /* iInterface: */

    /*Endpoint OUT Descriptor*/
    0x07,      /* bLength: Endpoint Descriptor size */
    ENDPOINT,  /* bDescriptorType: Endpoint */
    0x00,      /* bEndpointAddress */
    0x02,      /* bmAttributes: Bulk */
    0x40,      /* wMaxPacketSize: */
    0x00,
    0x00,      /* bInterval: ignore for Bulk transfer */

    /*Endpoint IN Descriptor*/
    0x07,      /* bLength: Endpoint Descriptor size */
    ENDPOINT,  /* bDescriptorType: Endpoint */
    0x80,      /* bEndpointAddress */
    0x02,      /* bmAttributes: Bulk */
    0x40,      /* wMaxPacketSize: */
    0x00,
    0x00       /* bInterval */
};


static status_t usb_register_cb(
    void *cookie,
    usb_callback_op_t op,
    const union usb_callback_args *args
)
{
    if (op == USB_CB_ONLINE) {
        for (int i = 0; i < MAX_USB_ENDPOINT_PAIRS; i++) {
            if ((0x1 << i) & registered_bulk_eps_in) {
                usbc_setup_endpoint(i, USB_IN, 0x40, USB_BULK);
            }
            if ((0x1 << i) & registered_bulk_eps_out) {
                usbc_setup_endpoint(i, USB_OUT, 0x40, USB_BULK);
            }
            if ((0x1 << i) & registered_intr_eps_in) {
                usbc_setup_endpoint(i, USB_IN, 0x40, USB_INTR);
            }
            if ((0x1 << i) & registered_intr_eps_out) {
                usbc_setup_endpoint(i, USB_OUT, 0x40, USB_INTR);
            }
        }

        usb_online = true;
    } else if (op == USB_CB_SETUP_MSG) {
        static uint8_t buf[EP0_MTU];
        const struct usb_setup *setup = args->setup;
        // NB: The host might send us some modem commands here. Since we're not a
        // real modem, we're probably okay to drop them on the floor and reply with
        // an acknowledgement.
        if (setup->length) {  // Has data phase?
            if (setup->request_type & 0x80) {
                // We have to send data?
                usbc_ep0_send(buf, setup->length, 64);
            } else {
                usbc_ep0_recv(buf, setup->length, NULL);
                usbc_ep0_ack();
            }
        } else {
            switch (setup->request) {
                case CDC_REQ_SEND_CMD:
                case CDC_REQ_GET_RESP:
                case CDC_REQ_SET_COMM_FEATURE:
                case CDC_REQ_GET_COMM_FEATURE:
                case CDC_REQ_CLEAR_COMM_FEATURE:
                case CDC_REQ_SET_LINE_CODING:
                case CDC_REQ_GET_LINE_CODING:
                case CDC_REQ_SET_CONTROL_LINE_STATE:
                case CDC_REQ_SEND_BREAK:
                    // Ack any command that we understand.
                    usbc_ep0_ack();
                    break;
            }
        }
    }

    return NO_ERROR;
}

status_t cdcserial_init(void)
{
    usb_register_callback(&usb_register_cb, NULL);
    return NO_ERROR;
}

void cdcserial_create_channel(int data_ep_addr, int ctrl_ep_addr)
{
    ctrl_if_descriptor[CTRL_IN_EP_ADDR_OFFSET]  = ctrl_ep_addr | 0x80;
    data_if_descriptor[DATA_IN_EP_ADDR_OFFSET]  = data_ep_addr | 0x80;
    data_if_descriptor[DATA_OUT_EP_ADDR_OFFSET] = data_ep_addr;

    ctrl_if_descriptor[LEADER_EP_NUMBER_OFFSET] =
        usb_get_current_iface_num_lowspeed();
    ctrl_if_descriptor[FOLLOWER_EP_NUMBER_OFFSET] =
        usb_get_current_iface_num_lowspeed() + 1;

    usb_append_interface_lowspeed(ctrl_if_descriptor, sizeof(ctrl_if_descriptor));
    usb_append_interface_lowspeed(data_if_descriptor, sizeof(data_if_descriptor));

    ctrl_if_descriptor[LEADER_EP_NUMBER_OFFSET] =
        usb_get_current_iface_num_highspeed();
    ctrl_if_descriptor[FOLLOWER_EP_NUMBER_OFFSET] =
        usb_get_current_iface_num_highspeed() + 1;

    usb_append_interface_highspeed(ctrl_if_descriptor, sizeof(ctrl_if_descriptor));
    usb_append_interface_highspeed(data_if_descriptor, sizeof(data_if_descriptor));

    registered_bulk_eps_in |= (0x1 << data_ep_addr);
    registered_bulk_eps_out |= (0x1 << data_ep_addr);
    registered_intr_eps_in |= (0x1 << ctrl_ep_addr);
}

static status_t usb_xmit_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
    event_signal(&txevt, false);
    return 0;
}

static status_t usb_recv_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
    event_signal(&rxevt, false);
    return 0;
}

// Write len bytes to the CDC Serial Virtual Com Port.
status_t cdcserial_write(size_t len, uint8_t *buf)
{
    LTRACEF("len = %d, buf = %p\n", len, buf);

    DEBUG_ASSERT(buf);

    if (!usb_online) {
        return ERR_NOT_READY;
    }

    usbc_transfer_t transfer = {
        .callback = &usb_xmit_cplt_cb,
        .result   = 0,
        .buf      = buf,
        .buflen   = len,
        .bufpos   = 0,
        .extra    = 0,
    };

    usbc_queue_tx(1, &transfer);
    event_wait(&txevt);

    return NO_ERROR;
}

// Read at most len bytes from the CDC Serial virtual Com Port. Returns the
// actual number of bytes read.
ssize_t cdcserial_read(size_t len, uint8_t *buf)
{
    LTRACEF("len = %d, buf = %p\n", len, buf);

    DEBUG_ASSERT(buf);

    if (!usb_online) {
        return ERR_NOT_READY;
    }

    usbc_transfer_t transfer = {
        .callback = &usb_recv_cplt_cb,
        .result = 0,
        .buf = buf,
        .buflen = len,
        .bufpos = 0,
        .extra = 0,
    };

    usbc_queue_rx(1, &transfer);
    event_wait(&rxevt);

    return transfer.bufpos;
}