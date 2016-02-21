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

#ifndef __PACKET_STRUCT_H
#define __PACKET_STRUCT_H

#include <stdint.h>

/* on the wire packet data structure */
#define PACKET_HEADER_MAGIC 0xDECAFBAD
#define PACKET_HEADER_VERSION 1

/* packet types */
#define PACKET_HEADER_TYPE_DATA 0
#define PACKET_HEADER_TYPE_ERR 1
#define PACKET_HEADER_TYPE_OOB 2

struct packet_header {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t type;

    uint8_t  data[0];
};

/* OOB messages */
#define PACKET_OOB_EOF      0
#define PACKET_OOB_SIGNAL   1
#define PACKET_OOB_RETURN   2
#define PACKET_OOB_RESIZE   3

/* data payload for OOB messages */
struct packet_oob_msg {
    uint32_t message;
    union {
        uint32_t signo;
        int32_t returncode;
        int32_t fileno; /* stdin, stdout, stderr */
        struct {
            uint32_t rows;
            uint32_t cols;
        } resize;
        uint32_t pad[4]; /* pad it out to 16 bytes, for future expansion*/
    } data;
};

#endif

