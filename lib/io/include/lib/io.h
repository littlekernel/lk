/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <lk/list.h>
#include <stdbool.h>
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
    /* optional: push any buffered output out. NULL means nothing is buffered. */
    status_t (*flush)(struct io_handle *handle);
} io_handle_hooks_t;

#define IO_HANDLE_MAGIC (0x696f6820)  // "ioh "

/* Assemble output into the writing thread's line buffer and hand this handle a
 * whole line at a time, instead of the many small writes the printf engine (and
 * putchar, and puts) actually produce. Without it, several threads writing to
 * the same handle interleave mid-line.
 *
 * Requires THREAD_LINEBUFFER_SIZE > 0; otherwise it is silently a no-op.
 *
 * The buffer itself lives in thread_t (linebuffer / linebuffer_pos /
 * linebuffer_owner) but is entirely owned by this library; the kernel only
 * provides the storage. Per thread rather than per handle because the point is
 * to keep concurrent writers apart: a buffer hanging off the handle would need
 * a lock and would put every writer back on one shared buffer, which is the
 * interleaving we are trying to avoid.
 *
 * Because the buffer is per thread and the destination is not, each thread
 * tags its pending bytes with the handle they are headed for
 * (linebuffer_owner), and a write to a *different* handle flushes what's
 * pending first -- otherwise a partial line held for one handle would have the
 * next handle's output appended to it and the whole spliced line delivered to
 * whichever of the two happened to complete it. When only one line buffered
 * handle exists the switch never fires, at a cost of one pointer per thread
 * and one well predicted compare per write.
 *
 * Contexts that cannot own the buffer (interrupt handlers, and anything holding
 * a spinlock -- see io_linebuffer_owner() in io.c) bypass it and write straight
 * through, so their output stays atomic per write but not per line.
 */
#define IO_HANDLE_FLAG_LINE_BUFFERED (1u << 0)

typedef struct io_handle {
    uint32_t magic;
    uint32_t flags;
    const io_handle_hooks_t *hooks;
} io_handle_t;

/* routines to call through the io handle */
ssize_t io_write(io_handle_t *io, const char *buf, size_t len);
ssize_t io_read(io_handle_t *io, char *buf, size_t len);
status_t io_flush(io_handle_t *io);

/* initialization routine */
#define IO_HANDLE_INITIAL_VALUE_ETC(_hooks, _flags) \
    { .magic = IO_HANDLE_MAGIC, .flags = (_flags), .hooks = _hooks }
#define IO_HANDLE_INITIAL_VALUE(_hooks) IO_HANDLE_INITIAL_VALUE_ETC(_hooks, 0)

static inline void io_handle_init_etc(io_handle_t *io, const io_handle_hooks_t *hooks,
                                      uint32_t flags) {
    *io = (io_handle_t)IO_HANDLE_INITIAL_VALUE_ETC(hooks, flags);
}

static inline void io_handle_init(io_handle_t *io, const io_handle_hooks_t *hooks) {
    io_handle_init_etc(io, hooks, 0);
}

/* Push out any partial line the calling thread has accumulated, to whichever
 * handle it belongs to. No-op if there is none, if line buffering is compiled
 * out, or if called from a context that doesn't own the buffer (interrupt or
 * spinlock context).
 */
void io_flush_thread_linebuffer(void);

/* Hand back the calling thread's pending partial line and clear it, without
 * writing it anywhere. Used by the panic path, which wants to emit it through
 * the polled console rather than through whatever handle it was destined for
 * (that handle may be a network session we cannot touch while crashing).
 *
 * Returns the length, and sets *out when non-zero. Unlike the above this does
 * not care about the calling context: while panicking, a partial line from the
 * interrupted thread is exactly the context worth having.
 */
size_t io_take_thread_linebuffer(const char **out);

/* the main console io handle */
extern io_handle_t console_io;

/* Switch the console into panic mode: output bypasses all locks, all registered
 * print callbacks and any buffering, and goes straight out the platform's
 * unbuffered (polled) debug port. Called from panic()/assert paths so that
 * crashing while another cpu holds the print lock, or while inside a print
 * callback, still produces output instead of hanging.
 *
 * Returns the previous state.
 */
bool console_set_panic_mode(bool panic);
bool console_in_panic_mode(void);

/* Push out any partial line the calling thread has accumulated in its console
 * line buffer. No-op if the line buffer is compiled out, if there is no partial
 * line, or if called from a context that doesn't own the buffer (interrupt or
 * spinlock context).
 */
void console_flush_linebuffer(void);

/* should the console also output to the platform's putc (usually UART) */
#ifndef CONSOLE_OUTPUT_TO_PLATFORM_PUTC
#define CONSOLE_OUTPUT_TO_PLATFORM_PUTC 1
#endif

/* Serialize console output with a spinlock, so that a single write emerges
 * atomically with respect to other cpus and to interrupt handlers. Costs an
 * interrupts-off window for the duration of the write, which on a slow uart is
 * milliseconds; set to 0 on latency sensitive targets to get the historical
 * unserialized behavior back.
 */
#ifndef CONSOLE_SERIALIZE_OUTPUT
#define CONSOLE_SERIALIZE_OUTPUT 1
#endif

#ifndef CONSOLE_HAS_INPUT_BUFFER
#define CONSOLE_HAS_INPUT_BUFFER 0
#endif

#if CONSOLE_HAS_INPUT_BUFFER
/* main input circular buffer that acts as the default input queue */
typedef struct cbuf cbuf_t;
extern cbuf_t console_input_cbuf;
#endif

__END_CDECLS
