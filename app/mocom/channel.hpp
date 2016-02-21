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

class mux;

class channel : lk::nocopy {
public:
    channel(mux &m, uint32_t num) : m_mux(m), m_num(num) {}
    virtual ~channel() {}

    virtual void process_rx_packet(const uint8_t *buf, size_t len) = 0;
    virtual status_t close();

    uint32_t get_num() const { return m_num; }

    virtual status_t queue_tx(const void *buf, size_t len);
    virtual void tx_complete() { m_tx_buf = nullptr; m_tx_len = 0; }

protected:
    mux &m_mux;

    uint32_t m_num;

    // rx stuff
    uint32_t m_rx_sequence = 0;
    uint32_t m_pending_ack_sequence = 0;
    bool     m_pending_ack = false;

    // tx stuff
    uint32_t m_tx_sequence = 0;
    uint32_t m_acked_tx_sequence = 0;
    const uint8_t *m_tx_buf = nullptr;
    size_t   m_tx_len = 0;

    // list of us in the mux class
    friend class mux;
    struct list_node m_list_node = LIST_INITIAL_CLEARED_VALUE;
};

class control_channel : public channel {
public:
    control_channel(mux &m, uint32_t num) : channel(m, num) {}
    virtual ~control_channel() {}

    virtual void process_rx_packet(const uint8_t *buf, size_t len) override;
};

namespace cmd_handler {
class handler;
}

class command_channel : public channel {
public:
    command_channel(mux &m, uint32_t num) : channel(m, num) {}
    virtual ~command_channel();

    virtual void process_rx_packet(const uint8_t *buf, size_t len) override;
    virtual void tx_complete() override;
    virtual status_t close() override;

private:
    enum {
        STATE_INITIAL,
        STATE_ESTABLISHED
    } m_state = STATE_INITIAL;

    // specialized handler to handle the command
    cmd_handler::handler *m_handler = nullptr;
};

} // namespace mocom

