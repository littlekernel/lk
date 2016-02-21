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

#include <list.h>
#include <sys/types.h>
#include <kernel/event.h>
#include <lib/cpputils/nocopy.hpp>
#include "transport.hpp"
#include "mux.hpp"

namespace mocom {

class mocom_app : lk::nocopy {
public:
    mocom_app(transport &);
    ~mocom_app();

    status_t init();

    status_t worker();

    // allow registering a worker routine to be run in the main thread context
    struct worker_callback {
        struct list_node node;
        void *context;
        lk_time_t (*work)(void *context);
    };
    void register_worker(worker_callback &cb);
    void unregister_worker(worker_callback &cb);

    // signal that the workers should cycle through
    void signal() { event_signal(&m_event, true); }
    void signal_irq() { event_signal(&m_event, false); }

private:
    // transport
    transport &m_transport;

    // mux layer
    mux m_mux;

    // main app event
    event_t m_event;

    struct list_node m_workers = LIST_INITIAL_VALUE(m_workers);
};

// the singleton app
extern mocom_app *the_app;

}
