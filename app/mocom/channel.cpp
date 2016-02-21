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
#include "channel.hpp"

#include <err.h>
#include <debug.h>
#include <stdio.h>
#include <assert.h>
#include <trace.h>
#include <string.h>
#include <rand.h>
#include <target.h>
#include <compiler.h>
#include <ctype.h>
#include <platform.h>

#include "mux.hpp"
#include "prot/mux.h"
#include "cmd_handler/console.hpp"

#define LOCAL_TRACE 0

namespace mocom {

// base channel class

status_t channel::queue_tx(const void *buf, size_t len)
{
    LTRACEF("buf %p, len %zu\n", buf, len);

    if (m_tx_buf)
        return ERR_BUSY;

    m_tx_buf = (const uint8_t *)buf;
    m_tx_len = len;

    return NO_ERROR;
}

status_t channel::close()
{
    return NO_ERROR;
}

// command channel

void control_channel::process_rx_packet(const uint8_t *buf, size_t len)
{
    LTRACEF("buf %p, len %zu\n", buf, len);

    const struct pmux_control_header *header = (const struct pmux_control_header *)buf;

    if (len < sizeof(struct pmux_control_header))
        return;

    switch (header->op) {
        default:
        case PMUX_CONTROL_NONE:
            break;
        case PMUX_CONTROL_CHANNEL_CLOSED:
            // they're telling us they got something on a closed channel
            // XXX ignore for now
            break;
        case PMUX_CONTROL_OPEN:
            // they're asking us to open a new channel
            PANIC_UNIMPLEMENTED;
            break;
        case PMUX_CONTROL_OPEN_COMMAND: {
            // they're asking us to open a command interpreter channel
            channel *c = new command_channel(m_mux, header->channel);
            if (!m_mux.add_channel(c)) {
                // already exists
                delete c;
            }
            break;
        }
        case PMUX_CONTROL_CLOSE: {
            // they're asking us to close a channel
            channel *c = m_mux.find_channel(header->channel);
            if (c) {
                c->close();

                m_mux.remove_channel(c);
                delete c;
            }
            break;
        }
    }
}

// command channel

static int tokenize_command(const char *inbuffer, size_t inbuflen, char *buffer, size_t buflen, char **args, size_t arg_count)
{
    size_t inpos;
    size_t outpos;
    size_t arg;
    enum {
        INITIAL = 0,
        NEXT_FIELD,
        SPACE,
        IN_SPACE,
        TOKEN,
        IN_TOKEN,
        QUOTED_TOKEN,
        IN_QUOTED_TOKEN,
    } state;

    inpos = 0;
    outpos = 0;
    arg = 0;
    state = INITIAL;

    for (;;) {
        char c = inbuffer[inpos];

        DEBUG_ASSERT(inpos < inbuflen);

        LTRACEF_LEVEL(2, "c 0x%hhx state %d arg %d inpos %d pos %d\n", c, state, arg, inpos, outpos);

        switch (state) {
            case INITIAL:
            case NEXT_FIELD:
                if (c == '\0')
                    goto done;
                if (isspace(c))
                    state = SPACE;
                else
                    state = TOKEN;
                break;
            case SPACE:
                state = IN_SPACE;
                break;
            case IN_SPACE:
                if (c == '\0')
                    goto done;
                if (!isspace(c)) {
                    state = TOKEN;
                } else {
                    inpos++; // consume the space
                    if (inpos == inbuflen)
                        goto done;
                }
                break;
            case TOKEN:
                // start of a token
                DEBUG_ASSERT(c != '\0');
                if (c == '"') {
                    // start of a quoted token
                    state = QUOTED_TOKEN;
                } else {
                    // regular, unquoted token
                    state = IN_TOKEN;
                    args[arg] = &buffer[outpos];
                }
                break;
            case IN_TOKEN:
                if (c == '\0') {
                    arg++;
                    goto done;
                }
                if (isspace(c)) {
                    arg++;
                    buffer[outpos] = 0;
                    outpos++;
                    /* are we out of tokens? */
                    if (arg == arg_count)
                        goto done;
                    state = NEXT_FIELD;
                } else {
                    buffer[outpos] = c;
                    outpos++;
                    inpos++;
                    if (inpos == inbuflen) {
                        arg++;
                        goto done;
                    }
                }
                break;
            case QUOTED_TOKEN:
                // start of a quoted token
                DEBUG_ASSERT(c == '"');

                state = IN_QUOTED_TOKEN;
                args[arg] = &buffer[outpos];
                inpos++; // consume the quote
                if (inpos == inbuflen)
                    goto done;
                break;
            case IN_QUOTED_TOKEN:
                if (c == '\0') {
                    arg++;
                    goto done;
                }
                if (c == '"') {
                    arg++;
                    buffer[outpos] = 0;
                    outpos++;
                    /* are we out of tokens? */
                    if (arg == arg_count)
                        goto done;

                    state = NEXT_FIELD;
                }
                buffer[outpos] = c;
                outpos++;
                inpos++;
                if (inpos == inbuflen) {
                    arg++;
                    goto done;
                }
                break;
        }
        /* are we at the end of the output buffer? */
        if (outpos == buflen - 1)
            break;
    }

done:
    buffer[outpos] = 0;
    return arg;
}

void command_channel::process_rx_packet(const uint8_t *buf, size_t len)
{
    LTRACEF("buf %p, len %zu\n", buf, len);

    //hexdump8_ex(buf, len, 0);

    if (m_state == STATE_INITIAL) {
        // try to parse the incoming command

        char outbuf[128];
        char *tokens[16];
        int count = tokenize_command((const char *)buf, len, outbuf, sizeof(outbuf), tokens, countof(tokens));

        LTRACEF("command word count %d\n", count);

        if (count >= 2 && !strcmp(tokens[0], "open") && !strcmp(tokens[1], "console")) {
            // the only command we understand right now
            DEBUG_ASSERT(!m_handler);

            cmd_handler::console *c = new cmd_handler::console(*this);
            if (c->Init() < NO_ERROR) {
                delete c;
                goto error;
            }
            m_handler = c;

            queue_tx(strdup("ok 0\n"), strlen("ok 0\n"));
            m_state = STATE_ESTABLISHED;
        } else {
error:
            // everything else is unrecognized
            snprintf(outbuf, sizeof(outbuf), "err %d %s\n", -1, "unrecognized command");
            LTRACEF("returning '%s\n", outbuf);
            queue_tx(strdup(outbuf), strlen(outbuf));
        }
    } else if (m_state == STATE_ESTABLISHED) {
        DEBUG_ASSERT(m_handler);

        m_handler->process_rx_packet(buf, len);
    }
}

void command_channel::tx_complete()
{
    LTRACE_ENTRY;

    channel::tx_complete();

    if (m_handler)
        m_handler->tx_complete();
}

status_t command_channel::close()
{
    LTRACE_ENTRY;

    if (m_handler)
        m_handler->close();

    return channel::close();
}

command_channel::~command_channel()
{
    if (m_handler)
        delete m_handler;
}

};

