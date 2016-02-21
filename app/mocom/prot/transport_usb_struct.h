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

#ifndef __TRANSPORT_USB_STRUCT_H
#define __TRANSPORT_USB_STRUCT_H

#include <inttypes.h>

#define USBLL_MAGIC 0x7573626c // 'usbl'
#define USBLL_VERSION 1

struct usbll_header {
    uint32_t magic;
    uint32_t version;
    uint32_t txid;
    uint32_t rxid;
    uint32_t command;
};
/*
 * Sync header definition
 * It is possible to send accross more data than sizeof(usbll_syn_header):
 *   data_offset:   offset of string data (calculated starting from usbll_syn_header)
 *   data_length:   sizeof string data (including \0 string terminator)
 */
struct usbll_syn_header {
    uint8_t nduid[40];
    uint32_t mtu;

    /* added later, short header means this is default */
    uint32_t heartbeat_interval;
    uint32_t timeout;

    /* adding support of extra data (assumed string) after syn header*/
    uint32_t data_offset;
    uint32_t data_length;
};

enum usbll_command {
    usbll_null = 0, // an ack-like packet to detect spurious syns
    usbll_rst,
    usbll_syn,
    usbll_data,
};

/*
 * Recover token structure definition
 * USBLL recovery token structure consist of txid, rxid, magic, version (see protocol header)
 */
typedef struct usbll_recovery_token {
    uint32_t magic;
    uint32_t version;
    uint32_t txid;
    uint32_t rxid;
} usbll_recovery_token_t;

#endif

