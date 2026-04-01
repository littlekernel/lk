//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <kernel/event.h>
#include <kernel/spinlock.h>
#include <lk/cpp.h>
#include <lib/bio.h>
#include <sys/types.h>

#include "ahci.h"
#include "ahci_hw.h"

class ahci_disk;

// per port AHCI object
class ahci_port final {
  public:
    ahci_port(ahci &a, uint index);
    ~ahci_port();

    DISALLOW_COPY_ASSIGN_AND_MOVE(ahci_port);

    handler_return irq_handler();

    status_t probe(ahci_disk **found_disk);

    status_t queue_command(const void *fis, size_t fis_len, void *buf, size_t buf_len, bool write, bool ncq, uint8_t tag, int *slot_out);
    status_t wait_for_completion(uint slot, uint32_t *error_status);

    // Register async callback for a pending command. Called after queue_command
    // returns a slot. bytes_to_transfer is the number of bytes to transfer
    // (returned as result on success).
    void register_async_callback(uint slot, bio_async_callback_t callback, bdev_t *bdev,
                                 void *callback_context, ssize_t bytes_to_transfer);

    // Tag allocation for NCQ
    int allocate_ncq_tag();
    void release_ncq_tag(uint8_t tag);

    auto index() const { return index_; }
    auto controller_unit() const { return ahci_.unit_num(); }
    bool supports_ncq() const { return (ahci_.get_capabilities() & (1U << 30)) != 0; }

    // constants
    static const size_t MAX_CMD_COUNT = 32;   // number of active command slots
    static const size_t PRD_PER_CMD = 16; // physical descriptors per command slot
    static const size_t CMD_TABLE_ENTRY_SIZE = sizeof(ahci_cmd_table) + sizeof(ahci_prd) * PRD_PER_CMD;
    static const size_t MAX_PRDT_RUN_LENGTH = 0x400000;  // 4MB AHCI PRDT limit

  private:
    struct async_cmd_info {
        bio_async_callback_t callback = nullptr;
        bdev_t *bdev = nullptr;
        void *callback_context = nullptr;
        ssize_t bytes_to_read_write = 0;  // bytes to transfer (for result on success)
    };

  private:
    uint32_t read_port_reg(ahci_port_reg reg);
    void write_port_reg(ahci_port_reg reg, uint32_t val);

    status_t find_free_cmdslot(uint *slot_out);
    volatile ahci_cmd_table *cmd_table_ptr(uint cmd_slot);

    bool is_command_queued(uint slot) {
        AutoSpinLock guard(&lock_);
        return (cmd_pending_ & (1U << slot)) != 0;
    }

    // members
    ahci &ahci_;
    uint index_;

    // per port spinlock
    spin_lock_t lock_ = SPIN_LOCK_INITIAL_VALUE;

    // actual number of command slots
    uint32_t command_slots_ = 0;

    // pending command bitmap
    uint32_t cmd_pending_ = 0;
    uint32_t ncq_active_ = 0;
    // bitmap of free NCQ tags (32 tags total)
    uint32_t free_ncq_tags_ = 0xffffffff;

    // async command tracking: indexed by command slot
    async_cmd_info async_cmds_[MAX_CMD_COUNT] = {};
    ssize_t cmd_results_[MAX_CMD_COUNT] = {};  // result (bytes read/written) for each slot

    event cmd_complete_event_[MAX_CMD_COUNT];

    volatile uint8_t *mem_region_ = nullptr;
    paddr_t mem_region_paddr_ = 0;
    volatile ahci_cmd_header *cmd_list_ = nullptr;
    volatile uint8_t *fis_ = nullptr;
    volatile ahci_cmd_table *cmd_table_ = nullptr;
};

inline uint32_t ahci_port::read_port_reg(ahci_port_reg reg) {
    return ahci_.read_port_reg(index_, reg);
}

inline void ahci_port::write_port_reg(ahci_port_reg reg, uint32_t val) {
    ahci_.write_port_reg(index_, reg, val);
}

inline volatile ahci_cmd_table *ahci_port::cmd_table_ptr(uint cmd_slot) {
    return (volatile ahci_cmd_table *)((uintptr_t)cmd_table_ + CMD_TABLE_ENTRY_SIZE * cmd_slot);
}
