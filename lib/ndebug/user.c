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

#include <lib/ndebug/ndebug.h>
#include <lib/ndebug/shared_structs.h>
#include <lib/ndebug/user.h>

#include <assert.h>
#include <err.h>
#include <kernel/mutex.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>

#define LOCAL_TRACE 0

#define HOST_MSG_TIMEOUT (1000)  // 1 Second

static volatile bool connected = false;

static mutex_t usr_mutex = MUTEX_INITIAL_VALUE(usr_mutex);

static uint8_t scratch_buffer[NDEBUG_MAX_PACKET_SIZE];

status_t ndebug_await_connection_usr(lk_time_t timeout)
{
    LTRACEF("timeout: %u\n", timeout);

    status_t result = ndebug_await_connection(NDEBUG_CHANNEL_USR, timeout);

    if (result == NO_ERROR) {
        connected = true;
    } else {
        connected = false;
    }

    return result;
}

ssize_t ndebug_read_usr(uint8_t *buf, const lk_time_t timeout)
{
    LTRACEF("buf: 0x%p, timeout: %u\n", buf, timeout);

    if (!connected) {
        // User must call ndebugusr_await_host and establish a connection
        // before continuing.
        return ERR_CHANNEL_CLOSED;
    }

    mutex_acquire(&usr_mutex);

    // Retry Loop
    while (true) {
        ssize_t result = ndebug_usb_read(NDEBUG_CHANNEL_USR,
                                         NDEBUG_MAX_PACKET_SIZE,
                                         timeout, scratch_buffer);
        if (result < (ssize_t)sizeof(ndebug_ctrl_packet_t)) {
            dprintf(INFO, "Short Packet. Len = %ld\n", result);
            continue;
        }

        ndebug_ctrl_packet_t *ctrl = (ndebug_ctrl_packet_t *)scratch_buffer;
        if (ctrl->magic != NDEBUG_CTRL_PACKET_MAGIC) {
            dprintf(INFO, "Bad Packet Magic = %u\n", ctrl->magic);
            continue;
        }

        if (ctrl->type == NDEBUG_CTRL_CMD_RESET) {
            msg_host(NDEBUG_CHANNEL_USR, NDEBUG_CTRL_CMD_RESET, timeout,
                     scratch_buffer);
            connected = false;
            mutex_release(&usr_mutex);
            return ERR_CHANNEL_CLOSED;
        } else if (ctrl->type == NDEBUG_CTRL_CMD_DATA) {
            const ssize_t n_data_bytes = result - sizeof(ndebug_ctrl_packet_t);
            memcpy(buf, scratch_buffer + sizeof(ndebug_ctrl_packet_t),
                   n_data_bytes);
            mutex_release(&usr_mutex);
            return n_data_bytes;
        } else {
            dprintf(INFO, "Unexpected packet type = %u\n", ctrl->type);
        }
    }
}

ssize_t ndebug_write_usr(uint8_t *buf, const size_t n, const lk_time_t timeout)
{
    LTRACEF("buf = 0x%p, n = %u, timeout = %u\n", buf, n, timeout);

    if (!connected) {
        // User must call ndebugusr_await_host and establish a connection
        // before continuing.
        return ERR_CHANNEL_CLOSED;
    }

    mutex_acquire(&usr_mutex);

    uint8_t *cursor = scratch_buffer;

    ndebug_ctrl_packet_t *pkt = (ndebug_ctrl_packet_t *)cursor;
    pkt->magic = NDEBUG_CTRL_PACKET_MAGIC;
    pkt->type = NDEBUG_CTRL_CMD_DATA;

    cursor += sizeof(ndebug_ctrl_packet_t);

    ssize_t res;
    size_t bytes_remaining = n;
    do {
        size_t bytes_to_copy = MIN(NDEBUG_USR_MAX_PACKET_SIZE, bytes_remaining);

        memcpy(cursor, buf, bytes_to_copy);

        buf += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;

        res = ndebug_usb_write(
                  NDEBUG_CHANNEL_USR,
                  bytes_to_copy + sizeof(ndebug_ctrl_packet_t),
                  timeout,
                  scratch_buffer
              );

        if (res < 0) {
            break;
        }

    } while (bytes_remaining > 0);

    mutex_release(&usr_mutex);

    if (res < 0) {
        return res;
    } else {
        return (ssize_t)n;
    }
}
