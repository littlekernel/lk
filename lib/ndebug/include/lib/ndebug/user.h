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

#include <lib/ndebug/ndebug.h>
#include <lib/ndebug/shared_structs.h>

typedef struct {
    uint32_t magic;
    uint32_t type;
} ndebug_ctrl_packet_t;

#define NDEBUG_USR_MAX_PACKET_SIZE (NDEBUG_MAX_PACKET_SIZE - sizeof(ndebug_ctrl_packet_t))


// Read and write to the NDebug user channel.
ssize_t ndebug_read_usr(uint8_t *buf, const lk_time_t timeout);
ssize_t ndebug_write_usr(uint8_t *buf, const size_t n, const lk_time_t timeout);


// Wait for the host to establish a connection on the usr channel.
status_t ndebugusr_await_host(lk_time_t timeout);