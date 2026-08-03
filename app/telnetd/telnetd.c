/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* A shell over telnet.
 *
 * Each accepted connection gets a session object wrapping the socket in an
 * io_handle, and a console_t bound to it. console_start() then binds the
 * session thread's stdin/stdout to the same place, so that commands -- which
 * print with plain printf -- land in the session rather than on the serial
 * console. Kernel diagnostics (dprintf, TRACEF, panics) deliberately do not
 * follow the binding and stay on the console.
 *
 * This is the reason WITH_THREAD_STDOUT exists; rules.mk turns it on.
 */

#include <app.h>
#include <assert.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <lib/console.h>
#include <lib/io.h>
#include <lib/minip.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>
#include <lk/trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

#ifndef TELNETD_PORT
#define TELNETD_PORT 23
#endif

/* Output is assembled here rather than issuing a tcp_write per character. The
 * console writes a character at a time when echoing, and printf hands the io
 * layer one write per format conversion.
 */
#ifndef TELNETD_TX_BUFSIZE
#define TELNETD_TX_BUFSIZE 256
#endif

#ifndef TELNETD_RX_BUFSIZE
#define TELNETD_RX_BUFSIZE 64
#endif

/* telnet protocol bits we care about (RFC 854/857/858) */
#define TELNET_SE   240
#define TELNET_SB   250
#define TELNET_WILL 251
#define TELNET_WONT 252
#define TELNET_DO   253
#define TELNET_DONT 254
#define TELNET_IAC  255

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SGA  3

/* Tell the client we will echo and will suppress go-ahead, which is the
 * standard way to ask for character-at-a-time mode.
 */
static const uint8_t telnet_server_options[] = {
    TELNET_IAC, TELNET_WILL, TELNET_OPT_ECHO,
    TELNET_IAC, TELNET_WILL, TELNET_OPT_SGA,
};

/* IAC parser states */
enum telnet_iac_state {
    IAC_NONE = 0,   /* ordinary data */
    IAC_SEEN,       /* saw IAC, next byte is a command */
    IAC_OPTION,     /* saw IAC <WILL|WONT|DO|DONT>, next byte is the option */
    IAC_SUBNEG,     /* inside IAC SB ... IAC SE */
    IAC_SUBNEG_IAC, /* inside subnegotiation, saw IAC */
};

typedef struct telnet_session {
    io_handle_t io;
    FILE file;

    tcp_socket_t *sock;

    /* Set once the connection is gone. The session object deliberately
     * outlives the connection: a command backgrounded with '&' inherits this
     * FILE* through TLS and may still be running, so instead of freeing we go
     * quiet and start returning errors. See the comment in session_release().
     */
    volatile bool dead;

    /* guards txbuf/txpos, since backgrounded commands share this session */
    mutex_t lock;

    char txbuf[TELNETD_TX_BUFSIZE];
    size_t txpos;
    bool last_was_cr; /* so an explicit CR LF doesn't become CR CR LF */

    uint8_t rxbuf[TELNETD_RX_BUFSIZE];
    enum telnet_iac_state iac_state;
} telnet_session_t;

static inline telnet_session_t *session_from_io(io_handle_t *io) {
    return containerof(io, telnet_session_t, io);
}

/* Push whatever is in txbuf at the socket. Caller holds the lock. */
static status_t session_flush_locked(telnet_session_t *s) {
    if (s->txpos == 0) {
        return NO_ERROR;
    }

    size_t pos = s->txpos;
    s->txpos = 0;

    if (s->dead) {
        return ERR_CHANNEL_CLOSED;
    }

    ssize_t ret = tcp_write(s->sock, s->txbuf, pos);
    if (ret < 0) {
        s->dead = true;
        return ERR_CHANNEL_CLOSED;
    }

    return NO_ERROR;
}

