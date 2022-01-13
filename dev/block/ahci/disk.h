//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lib/bio.h>
#include <lk/cpp.h>
#include <lk/list.h>
#include <sys/types.h>
#include <type_traits>

#include "port.h"

struct ahci_disk_bio_handler;

class ahci_disk final {
  public:
    explicit ahci_disk(ahci_port &p) : port_(p) {}
    ~ahci_disk();

    DISALLOW_COPY_ASSIGN_AND_MOVE(ahci_disk);

    status_t identify();

    status_t read_sectors(uint64_t lba, void *buf, size_t buf_len);
    status_t write_sectors(uint64_t lba, const void *buf, size_t buf_len);

    uint64_t total_size() const { return sector_count_ * logical_sector_size_; }
    uint32_t logical_sector_size() const { return logical_sector_size_; }
    uint32_t physical_sector_size() const { return physical_sector_size_; }
    uint64_t sector_count() const { return sector_count_; }

    char *bio_name(char *out, size_t out_size) const {
        snprintf(out, out_size, "ahci%u.%u", port_.controller_unit(), port_.index());
        return out;
    }

  private:
    ahci_port &port_;

    // wrapper class around the bio block device to translate bio
    // hooks back to ahci_disk methods and handle translating the
    // bdev pointer back to the ahci_disk pointer.
    struct bio_handler final {
        explicit bio_handler(ahci_disk *d) : disk_(d) {}
        ~bio_handler();

        DISALLOW_COPY_ASSIGN_AND_MOVE(bio_handler);

        status_t initialize_bdev();

        static bio_handler *bdev_to_handler(bdev_t *bdev) {
            return reinterpret_cast<bio_handler *>(bdev);
        }
        static ahci_disk *bdev_to_disk(bdev_t *bdev) {
            return reinterpret_cast<bio_handler *>(bdev)->disk_;
        }

        bdev_t bdev_;
        ahci_disk *disk_;
        bool registered_ = false;
    } bio_handler_ {this};;

    static_assert(std::is_standard_layout<bio_handler>::value, "");
    static_assert(offsetof(bio_handler, bdev_) == 0, "");

    // Must be visible to the ahci class for disk list management
    list_node node_ = LIST_INITIAL_CLEARED_VALUE;
    friend class ahci;

    // drive parameters
    uint32_t logical_sector_size_ = 512;
    uint32_t physical_sector_size_ = 512;
    uint64_t sector_count_ = 0;

    bool supports_ncq_ = false;
    uint8_t ncq_queue_depth_ = 0;

    // mostly static stuff
    char model_[41]{};
    char serial_[21]{};
    char firmware_[9]{};
};
