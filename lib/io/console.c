/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/io.h>

#include <lk/err.h>
#include <ctype.h>
#include <lk/debug.h>
#include <assert.h>
#include <lk/list.h>
#include <string.h>
#include <lib/cbuf.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>
#include <lk/init.h>
#include <stdio.h>
#include <stdlib.h>

/* routines for dealing with main console io */

static spin_lock_t print_spin_lock = 0;
static struct list_node print_callbacks = LIST_INITIAL_VALUE(print_callbacks);

#if CONSOLE_HAS_INPUT_BUFFER
#ifndef CONSOLE_BUF_LEN
#define CONSOLE_BUF_LEN 256
#endif

/* global input circular buffer */
cbuf_t console_input_cbuf;
static uint8_t console_cbuf_buf[CONSOLE_BUF_LEN];
#endif // CONSOLE_HAS_INPUT_BUFFER

/* Set once we're panicking or crashing. Deliberately not atomic: it is only
 * ever set to true, and a cpu that races and misses it simply takes the normal
 * path for one more write.
 */
static volatile bool console_panic_mode = false;

bool console_in_panic_mode(void) {
    return console_panic_mode;
}

static void console_write_out(const char *str, size_t len);
static void console_flush_linebuffer_panic(void);

bool console_set_panic_mode(bool panic) {
    bool old = console_panic_mode;
    console_panic_mode = panic;

    /* On the way into a panic, push out the partial line that led up to the
     * crash. The flag is already set, so this goes out the polled path and
     * takes no locks -- important, since we may have arrived here from an
     * exception with interrupts already disabled, or while another cpu holds
     * the print lock and is never coming back.
     */
    if (panic && !old) {
        console_flush_linebuffer_panic();
    }

    return old;
}

/* Synchronously push a run of characters at the registered callbacks and the
 * platform's debug port. This is the slow part: with a real uart it spins for
 * roughly a character time per byte.
 */
static void console_write_direct(const char *str, size_t len) {
#if CONSOLE_SERIALIZE_OUTPUT
    /* Hold the lock across both the callbacks and the platform write, so the
     * whole run of characters emerges atomically with respect to other cpus and
     * to interrupt handlers on this one.
     */
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&print_spin_lock);

    print_callback_t *cb;
    list_for_every_entry(&print_callbacks, cb, print_callback_t, entry) {
        if (cb->print)
            cb->print(cb, str, len);
    }

#if CONSOLE_OUTPUT_TO_PLATFORM_PUTC
    platform_dputs(str, len);
#endif

    spin_unlock_irqrestore(&print_spin_lock, state);
#else // !CONSOLE_SERIALIZE_OUTPUT
    /* Unserialized: output may interleave with other cpus and with interrupt
     * handlers, but no interrupts-off window is introduced.
     */
    if (!list_is_empty(&print_callbacks)) {
        arch_interrupt_saved_state_t state = spin_lock_irqsave(&print_spin_lock);

        print_callback_t *cb;
        list_for_every_entry(&print_callbacks, cb, print_callback_t, entry) {
            if (cb->print)
                cb->print(cb, str, len);
        }

        spin_unlock_irqrestore(&print_spin_lock, state);
    }

#if CONSOLE_OUTPUT_TO_PLATFORM_PUTC
    platform_dputs(str, len);
#endif
#endif // CONSOLE_SERIALIZE_OUTPUT
}

/* The bottom of the console output path. Everything printed to the console
 * funnels through here.
 */
static void console_write_out(const char *str, size_t len) {
    /* Panicking: take no locks, run no callbacks, use the polled output path.
     * Another cpu may be holding print_spin_lock and never coming back, and we
     * may have arrived here from inside a print callback.
     */
    if (unlikely(console_panic_mode)) {
        platform_pputs(str, len);
        return;
    }

    console_write_direct(str, len);
}

void register_print_callback(print_callback_t *cb) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&print_spin_lock);

    list_add_head(&print_callbacks, &cb->entry);

    spin_unlock_irqrestore(&print_spin_lock, state);
}

void unregister_print_callback(print_callback_t *cb) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&print_spin_lock);

    list_delete(&cb->entry);

    spin_unlock_irqrestore(&print_spin_lock, state);
}

/* Push out the partial line the calling thread has accumulated, on the way into
 * a panic. Goes out the polled path (panic mode is already set) rather than to
 * whichever handle the line was destined for, since that handle may be a
 * network session we have no business touching while crashing.
 */
static void console_flush_linebuffer_panic(void) {
    const char *pending = NULL;
    size_t len = io_take_thread_linebuffer(&pending);
    if (len > 0 && pending) {
        platform_pputs(pending, len);
    }
}

void console_flush_linebuffer(void) {
    io_flush_thread_linebuffer();
}

static ssize_t __debug_stdio_write(io_handle_t *io, const char *s, size_t len) {
    /* line assembly happens up in io_write(); by here we have a whole line */
    console_write_out(s, len);
    return len;
}

static status_t __debug_stdio_flush(io_handle_t *io) {
    console_flush_linebuffer();
    return NO_ERROR;
}

static ssize_t __debug_stdio_read(io_handle_t *io, char *s, size_t len) {
    if (len == 0)
        return 0;

#if CONSOLE_HAS_INPUT_BUFFER
    ssize_t err = cbuf_read(&console_input_cbuf, s, len, true);
    return err;
#else
    int err = platform_dgetc(s, true);
    if (err < 0)
        return err;

    return 1;
#endif
}

#if CONSOLE_HAS_INPUT_BUFFER
static void console_init_hook(uint level) {
    cbuf_initialize_etc(&console_input_cbuf, sizeof(console_cbuf_buf), console_cbuf_buf);
}

LK_INIT_HOOK(console, console_init_hook, LK_INIT_LEVEL_PLATFORM_EARLY - 1);
#endif

/* global console io handle */
static const io_handle_hooks_t console_io_hooks = {
    .write  = __debug_stdio_write,
    .read   = __debug_stdio_read,
    .flush  = __debug_stdio_flush,
};

io_handle_t console_io =
    IO_HANDLE_INITIAL_VALUE_ETC(&console_io_hooks, IO_HANDLE_FLAG_LINE_BUFFERED);