static ssize_t telnet_io_write(io_handle_t *io, const char *buf, size_t len) {
    telnet_session_t *s = session_from_io(io);

    if (s->dead) {
        /* Swallow output from anything still running against a closed session,
         * rather than failing it. It has nowhere useful to go.
         */
        return len;
    }

    mutex_acquire(&s->lock);

    for (size_t i = 0; i < len; i++) {
        const char c = buf[i];

        /* A telnet NVT ends a line with CR LF. The kernel emits bare LF (the
         * serial path adds the CR down in platform_dputc, which we don't go
         * through), and a client terminal in raw mode takes a lone LF as "down
         * one row, same column" -- so output walks diagonally across the
         * screen without this. Don't double up if a CR is already there.
         */
        const bool insert_cr = (c == '\n' && !s->last_was_cr);
        const size_t need = insert_cr ? 2 : 1;

        if (sizeof(s->txbuf) - s->txpos < need) {
            if (session_flush_locked(s) < 0) {
                break;
            }
        }

        if (insert_cr) {
            s->txbuf[s->txpos++] = '\r';
        }
        s->txbuf[s->txpos++] = c;
        s->last_was_cr = (c == '\r');

        /* flush at end of line, or when the buffer is full */
        if (c == '\n' || s->txpos == sizeof(s->txbuf)) {
            if (session_flush_locked(s) < 0) {
                break;
            }
        }
    }

    mutex_release(&s->lock);

    return len;
}

static status_t telnet_io_flush(io_handle_t *io) {
    telnet_session_t *s = session_from_io(io);

    mutex_acquire(&s->lock);
    status_t err = session_flush_locked(s);
    mutex_release(&s->lock);

    return err;
}

/* Read at least one byte of real data, stripping telnet control sequences.
 * Returns a negative error when the connection is gone, which the console's
 * line reader turns into end of input.
 */
static ssize_t telnet_io_read(io_handle_t *io, char *buf, size_t len) {
    telnet_session_t *s = session_from_io(io);

    if (len == 0) {
        return 0;
    }
    if (s->dead) {
        return ERR_CHANNEL_CLOSED;
    }

    for (;;) {
        /* anything we're about to print should go out before we block */
        telnet_io_flush(io);

        ssize_t ret = tcp_read(s->sock, s->rxbuf, MIN(len, sizeof(s->rxbuf)));
        if (ret <= 0) {
            s->dead = true;
            return ERR_CHANNEL_CLOSED;
        }

        size_t out = 0;
        for (ssize_t i = 0; i < ret; i++) {
            uint8_t c = s->rxbuf[i];

            switch (s->iac_state) {
                case IAC_NONE:
                    if (c == TELNET_IAC) {
                        s->iac_state = IAC_SEEN;
                    } else if (c == 0) {
                        /* NVT sends NUL after CR; drop it */
                    } else {
                        buf[out++] = (char)c;
                    }
                    break;
                case IAC_SEEN:
                    if (c == TELNET_IAC) {
                        /* escaped 255 */
                        buf[out++] = (char)c;
                        s->iac_state = IAC_NONE;
                    } else if (c == TELNET_WILL || c == TELNET_WONT ||
                               c == TELNET_DO || c == TELNET_DONT) {
                        s->iac_state = IAC_OPTION;
                    } else if (c == TELNET_SB) {
                        s->iac_state = IAC_SUBNEG;
                    } else {
                        /* two byte command, already consumed */
                        s->iac_state = IAC_NONE;
                    }
                    break;
                case IAC_OPTION:
                    /* We don't negotiate beyond the opening offer, so just
                     * consume whatever the client proposes.
                     */
                    s->iac_state = IAC_NONE;
                    break;
                case IAC_SUBNEG:
                    if (c == TELNET_IAC) {
                        s->iac_state = IAC_SUBNEG_IAC;
                    }
                    break;
                case IAC_SUBNEG_IAC:
                    s->iac_state = (c == TELNET_SE) ? IAC_NONE : IAC_SUBNEG;
                    break;
            }
        }

        /* if the read was entirely protocol chatter, go around again */
        if (out > 0) {
            return (ssize_t)out;
        }
    }
}

