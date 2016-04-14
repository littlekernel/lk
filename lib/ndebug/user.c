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
#include <string.h>
#include <trace.h>

#define LOCAL_TRACE 0

#define HOST_MSG_TIMEOUT (1000)  // 1 Second

static volatile bool connected = false;

static mutex_t usr_mutex = MUTEX_INITIAL_VALUE(usr_mutex);

static uint8_t scratch_buffer[NDEBUG_MAX_PACKET_SIZE];

static bool is_valid_connection_request(ssize_t n, const uint8_t *buf)
{
    LTRACEF("length: %ld, buf: 0x%p\n", n, buf);

    if (n < (ssize_t)sizeof(ndebug_ctrl_packet_t)) {
        dprintf(INFO, "Malformed Packet. Expected at least %u bytes, got %ld\n",
                sizeof(ndebug_ctrl_packet_t), n);
        return false;
    }

    ndebug_ctrl_packet_t *pkt = (ndebug_ctrl_packet_t *)buf;

    return pkt->magic == NDEBUG_CTRL_PACKET_MAGIC &&
           pkt->type == NDEBUG_CTRL_CMD_RESET;
}

static void write_buffer(const uint32_t message, uint8_t *buf)
{
    ndebug_ctrl_packet_t *pkt = (ndebug_ctrl_packet_t *)scratch_buffer;

    pkt->magic = NDEBUG_CTRL_PACKET_MAGIC;
    pkt->type = message;
}

static status_t send_message_to_host(const uint32_t message)
{
    LTRACEF("message: %d\n", message);

    write_buffer(message, scratch_buffer);

    ssize_t res = ndebug_usb_write(NDEBUG_CHANNEL_USR,
                                   sizeof(ndebug_ctrl_packet_t),
                                   HOST_MSG_TIMEOUT,
                                   scratch_buffer);

    if (res == ERR_TIMED_OUT) {
        dprintf(INFO, "send message %d timed out\n", message);
    } else if (res < 0) {
        dprintf(INFO, "send message %d failed with error %ld\n", message, res);
    }
    return res;
}

status_t ndebugusr_await_host(lk_time_t timeout)
{
    LTRACEF("timeout: %u\n", timeout);

    status_t result = await_usb_online(timeout);
    if (result != NO_ERROR) {
        return result;
    }

    while (true) {
        ssize_t bytes = ndebug_usb_read(NDEBUG_CHANNEL_USR,
                                        NDEBUG_MAX_PACKET_SIZE,
                                        timeout, scratch_buffer);
        if (bytes < 0) {
            return bytes;
        } else if (bytes < (ssize_t)sizeof(ndebug_ctrl_packet_t)) {
            continue;
        }

        if (is_valid_connection_request(bytes, scratch_buffer)) {
            // Connection established.
            if (send_message_to_host(NDEBUG_CTRL_CMD_ESTABLISHED) < 0) {
                continue;
            }
            connected = true;
            return NO_ERROR;
        } else {
            // Tell the host that it needs to reset the connection before
            // attempting any further communication.
            send_message_to_host(NDEBUG_CTRL_CMD_RESET);
        }
    }
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
            send_message_to_host(NDEBUG_CTRL_CMD_RESET);
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

    // TODO(gkalsi): Packetize the packet that's too big and send the data
    //               anyway.
    if (n > NDEBUG_USR_MAX_PACKET_SIZE)
        return ERR_TOO_BIG;

    mutex_acquire(&usr_mutex);

    uint8_t *cursor = scratch_buffer;

    write_buffer(NDEBUG_CTRL_CMD_DATA, cursor);

    cursor += sizeof(ndebug_ctrl_packet_t);

    memcpy(cursor, buf, n);

    ssize_t res = ndebug_usb_write(NDEBUG_CHANNEL_USR,
                                   n + sizeof(ndebug_ctrl_packet_t),
                                   timeout,
                                   scratch_buffer);

    mutex_release(&usr_mutex);

    return res;
}
