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

#include <lib/cdcconsole/cdcconsole.h>

#include <compiler.h>
#include <err.h>
#include <lib/cbuf.h>
#include <string.h>

#if !CONSOLE_HAS_INPUT_BUFFER
#error "CONSOLE_HAS_INPUT_BUFFER needs to be configured for cdcconsole."
#endif

/*
 * This implementation is pretty simple right now.  Possible future work:
 *  * Make cdcconsole_print() asynchronous.
 *  * Buffer output when offline so that you can see boot messages.
 *  * Buffer writes with a timeout allowing the driver to coalesce them into
 *    less packets.
 */

static status_t cdcconsole_rx_cb(ep_t endpoint, usbc_transfer_t *t);

static void cdcconsole_print(print_callback_t *cb, const char *str, size_t len) {
    cdcconsole_t *con = cb->context;
    if (con->online) {
        cdcserial_write(&con->cdc_chan, len, (uint8_t *)str);
    }
}

static void cdcconsole_queue_read(cdcconsole_t *con) {
    cdcserial_read_async(&con->cdc_chan, &con->rx_transfer, cdcconsole_rx_cb,
                         sizeof(con->rx_buf), con->rx_buf);
}

static status_t cdcconsole_rx_cb(ep_t endpoint, usbc_transfer_t *t) {
    cdcconsole_t *con = containerof(t, cdcconsole_t, rx_transfer);

    cbuf_write(&console_input_cbuf, t->buf, t->bufpos, false);
    cdcconsole_queue_read(con);
    return NO_ERROR;
}

static void cdcconsole_online(cdcserial_channel_t *chan, bool online) {
    cdcconsole_t *con = containerof(chan, cdcconsole_t, cdc_chan);
    con->online = online;
    if (online) {
        cdcconsole_queue_read(con);
    }
}

void cdcconsole_init(cdcconsole_t *con, int data_ep_addr, int ctrl_ep_addr) {
    memset(con, 0x0, sizeof(*con));

    con->cdc_chan.online_cb = cdcconsole_online;
    cdcserial_create_channel(&con->cdc_chan, data_ep_addr, ctrl_ep_addr);

    con->print_cb.print = cdcconsole_print;
    con->print_cb.context = con;
    register_print_callback(&con->print_cb);
}
