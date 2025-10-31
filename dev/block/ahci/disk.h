//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lk/cpp.h>
#include <lk/list.h>
#include <sys/types.h>

#include "port.h"

class ahci_disk {
  public:
    explicit ahci_disk(ahci_port &p) : port_(p) {}

    ~ahci_disk() = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(ahci_disk);

    status_t identify();

    status_t read_sectors(uint64_t lba, void *buf, size_t buf_len);
    status_t write_sectors(uint64_t lba, const void *buf, size_t buf_len);

    // Must be visible to the ahci class for disk list management
    list_node node_ = LIST_INITIAL_CLEARED_VALUE;

    uint64_t total_size() const { return sector_count_ * logical_sector_size_; }

  private:
    ahci_port &port_;

    // drive parameters
    uint32_t logical_sector_size_ = 512;
    uint32_t physical_sector_size_ = 512;
    uint64_t sector_count_ = 0;

    bool supports_ncq_ = false;
    uint8_t ncq_queue_depth_ = 0;
};
