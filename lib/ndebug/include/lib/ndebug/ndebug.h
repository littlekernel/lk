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

#pragma once

#include <compiler.h>
#include <stdint.h>
#include <sys/types.h>

#define NDEBUG_MAX_PACKET_SIZE (64)

__BEGIN_CDECLS

typedef enum {
    NDEBUG_CHANNEL_SYS,
    NDEBUG_CHANNEL_USR,

    NDEBUG_CHANNEL_COUNT,   // Count: always last.
} channel_t;

void ndebug_init(void);

ssize_t ndebug_usb_read(const channel_t ch, const size_t n,
                        const lk_time_t timeout, uint8_t *buf);
ssize_t ndebug_usb_write(const channel_t ch, const size_t n,
                         const lk_time_t timeout, uint8_t *buf);

status_t ndebug_await_connection(const channel_t ch, const lk_time_t timeout);

status_t msg_host(const channel_t ch, const uint32_t message,
                  const lk_time_t timeout, uint8_t *buf);
__END_CDECLS