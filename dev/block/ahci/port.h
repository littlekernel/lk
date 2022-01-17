//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lk/cpp.h>
#include <sys/types.h>

#include "ahci.h"
#include "ahci_hw.h"

// per port AHCI object
class ahci_port {
public:
    ahci_port(ahci &a, uint num);
    ~ahci_port();

    DISALLOW_COPY_ASSIGN_AND_MOVE(ahci_port);

    status_t probe();
    status_t identify();

private:
    uint32_t read_port_reg(ahci_port_reg reg);
    void write_port_reg(ahci_port_reg reg, uint32_t val);

    int find_free_cmdslot();
    volatile ahci_cmd_table *cmd_table_ptr(uint cmd_slot);

    // members
    ahci &ahci_;
    uint num_;

    void *mem_region_ = nullptr;
    paddr_t mem_region_paddr_ = 0;
    volatile ahci_cmd_header *cmd_list_ = nullptr;
    volatile uint8_t *fis_ = nullptr;
    volatile ahci_cmd_table *cmd_table_ = nullptr;

    const size_t CMD_COUNT = 32; // number of active command slots
    const size_t PRD_PER_CMD = 16; // physical descriptors per command slot
    const size_t CMD_TABLE_ENTRY_SIZE =  sizeof(ahci_cmd_table) + sizeof(ahci_prd) * PRD_PER_CMD;
};

inline uint32_t ahci_port::read_port_reg(ahci_port_reg reg) {
    return ahci_.read_port_reg(num_, reg);
}

inline void ahci_port::write_port_reg(ahci_port_reg reg, uint32_t val) {
    ahci_.write_port_reg(num_, reg, val);
}

inline volatile ahci_cmd_table *ahci_port::cmd_table_ptr(uint cmd_slot) {
    return (volatile ahci_cmd_table *)((uintptr_t)cmd_table_+ CMD_TABLE_ENTRY_SIZE * cmd_slot);
}

