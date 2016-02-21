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
#include <assert.h>
#include <stdint.h>
#include <trace.h>
#include <string.h>
#include <debug.h>

#include <../prot/packet_struct.h>

#define LOCAL_TRACE 0

namespace mocom {
namespace cmd_handler {

handler::handler(command_channel &c)
:   m_cc(c)
{
}

handler::~handler()
{
    DEBUG_ASSERT(!m_outbuf);
}

status_t handler::send_stdout_data(const char *buf, size_t len)
{
    LTRACEF("buf %p, len %zu\n", buf, len);

    if (m_outbuf)
        return ERR_NOT_READY;

    /// XXX remove
    m_outbuf = new char[len + sizeof(struct packet_header)];
    if (!m_outbuf)
        return ERR_NO_MEMORY;

    struct packet_header *header = (struct packet_header *)m_outbuf;
    header->magic = PACKET_HEADER_MAGIC;
    header->version = PACKET_HEADER_VERSION;
    header->type = PACKET_HEADER_TYPE_DATA;
    header->size = len;

    memcpy(header->data, buf, len);

    return m_cc.queue_tx(m_outbuf, sizeof(struct packet_header) + len);
}

void handler::tx_complete()
{
    LTRACE_ENTRY;

    if (m_outbuf) {
        delete m_outbuf;
        m_outbuf = nullptr;
    }
}

void handler::close()
{
    LTRACE_ENTRY;

    tx_complete();
}

}
}
