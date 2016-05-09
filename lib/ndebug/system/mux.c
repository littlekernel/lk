/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
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

#include <lib/ndebug/system/mux.h>

#include <assert.h>
#include <err.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <lib/ndebug/ndebug.h>
#include <lib/ndebug/shared_structs.h>
#include <lib/ndebug/system/consoleproxy.h>
#include <string.h>

typedef struct {
    event_t event;
    uint8_t *buf;
    ssize_t retcode;
    bool ready;
} ch_t;

ch_t channels[NDEBUG_SYS_CHANNEL_COUNT];
static volatile bool connected = false;

mutex_t write_sys_internal_lock = MUTEX_INITIAL_VALUE(write_sys_internal_lock);

#define FLOWCTRL_PKT_TIMEOUT 1000

static ssize_t ndebug_write_sys_internal(const uint8_t *buf, const size_t n,
        const sys_channel_t ch,
        const lk_time_t timeout,
        const uint32_t type)
{
    DEBUG_ASSERT(buf);

    if (n > MAX_MUX_PACKET_SIZE)
        return ERR_TOO_BIG;

    if (ch >= NDEBUG_SYS_CHANNEL_COUNT)
        return ERR_INVALID_ARGS;

    mutex_acquire(&write_sys_internal_lock);

    uint8_t write_buf[NDEBUG_MAX_PACKET_SIZE];
    uint8_t *cursor = write_buf;

    ndebug_system_packet_t *pkt = (ndebug_system_packet_t *)cursor;
    pkt->ctrl.magic = NDEBUG_CTRL_PACKET_MAGIC;
    pkt->ctrl.type = type;
    pkt->channel = ch;

    cursor += sizeof(ndebug_system_packet_t);
    memcpy(cursor, buf, n);

    ssize_t written = ndebug_usb_write(
                          NDEBUG_CHANNEL_SYS,
                          n + sizeof(ndebug_system_packet_t),
                          timeout,
                          write_buf
                      );

    if (written < 0) {
        connected = false;
    }

    mutex_release(&write_sys_internal_lock);

    return written;
}

static int reader_thread(void *argv)
{
    uint8_t buf[NDEBUG_MAX_PACKET_SIZE];

    while (true) {
        status_t result =
            ndebug_await_connection(NDEBUG_CHANNEL_SYS, INFINITE_TIME);
        if (result != NO_ERROR) {
            dprintf(INFO, "await_connection failed; status = %d\n", result);
            continue;
        }
        connected = true;

        while (true) {
            ssize_t bytes_read = ndebug_usb_read(
                                     NDEBUG_CHANNEL_SYS, sizeof(buf), INFINITE_TIME, buf);

            if (bytes_read < 0) break;

            if (bytes_read < (ssize_t)sizeof(ndebug_system_packet_t)) continue;

            ndebug_system_packet_t *pkt = (ndebug_system_packet_t *)buf;
            if (pkt->ctrl.magic != NDEBUG_CTRL_PACKET_MAGIC) continue;

            switch (pkt->ctrl.type) {
                case NDEBUG_CTRL_CMD_RESET: {
                    // We got a reset packet, unblock all the reader channels
                    // and go back to waiting for a connection attempt.
                    connected = false;
                    for (sys_channel_t ch = NDEBUG_SYS_CHANNEL_CONSOLE;
                            ch != NDEBUG_SYS_CHANNEL_COUNT; ch++) {
                        channels[ch].retcode = ERR_CHANNEL_CLOSED;
                        event_signal(&channels[ch].event, true);
                    }
                    break;
                }
                case NDEBUG_CTRL_CMD_DATA: {
                    if (pkt->channel >= NDEBUG_SYS_CHANNEL_COUNT) continue;
                    ch_t *channel = &channels[pkt->channel];

                    // Flow control: If this channel never signalled to the host
                    // that it was ready, then the packet is dropped on the
                    // floor.
                    if (!channel->ready) continue;

                    // Deframe the packet.
                    memcpy(channel->buf, buf + sizeof(ndebug_system_packet_t),
                           bytes_read - sizeof(ndebug_system_packet_t));

                    // Set the return code and signal to the blocked reader that
                    // it can pick up the packet.
                    channel->retcode =
                        bytes_read - sizeof(ndebug_system_packet_t);
                    channel->ready = false;
                    event_signal(&channel->event, true);

                    break;
                }
                case NDEBUG_CTRL_CMD_FLOWCTRL: {
                    // The host has requested an inventory of all "ready"
                    // channels.
                    for (sys_channel_t ch = NDEBUG_SYS_CHANNEL_CONSOLE;
                            ch != NDEBUG_SYS_CHANNEL_COUNT; ch++) {
                        ch_t *channel = &channels[ch];

                        if (!channel->ready) continue;
                        uint32_t rdych = (uint32_t)ch;
                        ndebug_write_sys_internal((uint8_t *)(&rdych),
                                                  sizeof(rdych),
                                                  ch, FLOWCTRL_PKT_TIMEOUT,
                                                  NDEBUG_CTRL_CMD_FLOWCTRL);
                    }
                    break;
                }
                default: {
                }
            }
            if (!connected) break;
        }
    }
    return 0;
}

void ndebug_sys_init(void)
{
    // Initialize the Channels.
    for (sys_channel_t ch = NDEBUG_SYS_CHANNEL_CONSOLE;
            ch != NDEBUG_SYS_CHANNEL_COUNT; ch++) {
        event_init(&channels[ch].event, false, EVENT_FLAG_AUTOUNSIGNAL);
        channels[ch].buf = malloc(NDEBUG_MAX_PACKET_SIZE);
        channels[ch].retcode = 0;
        channels[ch].ready = false;
    }

    // Start the reader thread.
    thread_resume(
        thread_create("ndebug mux reader", &reader_thread, NULL,
                      DEFAULT_PRIORITY, DEFAULT_STACK_SIZE)
    );

    // Initialize subsystems.
    consoleproxy_init();
}


ssize_t ndebug_write_sys(const uint8_t *buf, const size_t n,
                         const sys_channel_t ch, const lk_time_t timeout)
{
    ssize_t result =
        ndebug_write_sys_internal(buf, n, ch, timeout, NDEBUG_CTRL_CMD_DATA);
    return result;
}

ssize_t ndebug_read_sys(uint8_t **buf, const sys_channel_t ch,
                        const lk_time_t timeout)
{
    DEBUG_ASSERT(ch < NDEBUG_SYS_CHANNEL_COUNT);

    // Send a message to the host that a channel has become ready.
    if (ndebug_sys_connected()) {
        uint32_t data[2];
        data[0] = NDEBUG_SYS_CHANNEL_READY;
        data[1] = ch;
        ssize_t written =
            ndebug_write_sys_internal((uint8_t *)data, sizeof(data),
                                      ch, FLOWCTRL_PKT_TIMEOUT,
                                      NDEBUG_CTRL_CMD_FLOWCTRL);
        if (written < 0) {
            return written;
        }
    }

    // Block on an event and wait for the reader thread to signal us
    ch_t *channel = &channels[ch];
    channel->ready = true;

    status_t result = event_wait_timeout(&channel->event, timeout);
    if (result != NO_ERROR) {
        return result;
    }


    *buf = channel->buf;

    return channel->retcode;
}

bool ndebug_sys_connected(void)
{
    return connected;
}
