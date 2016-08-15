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
 *
 *******************************************************************************
 *
 * This app serves as an example of how to use the high level bidirectional
 * debugging pipe.
 *
 * The high level debugging protocol supports three operations:
 *
 *  + ndebugusr_await_host(timeout);
 *  + ndebug_read_usr(buffer, timeout);
 *  + ndebug_write_usr(buffer, bytes, timeout);
 *
 * The client must call ndebugusr_await_host prior to using the read/write api.
 * When a transport is connected and a remote debugger becomes available, this
 * method will return NO_ERROR. At this point, the client is welcome to use the
 * read/write API.
 *
 * Any subsequent read/write operation may return ERR_CHANNEL_CLOSED. This is
 * because we must be able to accommodate the possibility of the device becoming
 * disconnected.
 * If ERR_CHANNEL_CLOSED is received, the client MUST assume that the connection
 * has closed and will not be recovered.
 * The client MUST NOT issue any more read or write calls.
 * The client MUST call ndebugusr_await_host and wait for the host to become
 * active again at which point the client will be given a new connection to a
 * new host.
 *
 * The following example app implements a simple echo server that demonstrates
 * data being read then echoed back to the host.
 */

#include <lib/ndebug/ndebug.h>
#include <lib/ndebug/user.h>

#include <app.h>
#include <debug.h>
#include <err.h>
#include <string.h>

static void ndebugtest_entry(const struct app_descriptor *app, void *args)
{
    // Pad the buffer with one extra byte to acommodate a NULL character for
    // printing using printf(...)
    static uint8_t buf[NDEBUG_USR_MAX_PACKET_SIZE + 1];

    while (true) {
        // Wait for USB to connect and somebody to answer the phone on the other
        // end of the line.
        status_t result = ndebug_await_connection_usr(INFINITE_TIME);
        if (result != NO_ERROR) {
            printf("Error %d while awaiting host. Retrying...\n", result);
            continue;
        }

        // Read bytes from the USB stack and echo them back to the host until
        // the channel is closed. When the channel is closed, go back to waiting
        // for the host to reopen the channel.
        // Continue this process indefinitely.
        while (true) {

            // Read some bytes from the USB stack
            ssize_t bytes_read =
                ndebug_read_usr(buf, INFINITE_TIME);
            if (bytes_read == ERR_CHANNEL_CLOSED) {
                printf("Channel reset during read.\n");
                break;
            } else if (bytes_read < 0) {
                printf("Error while reading. Retcode = %ld\n", bytes_read);
                break;
            }

            // Tack a NULL to the end of the bytes and print them to the
            // console.
            buf[bytes_read] = '\0';
            printf("%s", buf);

            // Echo the bytes back the device.
            ssize_t bytes_written =
                ndebug_write_usr(buf, bytes_read, INFINITE_TIME);
            if (bytes_written == ERR_CHANNEL_CLOSED) {
                printf("Channel reset during write.\n");
                break;
            } else if (bytes_written < 0) {
                printf("Error while writing. Retcode = %ld\n", bytes_written);
                break;
            }

        }
    }
}

APP_START(ndebugtest)
  .entry = ndebugtest_entry,
APP_END