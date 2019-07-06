/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

/* in memory and DCC descriptors for the PDCC protocol */

/* shared outside of lk repository, be careful of modifications */
#define PDCC_VERSION 1

struct pdcc_buffer_descriptor {
    uint32_t version;

    uint32_t htod_buffer_phys;
    uint32_t htod_buffer_len;

    uint32_t dtoh_buffer_phys;
    uint32_t dtoh_buffer_len;
};

#define PDCC_VALID (1<<31)
#define PDCC_OPCODE_SHIFT (24)
#define PDCC_OPCODE(x) (((x) >> PDCC_OPCODE_SHIFT) & 0x7f)
#define PDCC_DATA(x) ((x) & 0x00ffffff);

enum {
    PDCC_OP_RESET = 0,
    PDCC_OP_BUF_HEADER,
    PDCC_OP_UPDATE_OUT_INDEX,
    PDCC_OP_CONSUMED_IN,
};

