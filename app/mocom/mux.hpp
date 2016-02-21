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
#include <list.h>
#include <lib/cpputils/nocopy.hpp>

namespace mocom {

class transport;
class channel;

class mux : lk::nocopy {
public:
    mux(transport &transport) : m_transport(transport) {}
    ~mux() {}

    void init();
    void set_online(bool online);
    void process_rx_packet(const uint8_t *buf, size_t len);
    ssize_t prepare_tx_packet(uint8_t *buf, size_t len);

private:
    // handle to transport
    transport &m_transport;

    channel *find_channel(uint32_t num);
    bool add_channel(channel *c);
    void remove_channel(channel *c);

    struct list_node m_channel_list = LIST_INITIAL_VALUE(m_channel_list);

    friend class control_channel;
};

} // namespace mocom

