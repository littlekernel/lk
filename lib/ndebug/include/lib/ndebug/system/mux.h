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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <lib/ndebug/ndebug.h>
#include <lib/ndebug/shared_structs.h>

#define MAX_MUX_PACKET_SIZE (NDEBUG_MAX_PACKET_SIZE - sizeof(ndebug_system_packet_t))

void ndebug_sys_init(void);
bool ndebug_sys_connected(void);

ssize_t ndebug_read_sys(uint8_t **buf, const sys_channel_t ch,
                        const lk_time_t timeout);

ssize_t ndebug_write_sys(const uint8_t *buf, const size_t n,
                         const sys_channel_t ch, const lk_time_t timeout);
