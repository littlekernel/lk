/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* Tests for the console output path in lib/io.
 *
 * The property under test is that a line of console output reaches the sink as
 * a single unit, even though the printf engine (and putchar, and puts) hand the
 * io layer many small writes, and even though several cpus may be printing at
 * once.
 *
 * These tests observe the output path by registering a print callback, which is
 * invoked from inside the console's print lock with exactly the runs of
 * characters the sink was handed. The callback therefore may not print, or do
 * anything else that could block or recurse; it only sets flags and counters.
 */

#include <lib/io.h>
#include <lib/unittest.h>

#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <stdio.h>
#include <string.h>

/* Lines emitted by these tests look like:
 *     "iotst:" <body_char repeated IO_TEST_BODY_LEN times> "\n"
 * The marker lets the callback ignore all the other console traffic going on
 * around the test, including the unit test framework's own output.
 */
#define IO_TEST_MARKER "iotst:"
#define IO_TEST_MARKER_LEN (sizeof(IO_TEST_MARKER) - 1)
#define IO_TEST_BODY_LEN 64
#define IO_TEST_LINE_LEN (IO_TEST_MARKER_LEN + IO_TEST_BODY_LEN + 1)

/* Keep the whole line comfortably inside a line buffer, so that a correctly
 * assembled line is never split by the buffer filling up.
 */
STATIC_ASSERT(THREAD_LINEBUFFER_SIZE == 0 || IO_TEST_LINE_LEN <= THREAD_LINEBUFFER_SIZE);

#define IO_TEST_MAX_THREADS 8

struct io_test_state {
    volatile bool active;

    /* Chunk level accounting: what the sink was handed in one call. Only
     * meaningful without the output ring, which re-chunks by design.
     */
    volatile uint marked_chunks;
    volatile uint malformed_chunks;
    volatile uint mixed_chunks;
    volatile uint misaligned_chunks;

    /* Line level accounting: the callback stream reassembled into lines. This
     * is the user visible property (what actually lands on the wire) and holds
     * in every configuration.
     */
    char linebuf[256];
    size_t linepos;
    volatile uint marked_lines;
    volatile uint bad_lines;

    /* every write handed to this sink, marker or not */
    volatile uint chunks;
};

static struct io_test_state test_state;

/* Is c one of the body characters the test writers use? */
static inline bool is_body_char(char c) {
    return c >= 'a' && c < (char)('a' + IO_TEST_MAX_THREADS);
}

/* Check one reassembled line. Returns false if it is one of ours and malformed. */
static bool line_is_well_formed(const char *line, size_t len) {
    if (len != IO_TEST_MARKER_LEN + IO_TEST_BODY_LEN) {
        return false;
    }
    char expected = line[IO_TEST_MARKER_LEN];
    if (!is_body_char(expected)) {
        return false;
    }
    for (size_t i = IO_TEST_MARKER_LEN; i < len; i++) {
        if (line[i] != expected) {
            return false;
        }
    }
    return true;
}

/* Reassemble the callback byte stream into lines and check the ones that are
 * ours. Unlike the chunk checks this holds however the sink chose to slice the
 * stream, so it is valid with the output ring enabled too.
 */
static void io_test_check_lines(struct io_test_state *s, const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c != '\n') {
            if (s->linepos < sizeof(s->linebuf)) {
                s->linebuf[s->linepos++] = c;
            }
            continue;
        }

        /* end of a line: is it one of ours? */
        size_t pos = s->linepos;
        s->linepos = 0;

        if (pos < IO_TEST_MARKER_LEN) {
            continue;
        }
        /* our lines may be preceded by unrelated output that had no newline
         * (the test framework's "[RUNNING]"), so search rather than anchor
         */
        for (size_t j = 0; j + IO_TEST_MARKER_LEN <= pos; j++) {
            if (memcmp(s->linebuf + j, IO_TEST_MARKER, IO_TEST_MARKER_LEN) == 0) {
                s->marked_lines++;
                if (!line_is_well_formed(s->linebuf + j, pos - j)) {
                    s->bad_lines++;
                }
                break;
            }
        }
    }
}

/* Account for one run of characters handed to a sink. Must not print: on the
 * console path this runs inside the print lock with interrupts disabled.
 */
