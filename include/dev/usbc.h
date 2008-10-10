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

#include <sys/types.h>
#include <debug.h>
#include <hw/usb.h>

void usbc_init(void);

typedef uint ep_t;

typedef enum {
	IN = 0,
	OUT
} ep_dir_t;

typedef enum {
	CB_RESET,
	CB_SUSPEND,
	CB_RESUME,
	CB_DISCONNECT,
	CB_ONLINE,
	CB_OFFLINE,
	CB_SETUP_MSG,
	
	/* endpoint transfer stuff */
	CB_EP_RXCOMPLETE,
	CB_EP_TXCOMPLETE,
	CB_EP_TRANSFER_CANCELLED,
} usbc_callback_op_t;

typedef struct {
	void *buf;
	size_t buflen;
	uint bufpos;
	int result;
	void *extra; // extra pointer to store whatever you want
} usbc_transfer;

enum {
	USB_TRANSFER_RESULT_OK = 0,
	USB_TRANSFER_RESULT_ERR = -1,
	USB_TRANSFER_RESULT_CANCELLED = -2,
};

typedef int (*ep_callback)(ep_t endpoint, usbc_callback_op_t op, usbc_transfer *transfer);

void usbc_setup_endpoint(ep_t ep, ep_dir_t dir, bool active, ep_callback callback, uint width, uint blocksize);
int usbc_queue_rx(ep_t ep, usbc_transfer *transfer);
int usbc_queue_tx(ep_t ep, usbc_transfer *transfer);

/* setup arg is valid during CB_SETUP_MSG */
union usb_callback_args {
	const struct usb_setup *setup;
};

typedef int (*usb_callback)(usbc_callback_op_t op, const union usb_callback_args *args);

int usbc_set_callback(usb_callback);
int usbc_set_active(bool active);

/* called back from within a callback to handle setup responses */
void usbc_ep0_ack(void);
void usbc_ep0_stall(void);
void usbc_ep0_send(const void *buf, size_t len, size_t maxlen);
void usbc_ep0_recv(void *buf, size_t len, ep_callback);

bool usbc_is_highspeed(void);

static inline void usbc_dump_transfer(const usbc_transfer *t)
{
	printf("usb transfer %p: buf %p, buflen %zd, bufpos %u, result %d\n", t, t->buf, t->buflen, t->bufpos, t->result);
}

#endif

