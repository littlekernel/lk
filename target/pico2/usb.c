/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <dev/usb.h>
#include <hw/usb.h>
#include <lk/err.h>

#include "target_p.h"

#define W(w) ((w) & 0xff), ((w) >> 8)

/*
 * Development-only placeholder IDs.
 *
 * Do not ship or publish devices with these values. Replace them with an
 * assigned VID/PID pair (for example, via USB-IF or pid.codes) before any use
 * outside local bring-up and testing.
 */
#define LK_USB_VID_PLACEHOLDER 0xffff
#define LK_USB_PID_PLACEHOLDER 0x0001

/* Minimal USB 2.0 device descriptor with no interfaces/classes. */
static const uint8_t dev_descr[] = {
    0x12,           /* descriptor length */
    DEVICE,         /* Device Descriptor type */
    W(0x0200),      /* USB Version */
    0x00,           /* class */
    0x00,           /* subclass */
    0x00,           /* protocol */
    64,             /* max packet size, ep0 */
    W(LK_USB_VID_PLACEHOLDER), /* vendor (development-only placeholder) */
    W(LK_USB_PID_PLACEHOLDER), /* product (development-only placeholder) */
    W(0x0001),      /* device release */
    0x01,           /* manufacturer string index */
    0x02,           /* product string index */
    0x03,           /* serial string index */
    0x01,           /* num configs */
};

/* Device qualifier for high-speed descriptor requests. */
static const uint8_t devqual_descr[] = {
    0x0a,
    DEVICE_QUALIFIER,
    W(0x0200),
    0x00,
    0x00,
    0x00,
    64,
    0x01,
    0x00,
};

/* Single configuration with zero interfaces. */
static const uint8_t cfg_descr[] = {
    0x09,
    CONFIGURATION,
    W(0x09),        /* total length */
    0x00,           /* # interfaces */
    0x01,           /* config value */
    0x00,           /* config string index */
    0x80,           /* bus powered */
    50,             /* 100mA */
};

static const uint8_t langid[] = { 0x04, 0x03, 0x09, 0x04 };

static usb_config config = {
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
    status_t st;

    st = usb_setup(&config);
    if (st < 0) {
        return;
    }

    (void)usb_add_string("LK", 1);
    (void)usb_add_string("LK Pico2", 2);
    (void)usb_add_string("0001", 3);

    (void)usb_start();
}
