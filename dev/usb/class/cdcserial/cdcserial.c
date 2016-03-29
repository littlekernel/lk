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
static event_t txevt = EVENT_INITIAL_VALUE(txevt, 0, EVENT_FLAG_AUTOUNSIGNAL);
static event_t rxevt = EVENT_INITIAL_VALUE(rxevt, 0, EVENT_FLAG_AUTOUNSIGNAL);

static volatile bool usb_online = false;

/* top level device descriptor */
static const uint8_t dev_descr[] = {
    0x12,           /* descriptor length */
    DEVICE,         /* Device Descriptor type */
    W(0x0200),      /* USB Version */
    0x00,           /* class: infer from interfaces */
    0x00,           /* subclass: infer from interfaces */
    0x00,           /* protocol infer from interfaces */
    64,             /* max packet size for endpoint 0 */
    W(0x9999),      /* vendor */
    W(0x9999),      /* product */
    W(0x9999),      /* release */
    0x2,            /* manufacturer string */
    0x1,            /* product string */
    0x0,            /* serialno string */
    0x1,            /* num configs */
};

/* high/low speed device qualifier */
static const uint8_t devqual_descr[] = {
    0x0a,             /* descriptor length */
    DEVICE_QUALIFIER, /* Device Qualifier type */
    W(0x0200),        /* USB version */
    0x00,             /* class */
    0x00,             /* subclass */
    0x00,             /* protocol */
    64,               /* max packet size for endpoint 0 */
    0x01,             /* num configs */
    0x00              /* reserved */
};

static const uint8_t cfg_descr[] = {
    0x09,           /* Length of Cfg Descr */
    CONFIGURATION,  /* Type of Cfg Descr */
    W(0x09),        /* Total Length (incl ifc, ept) */
    0x00,           /* # Interfaces */
    0x01,           /* Cfg Value */
    0x00,           /* Cfg String */
    0xc0,           /* Attributes -- self powered */
    250,            /* Power Consumption - 500mA */
};

static const uint8_t ctrl_if_descriptor[] = {
    0x09,           /* length */
    INTERFACE,      /* type */
    0x00,           /* interface num */
    0x00,           /* alternates */
    0x01,           /* endpoint count */
    0x02,           /* interface class */
    0x02,           /* interface subclass */
    0x01,           /* interface protocol */
    0x00,           /* string index */

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

    /*Union Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x06,   /* bDescriptorSubtype: Union func desc */
    0x00,   /* bMasterInterface: Communication class interface */
    0x01,   /* bSlaveInterface0: Data Class Interface */

    /* endpoint 1 IN */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x82,           /* address: 1 IN */
    0x03,           /* type: bulk */
    W(8),           /* max packet size: 64 */
    0xFF,           /* interval */
};

static const uint8_t data_if_descriptor[] = {
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
    0x01,      /* bEndpointAddress */
    0x02,      /* bmAttributes: Bulk */
    0x40,      /* wMaxPacketSize: */
    0x00,
    0x00,      /* bInterval: ignore for Bulk transfer */

    /*Endpoint IN Descriptor*/
    0x07,      /* bLength: Endpoint Descriptor size */
    ENDPOINT,  /* bDescriptorType: Endpoint */
    0x81,      /* bEndpointAddress */
    0x02,      /* bmAttributes: Bulk */
    0x40,      /* wMaxPacketSize: */
    0x00,
    0x00       /* bInterval */
};

static const uchar langid[] = { 0x04, 0x03, 0x09, 0x04 };

usb_config config = {
    .lowspeed = {
        .device = USB_DESC_STATIC(dev_descr),
        .device_qual = USB_DESC_STATIC(devqual_descr),
        .config = USB_DESC_STATIC(cfg_descr),
    },
    .highspeed = {
        .device = USB_DESC_STATIC(dev_descr),
        .device_qual = USB_DESC_STATIC(devqual_descr),
        .config = USB_DESC_STATIC(cfg_descr),
    },

    .langid = USB_DESC_STATIC(langid),
};

static status_t usb_register_cb(
    void *cookie,
    usb_callback_op_t op,
    const union usb_callback_args *args
)
{
    if (op == USB_CB_ONLINE) {
        usbc_setup_endpoint(1, USB_IN, 0x40, USB_BULK);
        usbc_setup_endpoint(1, USB_OUT, 0x40, USB_BULK);
        usbc_setup_endpoint(2, USB_IN, 0x40, USB_INTR);

        usb_online = true;
    }
    return NO_ERROR;
}

static status_t usb_setup_cb(
    void *cookie,
    usb_callback_op_t op,
    const union usb_callback_args *args
)
{
    if (op != USB_CB_SETUP_MSG) {
        return ERR_NOT_IMPLEMENTED;
    }

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

    return NO_ERROR;
}

status_t cdcserial_start(void)
{
    usb_setup(&config, &usb_setup_cb);

    usb_register_callback(&usb_register_cb, NULL);

    usb_append_interface_lowspeed(ctrl_if_descriptor, sizeof(ctrl_if_descriptor));
    usb_append_interface_lowspeed(data_if_descriptor, sizeof(data_if_descriptor));

    usb_append_interface_highspeed(ctrl_if_descriptor, sizeof(ctrl_if_descriptor));
    usb_append_interface_highspeed(data_if_descriptor, sizeof(data_if_descriptor));

    usb_add_string("LK", 1);
    usb_add_string("LK Industries", 2);

    usb_start();

    return NO_ERROR;
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