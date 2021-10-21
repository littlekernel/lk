/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <hw/usb.h>
#include <dev/usb.h>

__BEGIN_CDECLS

void usbc_init(void);

typedef uint ep_t;

typedef enum {
    USB_IN = 0,
    USB_OUT
} ep_dir_t;

typedef enum {
    USB_CTRL = 0x00,
    USB_ISOC = 0x01,
    USB_BULK = 0x02,
    USB_INTR = 0x03,
} ep_type_t;


struct usbc_transfer;
typedef status_t (*ep_callback)(ep_t endpoint, struct usbc_transfer *transfer);

typedef struct usbc_transfer {
    ep_callback callback;
    status_t result;
    void *buf;
    size_t buflen;
    uint bufpos;
    void *extra; // extra pointer to store whatever you want
} usbc_transfer_t;

enum {
    USB_TRANSFER_RESULT_OK = 0,
    USB_TRANSFER_RESULT_ERR = -1,
    USB_TRANSFER_RESULT_CANCELLED = -2,
};

status_t usbc_setup_endpoint(ep_t ep, ep_dir_t dir, uint width, ep_type_t type);
status_t usbc_queue_rx(ep_t ep, usbc_transfer_t *transfer);
status_t usbc_queue_tx(ep_t ep, usbc_transfer_t *transfer);
status_t usbc_flush_ep(ep_t ep);

status_t usbc_set_active(bool active);
void usbc_set_address(uint8_t address);

/* callback api the usbc driver uses */
status_t usbc_callback(usb_callback_op_t op, const union usb_callback_args *args);

/* called back from within a callback to handle setup responses */
void usbc_ep0_ack(void);
void usbc_ep0_stall(void);
void usbc_ep0_send(const void *buf, size_t len, size_t maxlen);
void usbc_ep0_recv(void *buf, size_t len, ep_callback);

bool usbc_is_highspeed(void);

static inline void usbc_dump_transfer(const usbc_transfer_t *t) {
    printf("usb transfer %p: cb %p buf %p, buflen %zd, bufpos %u, result %d\n", t, t->callback, t->buf, t->buflen, t->bufpos, t->result);
}

__END_CDECLS

