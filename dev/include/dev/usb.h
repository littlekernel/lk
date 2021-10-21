/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

/* top level initialization for usb client, abstracts away the interfaces */
typedef struct {
    void *desc;
    size_t len;
    uint flags;
} usb_descriptor __ALIGNED(2);

#define USB_DESC_FLAG_STATIC (0x1)

#define USB_DESC_STATIC(x) { .desc = (void *)(x), .len = sizeof(x), .flags = USB_DESC_FLAG_STATIC }

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

/* Returns the Interface Number that will be assigned to the next interface that
   is registered using usb_append_interface_(.*) */
uint8_t usb_get_current_iface_num_highspeed(void);
uint8_t usb_get_current_iface_num_lowspeed(void);

/* Append new interface descriptors to the existing config.*/
/* Returns interface number selected or error. */
int usb_append_interface_highspeed(const uint8_t *int_descr, size_t len);
int usb_append_interface_lowspeed(const uint8_t *int_descr, size_t len);

/* add a new string to the existing config */
status_t usb_add_string(const char *string, uint8_t id);

status_t usb_start(void);
status_t usb_stop(void);

/* callback api that anyone can register for */
status_t usb_register_callback(usb_callback_t, void *cookie);

__END_CDECLS