static void io_test_analyze(struct io_test_state *s, const char *str, size_t len) {
    if (!s->active) {
        return;
    }

    s->chunks++;

    io_test_check_lines(s, str, len);

    /* Find our marker. Anything without it is unrelated console traffic. */
    size_t marker_pos = len;
    for (size_t i = 0; i + IO_TEST_MARKER_LEN <= len; i++) {
        if (memcmp(str + i, IO_TEST_MARKER, IO_TEST_MARKER_LEN) == 0) {
            marker_pos = i;
            break;
        }
    }
    if (marker_pos == len) {
        return;
    }

    s->marked_chunks++;

    /* The marker should start the chunk, and the chunk should be exactly the
     * one line. Anything else means the line was split or spliced.
     */
    if (marker_pos != 0 || len != IO_TEST_LINE_LEN) {
        s->misaligned_chunks++;
        return;
    }

    if (str[len - 1] != '\n') {
        s->malformed_chunks++;
        return;
    }

    /* Every body character must belong to the same writer. */
    char expected = str[IO_TEST_MARKER_LEN];
    if (!is_body_char(expected)) {
        s->malformed_chunks++;
        return;
    }
    for (size_t i = IO_TEST_MARKER_LEN; i < len - 1; i++) {
        if (str[i] != expected) {
            s->mixed_chunks++;
            return;
        }
    }
}

static void io_test_print_callback(print_callback_t *cb, const char *str, size_t len) {
    io_test_analyze((struct io_test_state *)cb->context, str, len);
}

static print_callback_t io_test_cb = {
    .print = io_test_print_callback,
    .context = &test_state,
};

/* A standalone line buffered handle, standing in for a redirected destination
 * such as a telnet session. Writes land here instead of on the console, which
 * lets us check that redirected output gets the same line assembly.
 */
static struct io_test_state redirect_state;

/* The console path serializes callbacks under the print lock; this stand-in
 * handle has to do its own, since several writer threads share it and the
 * analyzer keeps state (a partial line, plus counters) across calls.
 */
static spin_lock_t io_test_redirect_lock = 0;

static ssize_t io_test_redirect_write(io_handle_t *io, const char *buf, size_t len) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&io_test_redirect_lock);
    io_test_analyze(&redirect_state, buf, len);
    spin_unlock_irqrestore(&io_test_redirect_lock, state);
    return len;
}

static const io_handle_hooks_t io_test_redirect_hooks = {
    .write = io_test_redirect_write,
};

static io_handle_t io_test_redirect_io =
    IO_HANDLE_INITIAL_VALUE_ETC(&io_test_redirect_hooks, IO_HANDLE_FLAG_LINE_BUFFERED);

static FILE io_test_redirect_file = { .io = &io_test_redirect_io };

/* Emit one test line the hard way: a separate write per character, which is
 * what the printf engine, putchar and puts all end up doing.
 */
static void emit_test_line(FILE *fp, char body_char) {
    fputs(IO_TEST_MARKER, fp);
    for (uint i = 0; i < IO_TEST_BODY_LEN; i++) {
        fputc(body_char, fp);
    }
    fputc('\n', fp);
}

static void io_test_begin(void) {
    /* Start from an empty line buffer. The unit test framework leaves a partial
     * line behind ("[RUNNING]" has no newline), and our first line would
     * otherwise be appended to it and legitimately split when the buffer fills.
     */
    fflush(console_stdout);

    memset((void *)&test_state, 0, sizeof(test_state));
    register_print_callback(&io_test_cb);
    test_state.active = true;
}

static void io_test_end(void) {
    test_state.active = false;
    unregister_print_callback(&io_test_cb);
}

/* A single thread emitting a line one character at a time should reach the sink
 * as exactly one write.
 */
static bool linebuffer_assembles_lines(void) {
    BEGIN_TEST;

    if (THREAD_LINEBUFFER_SIZE == 0) {
        unittest_printf("[skipped: THREAD_LINEBUFFER_SIZE == 0] ");
        END_TEST;
    }
    if (CONSOLE_RING_SIZE > 0) {
        /* the ring re-slices the stream on its way to the drain thread, so
         * write granularity is not observable from a print callback
         */
        unittest_printf("[skipped: CONSOLE_RING_SIZE > 0] ");
        END_TEST;
    }

    const uint lines = 32;

    io_test_begin();
    for (uint i = 0; i < lines; i++) {
        emit_test_line(console_stdout, 'a');
    }
    io_test_end();

    EXPECT_EQ(lines, test_state.marked_chunks,
              "each line should reach the sink as exactly one write");
    EXPECT_EQ(0u, test_state.misaligned_chunks, "lines were split or spliced");
    EXPECT_EQ(0u, test_state.malformed_chunks, "lines were malformed");
    EXPECT_EQ(0u, test_state.mixed_chunks, "lines mixed writers");

    END_TEST;
}