static const io_handle_hooks_t telnet_io_hooks = {
    .write = telnet_io_write,
    .read  = telnet_io_read,
    .flush = telnet_io_flush,
};

static telnet_session_t *session_create(tcp_socket_t *sock) {
    telnet_session_t *s = calloc(1, sizeof(telnet_session_t));
    if (!s) {
        return NULL;
    }

    /* Line buffered so that several threads printing into one session (a shell
     * plus anything it backgrounded with '&') don't interleave mid-line, the
     * same way console output doesn't.
     */
    io_handle_init_etc(&s->io, &telnet_io_hooks, IO_HANDLE_FLAG_LINE_BUFFERED);
    s->file.io = &s->io;
    s->sock = sock;
    s->iac_state = IAC_NONE;
    mutex_init(&s->lock);

    return s;
}

/* Tear down the connection but not the session object.
 *
 * Two separate things can still point at this session after the connection is
 * gone, and both would be left dangling by a free():
 *
 *  - A command backgrounded with '&' inherits this session's FILE* through TLS
 *    (thread_create copies all TLS slots), so it can outlive the shell thread.
 *
 *  - Any thread holding a partial line destined for this session records the
 *    handle in its linebuffer_owner (see lib/io), and only drops that reference
 *    when the line is flushed or the destination changes.
 *
 * Marking the session dead makes all further writes no-ops instead.
 *
 * TODO: refcount the session against both kinds of reference so it can actually
 * be freed. Until then a session costs one leaked object per connection.
 */
static void session_release(telnet_session_t *s) {
    mutex_acquire(&s->lock);
    s->dead = true;
    s->txpos = 0;
    mutex_release(&s->lock);

    tcp_close(s->sock);
    s->sock = NULL;
}

static int telnetd_session_thread(void *arg) {
    telnet_session_t *s = (telnet_session_t *)arg;

    /* ask the client for character at a time mode */
    tcp_write(s->sock, telnet_server_options, sizeof(telnet_server_options));

    console_t *con = console_create_etc(true, &s->file, &s->file);
    if (!con) {
        TRACEF("error creating console for telnet session\n");
        session_release(s);
        return ERR_NO_MEMORY;
    }

    fprintf(&s->file, "\nwelcome to lk\n\n");

    /* runs until the console loop ends, which happens when the connection
     * drops and the line reader sees end of input
     */
    console_start(con);

    session_release(s);

    /* NOTE: con is deliberately not freed either; console_get_current() may
     * still be reachable from a backgrounded command. Same TODO as above.
     */

    return 0;
}

static int telnetd_listen_thread(void *arg) {
    tcp_socket_t *listen_socket;

    status_t err = tcp_open_listen(&listen_socket, TELNETD_PORT);
    if (err < 0) {
        TRACEF("error %d opening telnet listen socket on port %d\n", err, TELNETD_PORT);
        return err;
    }

    dprintf(INFO, "telnetd: listening on port %d\n", TELNETD_PORT);

    for (;;) {
        tcp_socket_t *accept_socket;

        err = tcp_accept(listen_socket, &accept_socket);
        if (err < 0) {
            TRACEF("error %d accepting telnet socket, retrying\n", err);
            continue;
        }

        telnet_session_t *s = session_create(accept_socket);
        if (!s) {
            TRACEF("error allocating telnet session\n");
            tcp_close(accept_socket);
            continue;
        }

        thread_t *t = thread_create("telnetd_session", &telnetd_session_thread, s,
                                    DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        if (!t) {
            TRACEF("error creating telnet session thread\n");
            session_release(s);
            continue;
        }
        thread_detach_and_resume(t);
    }
}

static void telnetd_entry(const struct app_descriptor *app, void *args) {
    thread_detach_and_resume(thread_create("telnetd", &telnetd_listen_thread, NULL,
                                           DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
}

APP_START(telnetd)
.entry = telnetd_entry,
APP_END
