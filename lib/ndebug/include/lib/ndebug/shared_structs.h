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

#include <stdint.h>

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint32_t type;
}
ndebug_ctrl_packet_t;

typedef struct __attribute__((packed))
{
    ndebug_ctrl_packet_t ctrl;
    uint32_t channel;
}
ndebug_system_packet_t;

typedef enum {
    NDEBUG_SYS_CHANNEL_CONSOLE,
    NDEBUG_SYS_CHANNEL_COMMAND,

    NDEBUG_SYS_CHANNEL_COUNT,   // Count: always last.
} sys_channel_t;

#define NDEBUG_CTRL_PACKET_MAGIC (0x4354524C)

#define NDEBUG_CTRL_CMD_RESET (0x01)
#define NDEBUG_CTRL_CMD_DATA (0x02)
#define NDEBUG_CTRL_CMD_ESTABLISHED (0x03)
#define NDEBUG_CTRL_CMD_FLOWCTRL (0x04)

#define NDEBUG_SYS_CHANNEL_READY (0x01)

#define NDEBUG_USB_CLASS_USER_DEFINED (0xFF)
#define NDEBUG_SUBCLASS (0x02)

#define NDEBUG_PROTOCOL_LK_SYSTEM (0x01)
#define NDEBUG_PROTOCOL_SERIAL_PIPE (0x02)