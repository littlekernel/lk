#include <err.h>
#include <debug.h>
#include <stdio.h>
#include <target.h>
#include <compiler.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <hw/usb.h>

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
    0x0,            /* manufacturer string */
    0x0,            /* product string */
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
    0x81,           /* address: 1 IN */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval */

    /* endpoint 1 OUT */
    0x07,           /* length */
    ENDPOINT,       /* type */
    0x01,           /* address: 1 OUT */
    0x02,           /* type: bulk */
    W(64),          /* max packet size: 64 */
    00,             /* interval */
};

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

void usbtest_usb_setup(void)
{
    usb_setup(&config);
    printf("appending interfaces\n");
    usb_append_interface_lowspeed(if_descriptor_lowspeed, sizeof(if_descriptor_lowspeed));
    usb_append_interface_highspeed(if_descriptor_lowspeed, sizeof(if_descriptor_lowspeed));
    usbc_setup_endpoint(1, USB_OUT, 64);
    usbc_setup_endpoint(1, USB_IN, 64);
}

// vim: set ts=4 sw=4 expandtab:
