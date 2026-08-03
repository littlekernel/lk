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
#include <arch/arch_ops.h>
#include <kernel/thread.h>
#include <string.h>

static ssize_t io_write_direct(io_handle_t *io, const char *buf, size_t len) {
    if (!io->hooks->write)
        return ERR_NOT_SUPPORTED;

    return io->hooks->write(io, buf, len);
}

#if THREAD_LINEBUFFER_SIZE > 0

/* May the calling context use the current thread's line buffer?
 *
 * arch_ints_disabled() is the portable superset of "in an interrupt handler or
 * holding a spinlock". Both bypass the line buffer, but for different reasons,
 * and both reasons matter:
 *
 *  - Interrupt handler: correctness. The handler runs on whatever thread it
 *    interrupted, so appending would splice its output into the middle of that
 *    thread's half assembled line.
 *
 *  - Spinlock held: promptness. The buffer really is ours here and appending
 *    would be safe, but anything that doesn't end in a newline would then sit
 *    in the buffer until this thread prints again or flushes. A context that
 *    cannot block is exactly the one that may be about to deadlock or panic,
 *    which is when you least want the diagnostic you just printed stranded in
 *    a buffer. Unbuffered and interleaved beats buffered and never seen.
 *
 * The cost is that output from these contexts is only atomic per write rather
 * than per line -- visible during secondary cpu bringup, where several cpus
 * print with interrupts still disabled and their messages interleave. That is
 * the accepted trade, not an oversight.
 *
 * NOTE therefore that refining this to a true "am I in an interrupt handler"
 * test (a per cpu nesting count, or a working arch_in_int_handler()) would NOT
 * be a straight improvement: it would start buffering in spinlock context and
 * silently give up the promptness property above.
 */
static thread_t *io_linebuffer_owner(void) {
    /* NOTE: this also covers "already holding a lock the sink needs", which
     * would otherwise self-deadlock on re-entry: the console's print lock is
     * only ever taken with spin_lock_irqsave, so holding it implies interrupts
     * are off. Do not be tempted to add an explicit spin_lock_held() check --
     * LK's arch_spin_lock_held() reports whether the lock is held by *any* cpu,
     * so under contention it would bypass the line buffer on cpus that don't
     * hold it and drop us back to per-character interleaved output.
     */
    if (unlikely(arch_ints_disabled()))
        return NULL;

    thread_t *t = get_current_thread();
    if (unlikely(!t || t->magic != THREAD_MAGIC))
        return NULL;

    /* look for corruption rather than running off the end of the buffer */
    if (unlikely(t->linebuffer_pos > sizeof(t->linebuffer))) {
        t->linebuffer_pos = 0;
        t->linebuffer_owner = NULL;
        return NULL;
    }

    return t;
}

static ssize_t io_linebuffer_flush(thread_t *t) {
    if (t->linebuffer_pos == 0)
        return 0;

    size_t len = t->linebuffer_pos;
    io_handle_t *owner = t->linebuffer_owner;

    /* clear before writing, so that anything the sink itself prints can't
     * recurse back into a buffer we're in the middle of draining
     */
    t->linebuffer_pos = 0;
    t->linebuffer_owner = NULL;

    if (!owner)
        return 0;

    return io_write_direct(owner, t->linebuffer, len);
}

/* Assemble output into the calling thread's line buffer, handing the sink a
 * whole line at a time.
 */
