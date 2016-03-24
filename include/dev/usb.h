/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#ifndef __DEV_USB_H
#define __DEV_USB_H

#include <sys/types.h>
#include <compiler.h>

__BEGIN_CDECLS

/* top level initialization for usb client, abstracts away the interfaces */
typedef struct {
    void *desc;
    size_t len;
    uint flags;
} usb_descriptor __ALIGNED(2);

#define USB_DESC_FLAG_STATIC (0x1)

#define USB_DESC_STATIC(x) { .desc = (void *)(x), .len = sizeof(x), .flags = USB_DESC_FLAG_STATIC }

typedef struct {
    usb_descriptor string;
    uint8_t id;
} usb_string;

/* complete usb config struct, passed in to usb_setup() */
typedef struct {
    struct usb_descriptor_speed {
        usb_descriptor device;
        usb_descriptor device_qual;
        usb_descriptor config;
    } lowspeed, highspeed;
    usb_descriptor langid;
} usb_config;

/* external code needs to set up the usb stack via the following calls */
status_t usb_setup(usb_config *config);

/* apped new interface descriptors to the existing config if desired */
status_t usb_append_interface_highspeed(const uint8_t *int_descr, size_t len);
status_t usb_append_interface_lowspeed(const uint8_t *int_descr, size_t len);

status_t usb_add_string(const char *string, uint8_t id);

status_t usb_start(void);
status_t usb_stop(void);

/* callbacks from usbc and usb layers */
typedef enum {
    USB_CB_RESET,
    USB_CB_SUSPEND,
    USB_CB_RESUME,
    USB_CB_DISCONNECT,
    USB_CB_ONLINE,
    USB_CB_OFFLINE,
    USB_CB_SETUP_MSG,
} usb_callback_op_t;

/* setup arg is valid during CB_SETUP_MSG */
union usb_callback_args {
    const struct usb_setup *setup;
};

typedef status_t (*usb_callback_t)(void *cookie, usb_callback_op_t op, const union usb_callback_args *args);

/* callback api the usbc driver uses */
status_t usbc_callback(usb_callback_op_t op, const union usb_callback_args *args);

/* callback api that anyone can register for */
status_t usb_register_callback(usb_callback_t, void *cookie);

__END_CDECLS

#endif

