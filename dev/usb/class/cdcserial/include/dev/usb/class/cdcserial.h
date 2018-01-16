/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
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

#pragma once

#include <dev/usbc.h>
#include <kernel/event.h>
#include <sys/types.h>

typedef struct _cdcserial_channel_t cdcserial_channel_t;
typedef struct _cdcserial_channel_t {
    int data_ep_addr;
    int ctrl_ep_addr;

    event_t txevt;
    event_t rxevt;

    volatile bool usb_online;
    void (*online_cb)(cdcserial_channel_t *chan, bool online);

    // A bitfield corresponding to the registered endpoints. When we get a
    // USB_ONLINE event, these are the endpoints that we need to setup.
    volatile uint16_t registered_bulk_eps_in;
    volatile uint16_t registered_bulk_eps_out;
    volatile uint16_t registered_intr_eps_in;
    volatile uint16_t registered_intr_eps_out;
} cdcserial_channel_t;

void cdcserial_create_channel(cdcserial_channel_t *chan, int data_ep_addr, int ctrl_ep_addr);

// Write len bytes to the CDC Serial Virtual Com Port.
status_t cdcserial_write(cdcserial_channel_t *chan, size_t len, uint8_t *buf);
status_t cdcserial_write_async(cdcserial_channel_t *chan, usbc_transfer_t *transfer, ep_callback cb,
                               size_t len, uint8_t *buf);

// Read at most len bytes from the CDC Serial virtual Com Port. Returns the
// actual number of bytes read.
ssize_t cdcserial_read(cdcserial_channel_t *chan, size_t len, uint8_t *buf);
ssize_t cdcserial_read_async(cdcserial_channel_t *chan, usbc_transfer_t *transfer, ep_callback cb,
                             size_t len, uint8_t *buf);
