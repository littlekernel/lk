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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ata.h"

#define LOCAL_TRACE 0

namespace {

// copy and swap each pair of bytes from the identify data into a string,
// trimming trailing spaces
void copy_identify_string(char *out, size_t out_size, const uint16_t *in, size_t in_words) {
    size_t len = MIN(in_words * 2, out_size - 1);
    DEBUG_ASSERT(out_size >= in_words * 2 + 1);

    for (size_t i = 0; i < in_words; i++) {
        out[i * 2] = in[i] >> 8;
        out[i * 2 + 1] = in[i] & 0xff;
    }
    out[len] = '\0';

    // trim trailing spaces
    for (ssize_t i = len - 1; i >= 0; i--) {
        if (out[i] == ' ') {
            out[i] = '\0';
        } else {
            break;
        }
    }
}

} // namespace

ahci_disk::~ahci_disk() {
    // the bio handler will unregister itself in its destructor
}

status_t ahci_disk::identify() {
    LTRACE_ENTRY;

    uint16_t identify_data[256];
    FIS_REG_H2D fis = ata_cmd_identify();

    // issue the identify command
    int slot;
    auto err = port_.queue_command(&fis, sizeof(fis), identify_data, sizeof(identify_data), false, &slot);
    if (err != NO_ERROR) {
        printf("ahci_disk::identify: queue_command failed: %d\n", err);
        return err;
    }

    // wait for it to complete
    err = port_.wait_for_completion(slot, nullptr);
    if (err != NO_ERROR) {
        printf("ahci_disk::identify: wait_for_completion failed: %d\n", err);
        return err;
    }

    if (LOCAL_TRACE >= 2) {
        LTRACEF("identify data:\n");
        hexdump8(identify_data, sizeof(identify_data));
    }

    // copy some of the strings out
    copy_identify_string(model_, sizeof(model_), identify_data + ATA_IDENTIFY_MODEL_NUMBER, 20);
    copy_identify_string(serial_, sizeof(serial_), identify_data + ATA_IDENTIFY_SERIAL_NUMBER, 10);
    copy_identify_string(firmware_, sizeof(firmware_), identify_data + ATA_IDENTIFY_FIRMWARE_REVISION, 4);
    LTRACEF("model '%s'\n", model_);
    LTRACEF("serial '%s'\n", serial_);
    LTRACEF("firmware '%s'\n", firmware_);

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

    dprintf(INFO, "ahci%d: detected drive model '%s' serial '%s' size %#llx bytes\n",
            port_.controller_unit(), model_, serial_, total_size());

    // detect if the drive supports NCQ
    uint ncq_queue_depth = (identify_data[75] & 0xff) + 1;
    if (ncq_queue_depth >= 1) {
        LTRACEF("drive supports NCQ with queue depth %u\n", ncq_queue_depth);
        supports_ncq_ = true;
        ncq_queue_depth_ = ncq_queue_depth;
    }

    // char test_data[512];
    // status_t error = read_sectors(0, test_data, 512);
    // if (error != NO_ERROR) {
    //     printf("ahci_disk::identify: read_sectors failed: %d\n", error);
    //     return error;
    // }
    // hexdump8(test_data, 512);

    // construct a wrapper class around the bio block device
    err = bio_handler_.initialize_bdev();
    if (err != NO_ERROR) {
        printf("ahci_disk::identify: initialize_bdev failed: %d\n", err);
        return err;
    }

    return NO_ERROR;
}

// TODO: refactor the read/write routines below to use a command queuing layer

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
    // hexdump8(&fis, sizeof(fis));

    int slot;
    auto err = port_.queue_command(&fis, sizeof(fis),
                                   buf, buf_len,
                                   false, &slot);
    if (err != NO_ERROR) {
        return err;
    }

    // wait for it to complete
    uint32_t error_status;
    err = port_.wait_for_completion(slot, &error_status);
    if (err != NO_ERROR) {
        return err;
    }

    // check the status bits
    if (error_status & (1U << 0)) { // check ERR bit
        printf("ahci_disk::read_sectors: device reported error, status %#x\n", error_status);
        return ERR_IO;
    }

    LTRACEF("error_status %#x\n", error_status);

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
    // hexdump8(&fis, sizeof(fis));

    int slot;
    auto err = port_.queue_command(&fis, sizeof(fis),
                                   (void *)buf, buf_len,
                                   true, &slot);
    if (err != NO_ERROR) {
        return err;
    }

    // wait for it to complete
    uint32_t error_status;
    err = port_.wait_for_completion(slot, &error_status);
    if (err != NO_ERROR) {
        return err;
    }

    // check the status bits
    if (error_status & (1U << 0)) { // check ERR bit
        printf("ahci_disk::read_sectors: device reported error, status %#x\n", error_status);
        return ERR_IO;
    }

    LTRACEF("error_status %#x\n", error_status);

    return NO_ERROR;
}

// block io interface
ahci_disk::bio_handler::~bio_handler() {
    if (registered_) {
        bio_unregister_device(&bdev_);
        registered_ = false;
    }
}

status_t ahci_disk::bio_handler::initialize_bdev() {
    // construct the bio name of this disk
    char bdev_name[32];
    disk_->bio_name(bdev_name, sizeof(bdev_name));

    // initialize the block device in the bio layer
    bio_initialize_bdev(&bdev_,
                         bdev_name, // name will be filled in by bio layer
                         disk_->logical_sector_size(),
                         static_cast<bnum_t>(disk_->sector_count()),
                         0, // no erase geometry
                         nullptr,
                         BIO_FLAGS_NONE);

    // register some block device hooks
    bdev_.read_block = [](bdev_t *bdev, void *buf, bnum_t block, uint count) -> ssize_t {
        ahci_disk *disk = bdev_to_disk(bdev);
        uint64_t lba = static_cast<uint64_t>(block);
        size_t len = static_cast<size_t>(count) * disk->logical_sector_size();
        return disk->read_sectors(lba, buf, len) == NO_ERROR ? static_cast<ssize_t>(len) : ERR_IO;
    };
    bdev_.write_block = [](bdev_t *bdev, const void *buf, bnum_t block, uint count) -> ssize_t {
        ahci_disk *disk = bdev_to_disk(bdev);
        uint64_t lba = static_cast<uint64_t>(block);
        size_t len = static_cast<size_t>(count) * disk->logical_sector_size();
        return disk->write_sectors(lba, buf, len) == NO_ERROR ? static_cast<ssize_t>(len) : ERR_IO;
    };
    bdev_.close = [](bdev_t *bdev) {
        bio_handler *handler = bdev_to_handler(bdev);
        handler->registered_ = false;
    };
    // TODO: handle async read/write

    dprintf(INFO, "ahci%d: registering block device '%s'\n",
            disk_->port_.controller_unit(), bdev_name);

    // register the block device
    bio_register_device(&bdev_);
    registered_ = true;

    return NO_ERROR;
}
