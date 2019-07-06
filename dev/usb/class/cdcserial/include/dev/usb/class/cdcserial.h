/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
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
