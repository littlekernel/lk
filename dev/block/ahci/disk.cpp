//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include "disk.h"

#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdint.h>

#include "ata.h"

#define LOCAL_TRACE 1

ahci_disk::ahci_disk(ahci_port &p) : port_(p) {
}

ahci_disk::~ahci_disk() = default;

status_t ahci_disk::identify() {
    LTRACE_ENTRY;

    __ALIGNED(512) static uint8_t identify_data[512];
    FIS_REG_H2D fis = ata_cmd_identify();

    int slot;
    auto err = port_.queue_command(&fis, sizeof(fis), identify_data, sizeof(identify_data), false, &slot);
    if (err != NO_ERROR) {
        return err;
    }

    // wait for it to complete
    err = port_.wait_for_completion(slot);
    if (err != NO_ERROR) {
        return err;
    }

    LTRACEF("identify data:\n");
    hexdump8(identify_data, sizeof(identify_data));

    return NO_ERROR;
}

