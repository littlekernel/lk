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
#include <err.h>
#include <debug.h>
#include <stdio.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <dev/usb/class/bulktest.h>
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
    0xff,           /* class */
    0xff,           /* subclass */
    0xff,           /* protocol */
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

void target_usb_setup(void)
{
    usb_setup(&config);
    printf("appending interfaces\n");

    usb_add_string("LK", 1);
    usb_add_string("LK Industries", 2);

    /* add our bulk endpoint class device */
    usb_class_bulktest_init(1, 1, 1);

    usb_start();
}
