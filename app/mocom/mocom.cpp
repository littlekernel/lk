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
#include "mocom_app.hpp"
#include <app/mocom.h>

#include <err.h>
#include <debug.h>
#include <stdio.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <app.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>

#include "transport.hpp"
#include "usb.hpp"
#include <lib/cpputils/autolocker.hpp>

#define LOCAL_TRACE 1

namespace mocom {

mocom_app *the_app;

mocom_app::mocom_app(transport &t)
:   m_transport(t),
    m_mux(m_transport)
{
}

mocom_app::~mocom_app()
{
}

status_t mocom_app::worker()
{
    for (;;) {
        lk_time_t t = INFINITE_TIME;

        // call all of the workers, returning the shortest time returned

        // let the transport layer do some work
        lk_time_t temp = m_transport.do_work();
        if (temp < t)
            t = temp;

        // call all of the registered workers
        worker_callback *cb;
        list_for_every_entry(&m_workers, cb, worker_callback, node) {
            temp = cb->work(cb->context);
            if (temp < t)
                t = temp;
        }

        // wait a cycle
        if (t > 0) {
            //LTRACEF("waiting %u msec\n", t);
            event_wait_timeout(&m_event, t);
        }
    }

    return NO_ERROR;
}

void mocom_app::register_worker(worker_callback &cb)
{
    list_add_head(&m_workers, &cb.node);
}

void mocom_app::unregister_worker(worker_callback &cb)
{
    list_delete(&cb.node);
}

status_t mocom_app::init()
{
    event_init(&m_event, false, EVENT_FLAG_AUTOUNSIGNAL);

    m_transport.set_mux(&m_mux);
    m_transport.init();
    m_mux.init();

    return NO_ERROR;
}

extern "C" void mocom_init(const struct app_descriptor *app)
{
}

extern "C" void mocom_entry(const struct app_descriptor *, void *)
{
    LTRACE_ENTRY;

    // construct a transport for us to talk over
#if WITH_DEV_USB
    transport *t = create_usb_transport();
#endif

    // construct the main mocom app
    mocom_app app(*t);

    // set a singleton pointer to it
    the_app = &app;

    app.init();

    // bump ourselves to high priority
    thread_set_priority(HIGH_PRIORITY);

    app.worker();
}

} // namespace mocom

