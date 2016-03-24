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
#ifndef __DEV_USBC_H
#define __DEV_USBC_H

#include <compiler.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <hw/usb.h>

__BEGIN_CDECLS

void usbc_init(void);

typedef uint ep_t;

typedef enum {
    USB_IN = 0,
    USB_OUT
} ep_dir_t;

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

status_t usbc_setup_endpoint(ep_t ep, ep_dir_t dir, uint width);
status_t usbc_queue_rx(ep_t ep, usbc_transfer_t *transfer);
status_t usbc_queue_tx(ep_t ep, usbc_transfer_t *transfer);

status_t usbc_set_active(bool active);
void usbc_set_address(uint8_t address);

/* called back from within a callback to handle setup responses */
void usbc_ep0_ack(void);
void usbc_ep0_stall(void);
void usbc_ep0_send(const void *buf, size_t len, size_t maxlen);
void usbc_ep0_recv(void *buf, size_t len, ep_callback);

bool usbc_is_highspeed(void);

static inline void usbc_dump_transfer(const usbc_transfer_t *t)
{
    printf("usb transfer %p: cb %p buf %p, buflen %zd, bufpos %u, result %d\n", t, t->callback, t->buf, t->buflen, t->bufpos, t->result);
}

__END_CDECLS

#endif

