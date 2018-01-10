/*
 * Copyright (c) 2013-2015 Travis Geiselbrecht
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
#include <app/cdcserialtest/cdcserialtest.h>
#include <err.h>
#include <debug.h>
#include <stdio.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <dev/usb/class/cdcserial.h>
#include <hw/usb.h>
#include <lk/init.h>

#define LOCAL_TRACE 0

#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

/* top level device descriptor */
static const uint8_t dev_descr[] = {
    0x12,           /* descriptor length */
    DEVICE,         /* Device Descriptor type */
    W(0x0200),      /* USB Version */
    239,           /* class */
    2,           /* subclass */
    1,           /* protocol */
    64,             /* max packet size, ept0 */
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
    0x0a,           /* len */
    DEVICE_QUALIFIER, /* Device Qualifier type */
    W(0x0200),      /* USB version */
    0x00,           /* class */
    0x00,           /* subclass */
    0x00,           /* protocol */
    64,             /* max packet size, ept0 */
    0x01,           /* num configs */
    0x00            /* reserved */
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

static const uchar langid[] = { 0x04, 0x03, 0x09, 0x04 };

static const uint8_t if_descriptor_lowspeed[] = {
    0x09,           /* length */
    INTERFACE,      /* type */
    0x01,           /* interface num */
    0x00,           /* alternates */
    0x02,           /* endpoint count */
    0xff,           /* interface class */
    0xff,           /* interface subclass */
    0x00,           /* interface protocol */
    0x00,           /* string index */

    /* endpoint 1 IN */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x83,           /* address: 1 IN */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval */

    /* endpoint 1 OUT */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x03,           /* address: 1 OUT */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval */
};

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

static status_t ep_cb_rx(ep_t endpoint, usbc_transfer_t *t);
static status_t ep_cb_tx(ep_t endpoint, usbc_transfer_t *t);

static cdcserial_channel_t cdc_channel;

static void queue_rx(void)
{
    static usbc_transfer_t transfer;
    static uint8_t buf[512];

    transfer.callback = &ep_cb_rx;
    transfer.result = 0;
    transfer.buf = &buf;
    transfer.buflen = sizeof(buf);
    transfer.bufpos = 0;
    transfer.extra = 0;

    usbc_queue_rx(3, &transfer);
}

static void queue_tx(void)
{
    static usbc_transfer_t transfer;
    static uint8_t buf[512];

    for (uint i = 0; i < sizeof(buf); i++) {
        buf[i] = ~i;
    }

    transfer.callback = &ep_cb_tx;
    transfer.result = 0;
    transfer.buf = &buf;
    transfer.buflen = sizeof(buf);
    transfer.bufpos = 0;
    transfer.extra = 0;

    usbc_queue_tx(3, &transfer);
}

static status_t ep_cb_rx(ep_t endpoint, usbc_transfer_t *t)
{
#if LOCAL_TRACE
    LTRACEF("ep %u transfer %p\n", endpoint, t);
    usbc_dump_transfer(t);

    if (t->result >= 0) {
        hexdump8(t->buf, t->bufpos);
    }
#endif

    if (t->result >= 0)
        queue_rx();

    return NO_ERROR;
}

static status_t ep_cb_tx(ep_t endpoint, usbc_transfer_t *t)
{
#if LOCAL_TRACE
    LTRACEF("ep %u transfer %p\n", endpoint, t);
    usbc_dump_transfer(t);
#endif

    if (t->result >= 0)
        queue_tx();

    return NO_ERROR;
}

static status_t usb_cb(void *cookie, usb_callback_op_t op, const union usb_callback_args *args)
{
    LTRACEF("cookie %p, op %u, args %p\n", cookie, op, args);

    if (op == USB_CB_ONLINE) {
        usbc_setup_endpoint(3, USB_IN, 0x40, USB_BULK);
        usbc_setup_endpoint(3, USB_OUT, 0x40, USB_BULK);

        queue_rx();
        queue_tx();
    }
    return NO_ERROR;
}

void target_usb_setup(void)
{
    usb_setup(&config);
    printf("appending interfaces\n");
    cdcserial_create_channel(&cdc_channel, 0x1, 0x2);
    cdctest_setup(&cdc_channel);

    usb_append_interface_lowspeed(if_descriptor_lowspeed, sizeof(if_descriptor_lowspeed));
    usb_append_interface_highspeed(if_descriptor_lowspeed, sizeof(if_descriptor_lowspeed));
    usb_register_callback(&usb_cb, NULL);

    usb_add_string("LK", 1);
    usb_add_string("LK Industries", 2);

    usb_start();
}
