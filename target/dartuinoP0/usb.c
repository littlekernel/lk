/*
 * Copyright (c) 2013-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/compiler.h>
#include <lk/debug.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <lk/err.h>
#include <hw/usb.h>
#include <lk/init.h>
#include <stdio.h>
#include <target.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

extern void append_usb_interfaces(void);

/* top level device descriptor */
static const uint8_t dev_descr[] = {
    0x12,           /* descriptor length */
    DEVICE,         /* Device Descriptor type */
    W(0x0200),      /* USB Version */
    0xff,           /* class */
    0xff,           /* subclass */
    0xff,           /* protocol */
    64,             /* max packet size, ept0 */
    W(0x18D1),      /* vendor */
    W(0xA010),      /* product */
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

void target_usb_setup(void) {
    usb_setup(&config);

    append_usb_interfaces();

    usb_add_string("LK", 1);
    usb_add_string("LK Industries", 2);

    usb_start();
}
