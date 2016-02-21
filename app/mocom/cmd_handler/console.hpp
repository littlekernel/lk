/*
 * Copyright (c) 2015 Travis Geiselbrecht
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

#include <stdint.h>
#include <sys/types.h>
#include <debug.h>
#include <lib/cbuf.h>
#include <lib/cpputils/nocopy.hpp>

#include "handler.hpp"
#include "../mocom_app.hpp"

namespace mocom {
namespace cmd_handler {

class console : public handler {
public:
    console(command_channel &c);
    virtual ~console();

    virtual status_t Init() override;
    virtual void process_rx_packet(const uint8_t *buf, size_t len) override;
    virtual void tx_complete() override;

private:
    // main worker callback
    lk_time_t work();
    static lk_time_t _work(void *context) { return ((console *)context)->work(); }

    mocom_app::worker_callback m_worker;

    // debug console callback
    void debug_console_callback(print_callback_t *cb, const char *str, size_t len);
    static void _debug_console_callback(print_callback_t *cb, const char *str, size_t len) {
        ((console *)cb->context)->debug_console_callback(cb, str, len);
    }

    bool m_registered = false;
    print_callback_t m_cb;

    // circular buffer to hold outgoing print data
    cbuf_t m_outbuf;
    static const size_t OUTBUF_LEN = 1024;
    char   m_outbuf_buf[OUTBUF_LEN];

    // state of tx

};


}
}


