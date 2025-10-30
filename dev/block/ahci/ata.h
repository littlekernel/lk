//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <hw/ata.h>

// offsets in the 256 word (2 byte word) IDENTIFY structure
enum ata_identify_words {
    ATA_IDENTIFY_MODEL_NUMBER = 27,                // 40 bytes
    ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD = 100, // 4 words of logical sector count
    ATA_IDENTIFY_PHYS_TO_LOGICAL_SECTOR = 106,     // phys size / logical size
    ATA_IDENTIFY_LOGICAL_SECTOR_SIZE_DWORD = 117,  // dword of logical sector size
};

// ata helper routines
inline FIS_REG_H2D ata_cmd_identify() {
    FIS_REG_H2D fis = {};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.command = ATA_CMD_IDENTIFY;
    fis.device = 0; // drive 0
    fis.c = 1; // command

    return fis;
}
