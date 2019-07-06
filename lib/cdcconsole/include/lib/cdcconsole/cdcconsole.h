/*
 * Copyright (c) 2018 The Fuchsia Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <dev/usb/class/cdcserial.h>
#include <lib/cbuf.h>
#include <lib/io.h>

typedef struct {
    bool online;
    cdcserial_channel_t cdc_chan;

    print_callback_t print_cb;

    usbc_transfer_t rx_transfer;
    uint8_t rx_buf[64];

    spin_lock_t tx_lock;
    bool transmitting;
    cbuf_t tx_buf;
    usbc_transfer_t tx_transfer;
} cdcconsole_t;

void cdcconsole_init(cdcconsole_t *con, int data_ep_addr, int ctrl_ep_addr);