struct io_test_writer_args {
    FILE *out;
    char body_char;
    uint lines;
};

static int io_test_writer(void *raw_args) {
    struct io_test_writer_args *args = (struct io_test_writer_args *)raw_args;

    for (uint i = 0; i < args->lines; i++) {
        emit_test_line(args->out, args->body_char);
    }

    return 0;
}

/* Several threads printing at once must not interleave within a line. */
static bool concurrent_lines_do_not_interleave(void) {
    BEGIN_TEST;

    if (THREAD_LINEBUFFER_SIZE == 0) {
        unittest_printf("[skipped: THREAD_LINEBUFFER_SIZE == 0] ");
        END_TEST;
    }

    uint nthreads = SMP_MAX_CPUS;
    if (nthreads < 2) {
        nthreads = 2;
    }
    if (nthreads > IO_TEST_MAX_THREADS) {
        nthreads = IO_TEST_MAX_THREADS;
    }

    const uint lines_per_thread = 32;

    struct io_test_writer_args args[IO_TEST_MAX_THREADS];
    thread_t *threads[IO_TEST_MAX_THREADS];

    for (uint i = 0; i < nthreads; i++) {
        args[i].out = console_stdout;
        args[i].body_char = 'a' + i;
        args[i].lines = lines_per_thread;
        threads[i] = thread_create("io_test_writer", io_test_writer, &args[i],
                                   DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[i], "failed to create writer thread");
        /* deliberately left unpinned: the scheduler spreads them over whatever
         * cpus actually exist, and on a single cpu they still race via
         * preemption in the middle of a line
         */
    }

    io_test_begin();

    for (uint i = 0; i < nthreads; i++) {
        thread_resume(threads[i]);
    }
    for (uint i = 0; i < nthreads; i++) {
        thread_join(threads[i], NULL, INFINITE_TIME);
    }

    /* let the output ring drain, if there is one, before reading the counters */
    fflush(console_stdout);
    if (CONSOLE_RING_SIZE > 0) {
        thread_sleep(250);
    }

    io_test_end();

    /* The line level check is the user visible property and holds in every
     * configuration: whatever reaches the console, no line ever mixes writers.
     */
    EXPECT_EQ(nthreads * lines_per_thread, test_state.marked_lines,
              "every line written should appear exactly once");
    EXPECT_EQ(0u, test_state.bad_lines,
              "a line was split, spliced, or mixed two cpus' output");

    if (CONSOLE_RING_SIZE == 0) {
        /* without the ring the sink is handed one whole line per write, and we
         * can check that directly too
         */
        EXPECT_EQ(nthreads * lines_per_thread, test_state.marked_chunks,
                  "every line should reach the sink as exactly one write");
        EXPECT_EQ(0u, test_state.misaligned_chunks, "lines were split or spliced");
        EXPECT_EQ(0u, test_state.malformed_chunks, "lines were malformed");
        EXPECT_EQ(0u, test_state.mixed_chunks, "lines from different cpus interleaved");
    }

    END_TEST;
}

/* fflush() must push out a partial line that has no newline yet. */
static bool fflush_pushes_partial_line(void) {
    BEGIN_TEST;

    if (THREAD_LINEBUFFER_SIZE == 0) {
        unittest_printf("[skipped: THREAD_LINEBUFFER_SIZE == 0] ");
        END_TEST;
    }
    if (CONSOLE_RING_SIZE > 0) {
        /* the callback fires asynchronously on the drain thread, so counting
         * it immediately after the flush would race
         */
        unittest_printf("[skipped: CONSOLE_RING_SIZE > 0] ");
        END_TEST;
    }

    io_test_begin();

    /* a line with no terminating newline should not reach the sink on its own */
    fputs(IO_TEST_MARKER, console_stdout);
    for (uint i = 0; i < IO_TEST_BODY_LEN; i++) {
        fputc('a', console_stdout);
    }
    uint before_flush = test_state.marked_chunks;

    fflush(console_stdout);
    uint after_flush = test_state.marked_chunks;

    /* put the newline back so the console isn't left mid line */
    fputc('\n', console_stdout);

    io_test_end();

    EXPECT_EQ(0u, before_flush, "a partial line should stay buffered");
    EXPECT_EQ(1u, after_flush, "fflush should push the partial line out");

    END_TEST;
}

