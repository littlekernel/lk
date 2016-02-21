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
#include "console.hpp"

#include <err.h>
#include <stdint.h>
#include <trace.h>
#include <assert.h>
#include <debug.h>

#include <../prot/packet_struct.h>
#include <../mocom_app.hpp>

#define LOCAL_TRACE 0

namespace mocom {
namespace cmd_handler {

console::console(command_channel &c)
:   handler(c)
{
}

console::~console()
{
    the_app->unregister_worker(m_worker);
    if (m_registered)
        unregister_print_callback(&m_cb);
}

void console::debug_console_callback(print_callback_t *cb, const char *str, size_t len)
{
    cbuf_write(&m_outbuf, str, len, false);

    the_app->signal_irq();
}

lk_time_t console::work()
{
    //LTRACE_ENTRY;

    // see if we need to output anything
    if (!is_tx_pending() && cbuf_space_used(&m_outbuf) > 0) {
        // grab a pointer directly into the cbuf and send it
        iovec_t iov[2];
        cbuf_peek(&m_outbuf, iov);

        DEBUG_ASSERT(iov[0].iov_len > 0);

        size_t _len = iov[0].iov_len;
        send_stdout_data((const char *)iov[0].iov_base, _len);

        // consume the data we sent from the cbuf
        cbuf_read(&m_outbuf, NULL, _len, false);

        // make sure the worker loops around immediately at least once to queue data
        return 0;
    } else {
        return INFINITE_TIME;
    }
}

void console::process_rx_packet(const uint8_t *buf, size_t len)
{
    LTRACEF("buf %p, len %zu\n", buf, len);

    const struct packet_header *header = (const struct packet_header *)buf;

    if (len < sizeof(struct packet_header))
        return; // XXX shoudl probably close the channel
    if (header->magic != PACKET_HEADER_MAGIC)
        return;
    if (header->version != PACKET_HEADER_VERSION)
        return;

    if (header->type == PACKET_HEADER_TYPE_DATA) {
        const uint8_t *data = (const uint8_t *)header->data;
        hexdump8_ex(data, header->size, 0);

        // feed it into the input queue (which doesn't exist right now)
    }
}

void console::tx_complete()
{
    LTRACE_ENTRY;

    handler::tx_complete();

    if (cbuf_space_used(&m_outbuf) > 0) {
        the_app->signal();
    }
}

status_t console::Init()
{
    LTRACE_ENTRY;

    // initialize the outgoing circular buffer
    cbuf_initialize_etc(&m_outbuf, OUTBUF_LEN, &m_outbuf_buf);

    // register a worker callback from the main mocom app
    m_worker.context = this;
    m_worker.work = &_work;
    the_app->register_worker(m_worker);

    // register for a debug console handler
    m_cb.context = this;
    m_cb.print = &_debug_console_callback;
    register_print_callback(&m_cb);
    m_registered = true;

    return NO_ERROR;
}

}
}
