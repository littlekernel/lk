//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include "disk.h"

#include <lk/bits.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdint.h>
#include <string.h>

#include "ata.h"

#define LOCAL_TRACE 1

// offsets in the 256 word (2 byte word) IDENTIFY structure
enum ata_identify_words {
    ATA_IDENTIFY_MODEL_NUMBER = 27, // 40 bytes
    ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD = 100, // 4 words of logical sector count
    ATA_IDENTIFY_PHYS_TO_LOGICAL_SECTOR = 106, // phys size / logical size
    ATA_IDENTIFY_LOGICAL_SECTOR_SIZE_DWORD = 117, // dword of logical sector size
};

ahci_disk::ahci_disk(ahci_port &p) : port_(p) {
}

ahci_disk::~ahci_disk() = default;

status_t ahci_disk::identify() {
    LTRACE_ENTRY;

    __ALIGNED(512) static uint16_t identify_data[256];
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

    char model[20*2 + 1] = {};
    for (auto i = 0; i < 20; i++) {
        model[i * 2] = identify_data[ATA_IDENTIFY_MODEL_NUMBER + i] >> 8;
        model[i * 2 + 1] = identify_data[ATA_IDENTIFY_MODEL_NUMBER + i] & 0xff;
    }
    LTRACEF("model '%s'\n", model);

    // assumes LBA48
    bool lba48 = identify_data[83] & (1 << 10);
    if (!lba48) {
        printf("AHCI: LBA48 required, aborting\n");
        return ERR_NOT_SUPPORTED;
    }

    // sector count is 4 words at offset 100
    uint64_t sector_count = identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD] |
                            ((uint64_t)identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD + 1] << 16) |
                            ((uint64_t)identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD + 2] << 32) |
                            ((uint64_t)identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD + 3] << 48);

    LTRACEF("logical sector count %#llx\n", sector_count);

    // defaults to 512 bytes
    uint32_t logical_sector_size = 512;
    uint32_t physical_sector_size = 512;

    auto phys_to_logical_sector = identify_data[ATA_IDENTIFY_PHYS_TO_LOGICAL_SECTOR];
    //LTRACEF("phys size / logical size %#hx\n", identify_data[ATA_IDENTIFY_PHYS_TO_LOGICAL_SECTOR]);
    if (BITS(phys_to_logical_sector, 15, 14) == (1 << 14)) { // word 106 has valid info
        if (BIT(phys_to_logical_sector, 12)) {
            // logical sector size is specified in word 117..118
            logical_sector_size = identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_SIZE_DWORD] |
                                  ((uint32_t)identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_SIZE_DWORD + 1] << 16);
        }

        // bits 3:0 have physical sector size in power of 2 times logical size
        physical_sector_size = (1U << BITS(phys_to_logical_sector, 3, 0)) * logical_sector_size;
    }

    LTRACEF("logical sector size %#x\n", logical_sector_size);
    LTRACEF("physical sector size %#x\n", physical_sector_size);
    LTRACEF("total size %#llx\n", sector_count * logical_sector_size);

    return NO_ERROR;
}

