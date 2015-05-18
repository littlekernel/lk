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
#include <trace.h>
#include <err.h>
#include <malloc.h>
#include <lk/init.h>
#include <arch/arm.h>
#include <arch/arm/dcc.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <platform.h>

struct dcc_state {
    dcc_rx_callback_t rx_callback;
    mutex_t  lock;
    thread_t *worker;
};

#define SLOW_POLL_RATE 100
#define FAST_POLL_TIMEOUT 5

static int dcc_worker_entry(void *arg)
{
    struct dcc_state *dcc = (struct dcc_state *)arg;
    lk_time_t fast_poll_start;
    bool fast_poll;

    fast_poll = false;
    for (;;) {
        // wait for a bit if we're in slow poll mode
        if (!fast_poll) {
            thread_sleep(SLOW_POLL_RATE);
        }

        if (arm_dcc_read_available()) {
            uint32_t val = arm_read_dbgdtrrxint();

            dcc->rx_callback(val);

            // we just received something, so go to a faster poll rate
            fast_poll = true;
            fast_poll_start = current_time();
        } else {
            // didn't see anything
            if (fast_poll && current_time() - fast_poll_start >= FAST_POLL_TIMEOUT) {
                fast_poll = false; // go back to slow poll
            }
        }
    }

    return 0;
}

status_t arm_dcc_enable(dcc_rx_callback_t rx_callback)
{
    struct dcc_state *state = malloc(sizeof(struct dcc_state));
    if (!state)
        return ERR_NO_MEMORY;

    state->rx_callback = rx_callback;
    mutex_init(&state->lock);

    state->worker = thread_create("dcc worker", dcc_worker_entry, state, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(state->worker);

    return NO_ERROR;
}

bool arm_dcc_read_available(void)
{
    uint32_t dscr = arm_read_dbgdscr();
    if (dscr & (1<<30)) { // rx full
        return true;
    } else {
        return false;
    }
}

ssize_t arm_dcc_read(uint32_t *buf, size_t len, lk_time_t timeout)
{
    lk_time_t start = 0;

    if (timeout != 0)
        start = current_time();

    ssize_t count = 0;
    while (count < (ssize_t)len) {

        uint32_t dscr = arm_read_dbgdscr();
        if (dscr & (1<<30)) { // rx full
            uint32_t val = arm_read_dbgdtrrxint();
            *buf++ = val;

            count++;
        } else {
            if (timeout == 0 || current_time() - start >= timeout) {
                break;
            }
        }
    }

    return count;
}

ssize_t arm_dcc_write(const uint32_t *buf, size_t len, lk_time_t timeout)
{
    lk_time_t start = 0;

    if (timeout != 0)
        start = current_time();

    ssize_t count = 0;
    while (count < (ssize_t)len) {

        uint32_t dscr = arm_read_dbgdscr();
        if ((dscr & (1<<29)) == 0) { // tx empty
            arm_write_dbgdtrrxint(*buf);
            count++;
            buf++;
        } else {
            if (timeout == 0 || current_time() - start >= timeout) {
                break;
            }
        }
    }

    return count;
}

#if WITH_LIB_CONSOLE
#include <lib/console.h>
#include <string.h>

static void dcc_rx_callback(uint32_t val)
{
    static int count = 0;
    count += 4;
    if ((count % 1000) == 0)
        printf("count %d\n", count);
}

static int cmd_dcc(int argc, const cmd_args *argv)
{
    static bool dcc_started = false;

    if (argc < 2) {
        printf("not enough args\n");
        return -1;
    }

    if (!strcmp(argv[1].str, "start")) {
        if (!dcc_started) {
            printf("starting dcc\n");

            status_t err = arm_dcc_enable(&dcc_rx_callback);
            printf("arm_dcc_enable returns %d\n", err);
            dcc_started = true;
        }
    } else if (!strcmp(argv[1].str, "write")) {
        for (int i = 2; i < argc; i++) {
            uint32_t buf[128];
            size_t len = strlen(argv[i].str);
            for (uint j = 0; j < len; j++) {
                buf[j] = argv[i].str[j];
            }
            arm_dcc_write(buf, strlen(argv[i].str), 1000);
        }
    } else if (!strcmp(argv[1].str, "read")) {
        uint32_t buf[128];

        ssize_t len = arm_dcc_read(buf, sizeof(buf), 1000);
        printf("arm_dcc_read returns %ld\n", len);
        if (len > 0) {
            hexdump(buf, len);
        }
    } else {
        printf("unknown args\n");
    }

    return 0;
}

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 1
    { "dcc", "dcc stuff", &cmd_dcc }
#endif
STATIC_COMMAND_END(dcc);

#endif

