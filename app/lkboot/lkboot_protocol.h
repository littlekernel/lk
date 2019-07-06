/*
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

typedef struct {
    unsigned char opcode;
    unsigned char extra;
    unsigned short length;
} msg_hdr_t;

// unless otherwise specified, extra is always zero.

#define MSG_OKAY    0x00
// length must be zero.
// server indicates command was successful.

#define MSG_FAIL    0xFF
// length may be nonzero, if so data is a human readable error message
// extra may be nonzero, if so it is a more specific error code

#define MSG_LOG     0xFE
// data contains human readable log message from server
// server may issue these at any time

#define MSG_GO_AHEAD    0x01
// length must be zero
// server indicates that command was valid and it is ready for data
// client should send MSG_SEND_DATA messages to transfer data

#define MSG_CMD     0x40
// length must be greater than zero
// data will contain an ascii command
// server may reject excessively large commands

#define MSG_SEND_DATA   0x41
// client sends data to server
// length is datalen -1 (to allow for full 64k chunks)

#define MSG_END_DATA    0x42
// client ends data stream
// server will then respond with MSG_OKAY or MSG_FAIL

// command strings are in the form of
// <command> ':' <decimal-datalen> ':' <optional-arguments>

// example:
// C: MSG_CMD "flash:32768:bootloader"
// S: MSG_GO_AHEAD
// C: MSG_SEND_DATA 16384 ...
// C: MSG_SEND_DATA 16384 ...
// C: MSG_END_DATA
// S: MSG_LOG "erasing sectors"
// S: MSG_LOG "writing sectors"
// S: MSG_OKAY
//
// C: MSG_CMD "eraese:0:bootloader"
// S: MSG_FAIL "unknown command 'eraese'"
//
// C: MSG_CMD "reboot:0:"
// S: MSG_OKAY