/* Output redirected away from the console -- a telnet session, say -- must get
 * the same per thread line assembly, or several threads sharing one session
 * interleave exactly the way console output used to.
 */
static bool redirected_handle_is_line_buffered(void) {
    BEGIN_TEST;

    if (THREAD_LINEBUFFER_SIZE == 0) {
        unittest_printf("[skipped: THREAD_LINEBUFFER_SIZE == 0] ");
        END_TEST;
    }

    uint nthreads = SMP_MAX_CPUS;
    if (nthreads < 2) {
        nthreads = 2;
    }
    if (nthreads > IO_TEST_MAX_THREADS) {
        nthreads = IO_TEST_MAX_THREADS;
    }

    const uint lines_per_thread = 32;

    struct io_test_writer_args args[IO_TEST_MAX_THREADS];
    thread_t *threads[IO_TEST_MAX_THREADS];

    for (uint i = 0; i < nthreads; i++) {
        args[i].out = &io_test_redirect_file;
        args[i].body_char = 'a' + i;
        args[i].lines = lines_per_thread;
        threads[i] = thread_create("io_test_redirect", io_test_writer, &args[i],
                                   DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(threads[i], "failed to create writer thread");
    }

    memset((void *)&redirect_state, 0, sizeof(redirect_state));
    redirect_state.active = true;

    for (uint i = 0; i < nthreads; i++) {
        thread_resume(threads[i]);
    }
    for (uint i = 0; i < nthreads; i++) {
        thread_join(threads[i], NULL, INFINITE_TIME);
    }

    redirect_state.active = false;

    /* No console ring is involved here: the line buffer hands this handle its
     * writes directly, so we can check chunk granularity as well as lines.
     */
    EXPECT_EQ(nthreads * lines_per_thread, redirect_state.marked_lines,
              "every line written should appear exactly once");
    EXPECT_EQ(0u, redirect_state.bad_lines,
              "a line was split, spliced, or mixed two threads' output");
    EXPECT_EQ(nthreads * lines_per_thread, redirect_state.marked_chunks,
              "each line should reach the handle as exactly one write");
    EXPECT_EQ(0u, redirect_state.misaligned_chunks, "lines were split or spliced");
    EXPECT_EQ(0u, redirect_state.malformed_chunks, "lines were malformed");
    EXPECT_EQ(0u, redirect_state.mixed_chunks, "lines from different threads interleaved");

    END_TEST;
}

/* A partial line held for one handle must be pushed out when the same thread
 * starts writing to a different handle, or the two destinations' output would
 * be spliced together. This is what makes dprintf-to-console and
 * printf-to-session coexist on one thread.
 */
static bool switching_handles_flushes_partial_line(void) {
    BEGIN_TEST;

    if (THREAD_LINEBUFFER_SIZE == 0) {
        unittest_printf("[skipped: THREAD_LINEBUFFER_SIZE == 0] ");
        END_TEST;
    }

    /* start clean on both sides */
    fflush(console_stdout);
    io_flush_thread_linebuffer();

    memset((void *)&redirect_state, 0, sizeof(redirect_state));
    redirect_state.active = true;

    /* a line with no terminating newline: should sit in our line buffer */
    fputs(IO_TEST_MARKER, &io_test_redirect_file);
    for (uint i = 0; i < IO_TEST_BODY_LEN; i++) {
        fputc('a', &io_test_redirect_file);
    }
    uint before_switch = redirect_state.chunks;

    /* now write somewhere else entirely; the pending line must go out first */
    fputs("io_test: switching handles\n", console_stdout);
    uint after_switch = redirect_state.chunks;

    redirect_state.active = false;

    EXPECT_EQ(0u, before_switch, "a partial line should stay buffered");
    EXPECT_EQ(1u, after_switch,
              "switching handles should flush the previous handle's partial line");

    END_TEST;
}

BEGIN_TEST_CASE(io_tests)
RUN_TEST(linebuffer_assembles_lines)
RUN_TEST(concurrent_lines_do_not_interleave)
RUN_TEST(fflush_pushes_partial_line)
RUN_TEST(redirected_handle_is_line_buffered)
RUN_TEST(switching_handles_flushes_partial_line)
END_TEST_CASE(io_tests)
