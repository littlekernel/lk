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
    ATA_IDENTIFY_SERIAL_NUMBER = 10,               // 20 bytes
    ATA_IDENTIFY_FIRMWARE_REVISION = 23,           // 8 bytes
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
    fis.device = 0;
    fis.c = 1; // command

    return fis;
}

inline FIS_REG_H2D ata_cmd_read_dma_ext(uint64_t lba, uint16_t sector_count) {
    FIS_REG_H2D fis = {};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.command = ATA_CMD_READ_DMA_EXT;
    fis.device = 1 << 6; // LBA mode
    fis.c = 1;           // command

    // set LBA
    fis.lba0 = (lba >> 0) & 0xff;
    fis.lba1 = (lba >> 8) & 0xff;
    fis.lba2 = (lba >> 16) & 0xff;
    fis.lba3 = (lba >> 24) & 0xff;
    fis.lba4 = (lba >> 32) & 0xff;
    fis.lba5 = (lba >> 40) & 0xff;

    // set sector count
    fis.countl = sector_count & 0xff;
    fis.counth = (sector_count >> 8) & 0xff;

    return fis;
}

inline FIS_REG_H2D ata_cmd_write_dma_ext(uint64_t lba, uint16_t sector_count) {
    FIS_REG_H2D fis = {};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.command = ATA_CMD_WRITE_DMA_EXT;
    fis.device = 1 << 6; // LBA mode
    fis.c = 1;           // command

    // set LBA
    fis.lba0 = (lba >> 0) & 0xff;
    fis.lba1 = (lba >> 8) & 0xff;
    fis.lba2 = (lba >> 16) & 0xff;
    fis.lba3 = (lba >> 24) & 0xff;
    fis.lba4 = (lba >> 32) & 0xff;
    fis.lba5 = (lba >> 40) & 0xff;

    // set sector count
    fis.countl = sector_count & 0xff;
    fis.counth = (sector_count >> 8) & 0xff;

    return fis;
}

inline FIS_REG_H2D ata_cmd_flush_cache_ext() {
    FIS_REG_H2D fis = {};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.command = ATA_CMD_FLUSH_CACHE_EXT;
    fis.device = 0;
    fis.c = 1; // command

    return fis;
}

inline FIS_REG_H2D ata_cmd_flush_cache() {
    FIS_REG_H2D fis = {};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.command = ATA_CMD_FLUSH_CACHE;
    fis.device = 0;
    fis.c = 1; // command

    return fis;
}