static ssize_t io_write_line_buffered(io_handle_t *io, const char *buf, size_t len) {
    thread_t *t = io_linebuffer_owner();
    if (unlikely(!t))
        return io_write_direct(io, buf, len);

    /* Switching destinations: get the previous handle's partial line out first,
     * or it would be appended to here and delivered to the wrong sink. See
     * IO_HANDLE_FLAG_LINE_BUFFERED in lib/io.h for when this arises.
     *
     * An error writing the *old* handle isn't this write's problem, so it is
     * deliberately not propagated here.
     */
    if (unlikely(t->linebuffer_owner && t->linebuffer_owner != io))
        (void)io_linebuffer_flush(t);

    t->linebuffer_owner = io;

    size_t remaining_in = len;
    while (remaining_in > 0) {
        size_t space = sizeof(t->linebuffer) - t->linebuffer_pos;
        size_t chunk = (remaining_in < space) ? remaining_in : space;
        const char *nl = memchr(buf, '\n', chunk);

        size_t size;
        bool inject_newline;
        bool flush;
        if (nl) {
            /* a newline that fits in what's left of the buffer */
            size = (size_t)(nl - buf) + 1;
            inject_newline = false;
            flush = true;
        } else if (chunk == space) {
            /* about to fill the buffer with no newline in sight, so break the
             * line here and inject one (leaving room for it)
             */
            size = space - 1;
            inject_newline = true;
            flush = true;
        } else {
            /* just accumulate */
            size = chunk;
            inject_newline = false;
            flush = false;
        }

        memcpy(t->linebuffer + t->linebuffer_pos, buf, size);
        t->linebuffer_pos += size;
        buf += size;
        remaining_in -= size;

        if (inject_newline)
            t->linebuffer[t->linebuffer_pos++] = '\n';

        if (flush) {
            io_handle_t *owner = t->linebuffer_owner;
            ssize_t err = io_linebuffer_flush(t);
            if (unlikely(err < 0)) {
                /* Surface the sink's error rather than reporting a successful
                 * write. Buffering necessarily defers errors -- a caller can
                 * only learn about them on the write or flush that actually
                 * reaches the sink -- but it must not swallow them.
                 */
                return err;
            }
            t->linebuffer_owner = owner;
        }
    }

    /* nothing left pending means nothing to attribute */
    if (t->linebuffer_pos == 0)
        t->linebuffer_owner = NULL;

    return len;
}

void io_flush_thread_linebuffer(void) {
    thread_t *t = io_linebuffer_owner();
    if (!t)
        return;

    io_linebuffer_flush(t);
}

size_t io_take_thread_linebuffer(const char **out) {
    thread_t *t = get_current_thread();
    if (unlikely(!t || t->magic != THREAD_MAGIC))
        return 0;
    if (unlikely(t->linebuffer_pos > sizeof(t->linebuffer))) {
        t->linebuffer_pos = 0;
        t->linebuffer_owner = NULL;
        return 0;
    }

    size_t len = t->linebuffer_pos;
    t->linebuffer_pos = 0;
    t->linebuffer_owner = NULL;

    if (len > 0)
        *out = t->linebuffer;

    return len;
}

#else // THREAD_LINEBUFFER_SIZE == 0

void io_flush_thread_linebuffer(void) {}

size_t io_take_thread_linebuffer(const char **out) {
    return 0;
}

#endif // THREAD_LINEBUFFER_SIZE > 0

ssize_t io_write(io_handle_t *io, const char *buf, size_t len) {
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

#if THREAD_LINEBUFFER_SIZE > 0
    if (io->flags & IO_HANDLE_FLAG_LINE_BUFFERED)
        return io_write_line_buffered(io, buf, len);
#endif

    return io_write_direct(io, buf, len);
}

ssize_t io_read(io_handle_t *io, char *buf, size_t len) {
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

    if (!io->hooks->read)
        return ERR_NOT_SUPPORTED;

    return io->hooks->read(io, buf, len);
}

status_t io_flush(io_handle_t *io) {
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

#if THREAD_LINEBUFFER_SIZE > 0
    /* push out the line we've assembled for this handle before asking the
     * handle to flush whatever it buffers downstream of us
     */
    if (io->flags & IO_HANDLE_FLAG_LINE_BUFFERED) {
        thread_t *t = io_linebuffer_owner();
        if (t && t->linebuffer_owner == io) {
            ssize_t err = io_linebuffer_flush(t);
            if (unlikely(err < 0))
                return (status_t)err;
        }
    }
#endif

    /* handles that buffer nothing are trivially flushed */
    if (!io->hooks->flush)
        return NO_ERROR;

    return io->hooks->flush(io);
}

