/* @@@LICENSE
*
*      Copyright (c) 2008-2013 LG Electronics, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef __LIB_PMUX_MUX_H
#define __LIB_PMUX_MUX_H

#include <sys/types.h>
#include <inttypes.h>

/* pmux definitions */
#define PMUX_MAGIC    0x706d7578 /* 'pmux' */
#define PMUX_VERSION  3   /* current version */

#define PMUX_FLAG_ACK 1 /* this packet is an ack */

struct pmux_header {
    uint32_t magic;
    uint8_t  version;
    uint8_t  _pad;
    uint16_t flags;
    uint32_t channel;
    uint32_t sequence;
    uint32_t payload_len;
    uint32_t total_len;

    uint32_t crc;

    uint8_t data[0];
};

/* control messages */
enum pmux_control_message_op {
    PMUX_CONTROL_NONE = 0,
    PMUX_CONTROL_CHANNEL_CLOSED,
    PMUX_CONTROL_OPEN,
    PMUX_CONTROL_OPEN_COMMAND, /* open a channel with a command interpreter */
    PMUX_CONTROL_CLOSE,
};

struct pmux_control_header {
    uint32_t op;
    uint32_t channel;
    uint32_t len;
    uint8_t  data[0];
};

enum pmux_channel_num {
    PMUX_CHANNEL_NULL,                       /* non channel, for when you actually dont want to send data */
    PMUX_CHANNEL_CONTROL,                    /* special control commands go over this channel */

    PMUX_CHANNEL_CMDSERVICE   = 0x50,        /* device service commands go over this channel */

    PMUX_CHANNEL_MAX_FIXED    = 0xfff,
    PMUX_CHANNEL_HOST_BASE    = 0x1000,
    PMUX_CHANNEL_MAX_HOST     = 0x7fffffff,
    PMUX_CHANNEL_DEVICE_BASE  = 0x80000000,
    PMUX_CHANNEL_MAX_DEVICE   = 0xffffffff
};

#endif

