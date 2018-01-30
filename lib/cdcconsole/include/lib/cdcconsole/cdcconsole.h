/*
 * Copyright (c) 2018 The Fuchsia Authors
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
