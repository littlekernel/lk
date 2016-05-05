/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
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

#include <lib/ndebug/system/consoleproxy.h>

#include <kernel/thread.h>
#include <lib/cbuf.h>
#include <lib/io.h>
#include <lib/ndebug/shared_structs.h>
#include <lib/ndebug/system/mux.h>

#define CONSOLE_IO_DEADLINE_MS 500

void consoleproxy_print_callback(print_callback_t *cb,
                                 const char *str, size_t len)
{
    if (!ndebug_sys_connected()) return;

    size_t written = 0;
    while (written < len) {
        size_t chunk_len = MIN(len, MAX_MUX_PACKET_SIZE);

        ssize_t result = ndebug_write_sys(
                             (uint8_t *)(str + written),
                             chunk_len,
                             NDEBUG_SYS_CHANNEL_CONSOLE,
                             CONSOLE_IO_DEADLINE_MS
                         );

        if (result < 0) break;
        written += result;
    }
}

static print_callback_t cb = {
    .entry = { 0 },
    .print = consoleproxy_print_callback,
    .context = NULL
};

static int console_input_thread(void *arg)
{
    uint8_t *buf;
    while (true) {
        ssize_t bytes =
            ndebug_read_sys(&buf, NDEBUG_SYS_CHANNEL_CONSOLE, INFINITE_TIME);
        if (bytes <= 0) continue;
        // Put the buffer in the console input queue.
        cbuf_write(&console_input_cbuf, buf, bytes, true);
    }
    return 0;
}

void consoleproxy_init(void)
{
    thread_resume(
        thread_create("ndebug console input", &console_input_thread, NULL,
                      DEFAULT_PRIORITY, DEFAULT_STACK_SIZE)
    );
    register_print_callback(&cb);
}
