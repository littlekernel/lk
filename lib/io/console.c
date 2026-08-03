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
#include <kernel/event.h>
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
#if CONSOLE_RING_SIZE > 0
static void console_ring_drain_panic(void);
#endif

bool console_set_panic_mode(bool panic) {
    bool old = console_panic_mode;
    console_panic_mode = panic;

    /* On the way into a panic, push out everything still in flight: first
     * whatever is queued in the ring, then the partial line that led up to the
     * crash. The flag is already set, so these go out the polled path and take
     * no locks -- important, since we may have arrived here from an exception
     * with interrupts already disabled, or while another cpu holds the print
     * lock and is never coming back.
     */
    if (panic && !old) {
#if CONSOLE_RING_SIZE > 0
        console_ring_drain_panic();
#endif
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

#if CONSOLE_RING_SIZE > 0

STATIC_ASSERT((CONSOLE_RING_SIZE & (CONSOLE_RING_SIZE - 1)) == 0); // power of two
#define CONSOLE_RING_MASK (CONSOLE_RING_SIZE - 1)

/* How much the drain thread pulls out of the ring at a time. */
#define CONSOLE_DRAIN_CHUNK 128

/* Backstop poll interval for output queued in a context that couldn't wake us. */
#define CONSOLE_DRAIN_POLL_MS 10

/* head/tail are monotonically increasing and masked on use, so "how full is it"
 * is a single subtraction and can never be confused with "empty".
 * All of these are guarded by print_spin_lock.
 */
static char console_ring[CONSOLE_RING_SIZE];
static size_t ring_head;
static size_t ring_tail;
static size_t ring_dropped;

static event_t ring_event = EVENT_INITIAL_VALUE(ring_event, false, EVENT_FLAG_AUTOUNSIGNAL);

/* The drain thread, once it exists. Also used to detect re-entry: if a print
 * callback prints while we're draining, that output has to go out directly
 * instead of being queued behind the drain we're already inside of.
 */
static thread_t *ring_drain_thread;

/* Poke the drain thread, if it is safe to do so from here.
 *
 * event_signal() acquires thread_lock, so calling it while already holding
 * thread_lock would deadlock on a non recursive spinlock -- and printing with
 * thread_lock held is entirely normal (dump_all_threads() does exactly that).
 * In that case we skip the wakeup; the drain thread waits with a timeout, so
 * anything queued this way still goes out promptly.
 *
 * From a plain interrupt handler this is fine: thread_lock isn't held, and
 * asking for no reschedule keeps it safe.
 */
static void console_ring_wake(void) {
    if (unlikely(thread_lock_held())) {
        return;
    }
    event_signal(&ring_event, false);
}

static void console_ring_enqueue(const char *str, size_t len) {
    /* Decide this *before* taking the lock: spin_lock_irqsave disables
     * interrupts, so arch_ints_disabled() inside the lock is always true and
     * would silently select the lossy path every time.
     */
    const bool can_wait = !arch_ints_disabled();

    /* A write has to land in the ring all at once. Committing it in pieces
     * would mean dropping the lock partway through, letting another cpu splice
     * its output into the middle of ours -- precisely the interleaving this
     * whole path exists to prevent.
     */
    if (unlikely(len > CONSOLE_RING_SIZE)) {
        /* Bigger than the ring, so it can never be queued atomically. Rare
         * enough (a single write larger than the whole buffer) to just take the
         * synchronous path.
         */
        console_write_direct(str, len);
        return;
    }

    for (;;) {
        arch_interrupt_saved_state_t state = spin_lock_irqsave(&print_spin_lock);

        size_t space = CONSOLE_RING_SIZE - (ring_head - ring_tail);
        if (space < len) {
            if (can_wait) {
                /* Thread context: wait for room rather than losing output, so
                 * that interactive use stays lossless. Drop the lock and
                 * restore interrupt state before yielding.
                 *
                 * NOTE: this relies on the drain thread outranking us, which is
                 * why it runs at HIGH_PRIORITY - 1 -- yielding from a thread of
                 * higher priority than the drain thread would never let it run.
                 * A real time or DPC priority thread that fills the ring on a
                 * uniprocessor can still spin here; do not lower the drain
                 * thread's priority.
                 */
                spin_unlock_irqrestore(&print_spin_lock, state);
                console_ring_wake();
                thread_yield();
                continue;
            }

            /* Interrupt context: we cannot block, so drop just enough of the
             * oldest output to fit this write whole, and account for it. The
             * drain thread reports the loss.
             */
            size_t drop = len - space;
            ring_tail += drop;
            ring_dropped += drop;
        }

        for (size_t i = 0; i < len; i++) {
            console_ring[(ring_head + i) & CONSOLE_RING_MASK] = str[i];
        }
        ring_head += len;

        spin_unlock_irqrestore(&print_spin_lock, state);
        break;
    }

    console_ring_wake();
}

/* Pull everything currently queued out to the real console. */
static void console_ring_drain(void) {
    for (;;) {
        char buf[CONSOLE_DRAIN_CHUNK];

        arch_interrupt_saved_state_t state = spin_lock_irqsave(&print_spin_lock);

        size_t dropped = ring_dropped;
        ring_dropped = 0;

        size_t avail = ring_head - ring_tail;
        size_t chunk = MIN(avail, sizeof(buf));
        for (size_t i = 0; i < chunk; i++) {
            buf[i] = console_ring[(ring_tail + i) & CONSOLE_RING_MASK];
        }
        ring_tail += chunk;

        spin_unlock_irqrestore(&print_spin_lock, state);

        if (unlikely(dropped > 0)) {
            char msg[64];
            int n = snprintf(msg, sizeof(msg), "\n[console: dropped %zu bytes]\n", dropped);
            if (n > 0) {
                console_write_direct(msg, MIN((size_t)n, sizeof(msg) - 1));
            }
        }

        if (chunk == 0) {
            return;
        }

        /* the slow part, with interrupts enabled */
        console_write_direct(buf, chunk);
    }
}

/* Get whatever is queued out before we fall back to the polled path.
 *
 * Uses a try lock: if another cpu holds the print lock we would rather lose the
 * backlog than deadlock, since at this point that cpu may be the one that
 * crashed while holding it.
 */
static void console_ring_drain_panic(void) {
    arch_interrupt_saved_state_t state = arch_interrupt_save();

    if (spin_trylock(&print_spin_lock) == 0) {
        while (ring_tail != ring_head) {
            char c = console_ring[ring_tail & CONSOLE_RING_MASK];
            ring_tail++;
            platform_pputs(&c, 1);
        }
        spin_unlock(&print_spin_lock);
    }

    arch_interrupt_restore(state);
}

static int console_drain_thread_entry(void *arg) {
    for (;;) {
        /* Timed rather than indefinite: a writer holding thread_lock cannot
         * signal us (see console_ring_wake()), so poll as a backstop to make
         * sure nothing sits in the ring indefinitely.
         */
        event_wait_timeout(&ring_event, CONSOLE_DRAIN_POLL_MS);
        console_ring_drain();
    }
    return 0;
}

static void console_ring_init_hook(uint level) {
    /* Must outrank ordinary writers: when the ring fills, a writer yields and
     * waits for room, which only makes progress if the drain thread can be
     * scheduled ahead of it. See console_ring_enqueue().
     */
    thread_t *t = thread_create("console_drain", console_drain_thread_entry, NULL,
                                HIGH_PRIORITY - 1, DEFAULT_STACK_SIZE);
    if (!t) {
        /* stay in synchronous mode; console_ring_active() keeps returning false */
        return;
    }

    /* published last: until this is set, output takes the direct path */
    ring_drain_thread = t;
    thread_detach_and_resume(t);
}

LK_INIT_HOOK(console_ring, console_ring_init_hook, LK_INIT_LEVEL_THREADING);

#endif // CONSOLE_RING_SIZE > 0

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

#if CONSOLE_RING_SIZE > 0
    /* Queue it, unless the ring isn't up yet (early boot) or we're the drain
     * thread itself, in which case queueing would just feed us our own tail.
     *
     * Note interrupt context goes through the ring too rather than around it:
     * bypassing would let interrupt output overtake queued output that
     * logically came first, which reads worse than the interleaving this whole
     * exercise is meant to fix.
     */
    if (likely(ring_drain_thread) && get_current_thread() != ring_drain_thread) {
        console_ring_enqueue(str, len);
        return;
    }
#endif

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

