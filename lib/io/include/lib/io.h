/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
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

#include <compiler.h>
#include <list.h>
#include <sys/types.h>

/* LK specific calls to register to get input/output of the main console */

__BEGIN_CDECLS

typedef struct __print_callback print_callback_t;
struct __print_callback {
    struct list_node entry;
    void (*print)(print_callback_t *cb, const char *str, size_t len);
    void *context;
};

/* register callback to receive debug prints */
void register_print_callback(print_callback_t *cb);
void unregister_print_callback(print_callback_t *cb);

/* the underlying handle to talk to io devices */
struct io_handle;
typedef struct io_handle_hooks {
    ssize_t (*write)(struct io_handle *handle, const char *buf, size_t len);
    ssize_t (*read)(struct io_handle *handle, char *buf, size_t len);
} io_handle_hooks_t;

#define IO_HANDLE_MAGIC (0x696f6820)  // "ioh "

typedef struct io_handle {
    uint32_t magic;
    const io_handle_hooks_t *hooks;
} io_handle_t;

/* routines to call through the io handle */
ssize_t io_write(io_handle_t *io, const char *buf, size_t len);
ssize_t io_read(io_handle_t *io, char *buf, size_t len);

/* initialization routine */
#define IO_HANDLE_INITIAL_VALUE(_hooks) { .magic = IO_HANDLE_MAGIC, .hooks = _hooks }

static inline void io_handle_init(io_handle_t *io, io_handle_hooks_t *hooks)
{
    *io = (io_handle_t)IO_HANDLE_INITIAL_VALUE(hooks);
}

/* the main console io handle */
extern io_handle_t console_io;

#ifndef CONSOLE_HAS_INPUT_BUFFER
#define CONSOLE_HAS_INPUT_BUFFER 0
#endif

#if CONSOLE_HAS_INPUT_BUFFER
/* main input circular buffer that acts as the default input queue */
typedef struct cbuf cbuf_t;
extern cbuf_t console_input_cbuf;
#endif

__END_CDECLS
