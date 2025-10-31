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

status_t ahci_disk::identify() {
    LTRACE_ENTRY;

    // TODO: move this buffer to be per-disk instead of static
    __ALIGNED(512)
    static uint16_t identify_data[256];
    FIS_REG_H2D fis = ata_cmd_identify();

    int slot;
    auto err = port_.queue_command(&fis, sizeof(fis), identify_data, sizeof(identify_data), false, &slot);
    if (err != NO_ERROR) {
        printf("ahci_disk::identify: queue_command failed: %d\n", err);
        return err;
    }

    // wait for it to complete
    err = port_.wait_for_completion(slot);
    if (err != NO_ERROR) {
        printf("ahci_disk::identify: wait_for_completion failed: %d\n", err);
        return err;
    }

    if (LOCAL_TRACE >= 2) {
        LTRACEF("identify data:\n");
        hexdump8(identify_data, sizeof(identify_data));
    }

    // copy the model string out, swapping each byte pair
    char model[20 * 2 + 1] = {};
    for (size_t i = 0; i < 20; i++) {
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
    sector_count_ = identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD] |
                    (static_cast<uint64_t>(identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD + 1]) << 16) |
                    (static_cast<uint64_t>(identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD + 2]) << 32) |
                    (static_cast<uint64_t>(identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_COUNT_QWORD + 3]) << 48);

    LTRACEF("logical sector count %#llx\n", sector_count_);

    // defaults to 512 bytes
    logical_sector_size_ = 512;
    physical_sector_size_ = 512;

    auto phys_to_logical_sector = identify_data[ATA_IDENTIFY_PHYS_TO_LOGICAL_SECTOR];
    // LTRACEF("phys size / logical size %#hx\n", identify_data[ATA_IDENTIFY_PHYS_TO_LOGICAL_SECTOR]);
    if (BITS(phys_to_logical_sector, 15, 14) == (1 << 14)) { // word 106 has valid info
        if (BIT(phys_to_logical_sector, 12)) {
            // logical sector size is specified in word 117..118
            logical_sector_size_ = identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_SIZE_DWORD] |
                                   (static_cast<uint32_t>(identify_data[ATA_IDENTIFY_LOGICAL_SECTOR_SIZE_DWORD + 1]) << 16);
        }

        // bits 3:0 have physical sector size in power of 2 times logical size
        physical_sector_size_ = (1U << BITS(phys_to_logical_sector, 3, 0)) * logical_sector_size_;
    }

    LTRACEF("logical sector size %#x\n", logical_sector_size_);
    LTRACEF("physical sector size %#x\n", physical_sector_size_);
    LTRACEF("total size %#llx\n", total_size());

    // detect if the drive supports NCQ
    uint ncq_queue_depth = (identify_data[75] & 0xff) + 1;
    if (ncq_queue_depth >= 1) {
        LTRACEF("drive supports NCQ with queue depth %u\n", ncq_queue_depth);
        supports_ncq_ = true;
        ncq_queue_depth_ = ncq_queue_depth;
    }

    return NO_ERROR;
}

status_t ahci_disk::read_sectors(uint64_t lba, void *buf, size_t buf_len) {
    LTRACEF("lba %#llx buf %p len %zu\n", lba, buf, buf_len);

    if (buf_len % logical_sector_size_ != 0) {
        return ERR_INVALID_ARGS;
    }

    size_t sector_count = buf_len / logical_sector_size_;
    if (sector_count == 0 || sector_count > 0xffff) {
        return ERR_INVALID_ARGS;
    }

    FIS_REG_H2D fis = ata_cmd_read_dma_ext(lba, sector_count);
    hexdump8(&fis, sizeof(fis));

    int slot;
    auto err = port_.queue_command(&fis, sizeof(fis),
                                   buf, buf_len,
                                   false, &slot);
    if (err != NO_ERROR) {
        return err;
    }

    // wait for it to complete
    err = port_.wait_for_completion(slot);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}

status_t ahci_disk::write_sectors(uint64_t lba, const void *buf, size_t buf_len) {
    LTRACEF("lba %#llx buf %p len %zu\n", lba, buf, buf_len);

    if (buf_len % logical_sector_size_ != 0) {
        return ERR_INVALID_ARGS;
    }

    size_t sector_count = buf_len / logical_sector_size_;
    if (sector_count == 0 || sector_count > 0xffff) {
        return ERR_INVALID_ARGS;
    }

    FIS_REG_H2D fis = ata_cmd_write_dma_ext(lba, sector_count);
    hexdump8(&fis, sizeof(fis));

    int slot;
    auto err = port_.queue_command(&fis, sizeof(fis),
                                   (void *)buf, buf_len,
                                   true, &slot);
    if (err != NO_ERROR) {
        return err;
    }

    // wait for it to complete
    err = port_.wait_for_completion(slot);
    if (err != NO_ERROR) {
        return err;
    }

    return NO_ERROR;
}
