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

/* routines for dealing with main console io */

#if WITH_LIB_SM
#define PRINT_LOCK_FLAGS SPIN_LOCK_FLAG_IRQ_FIQ
#else
#define PRINT_LOCK_FLAGS SPIN_LOCK_FLAG_INTERRUPTS
#endif

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

/* print lock must be held when invoking out, outs, outc */
static void out_count(const char *str, size_t len) {
    print_callback_t *cb;

    /* print to any registered loggers */
    if (!list_is_empty(&print_callbacks)) {
        spin_lock_saved_state_t state;
        spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);

        list_for_every_entry(&print_callbacks, cb, print_callback_t, entry) {
            if (cb->print)
                cb->print(cb, str, len);
        }

        spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
    }

#if CONSOLE_OUTPUT_TO_PLATFORM_PUTC
    size_t i;
    /* write out the serial port */
    for (i = 0; i < len; i++) {
        platform_dputc(str[i]);
    }
#endif
}

void register_print_callback(print_callback_t *cb) {
    spin_lock_saved_state_t state;
    spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);

    list_add_head(&print_callbacks, &cb->entry);

    spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
}

void unregister_print_callback(print_callback_t *cb) {
    spin_lock_saved_state_t state;
    spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);

    list_delete(&cb->entry);

    spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
}

static ssize_t __debug_stdio_write(io_handle_t *io, const char *s, size_t len) {
    out_count(s, len);
    return len;
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
};

io_handle_t console_io = IO_HANDLE_INITIAL_VALUE(&console_io_hooks);

