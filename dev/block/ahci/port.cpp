//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include "port.h"

#include <lk/bits.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <kernel/vm.h>
#include <kernel/thread.h>
#include <string.h>

#include "ata.h"

#define LOCAL_TRACE 1

ahci_port::ahci_port(ahci &a, uint num) : ahci_(a), num_(num) {}
ahci_port::~ahci_port() {
    if (mem_region_) {
        vmm_free_region(vmm_get_kernel_aspace(), (vaddr_t)mem_region_);
    }
}

status_t ahci_port::probe() {
    // check if drive is present
    auto ssts = read_port_reg(ahci_port_reg::PxSSTS);
    if (BITS(ssts, 3, 0) != 3) { // check SSTS.DET == 3 (device present and phy comm established)
        return ERR_NOT_FOUND;
    }
    if (BITS_SHIFT(ssts, 11, 8) != 1) { // check SSTS.IPM == 1 (interface in active state)
        return ERR_NOT_FOUND;
    }
    dprintf(INFO, "ahci%d port %u: device present and interface in active state\n",
            ahci_.get_unit_num(), num_);

    auto sig = read_port_reg(ahci_port_reg::PxSIG);
    LTRACEF("port %u: sig %#x\n", num_, sig);

    if (sig != 0x101) { // we can only handle SATA drives now
        TRACEF("skipping unhandled signature %#x\n", sig);
        return ERR_NOT_FOUND;
    }

    LTRACEF("port %u: PxCLB %#x\n", num_, read_port_reg(ahci_port_reg::PxCLB));
    LTRACEF("port %u: PxCMD %#x\n", num_, read_port_reg(ahci_port_reg::PxCMD));

    // stop the port so we can reset addresses
    auto cmd_reg = read_port_reg(ahci_port_reg::PxCMD);
    cmd_reg &= ~((1<<4) | // clear CMD.FRE (fis receive enable)
            (1<<0));  // clear CMD.ST (start)
    write_port_reg(ahci_port_reg::PxCMD, cmd_reg);
    // TODO: wait for CMD.FR to stop

    // allocate a block of contiguous memory for
    // 32 command list heads (32 * 0x20)
    // a FIS struct (256 bytes)
    // 32 command tables with 16 PRDTs per
    const size_t size = (CMD_COUNT * sizeof(ahci_cmd_header)) + 256 +
        (CMD_COUNT * CMD_TABLE_ENTRY_SIZE);

    // allocate a contiguous block of ram
    char str[32];
    snprintf(str, sizeof(str), "ahci%d.%u cmd/fis", ahci_.get_unit_num(), num_);
    status_t err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, size,
                             (void **)&mem_region_, 0, /* vmm_flags */ 0,
                             ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    memset(mem_region_, 0, size);
    mem_region_paddr_ = vaddr_to_paddr(mem_region_);

    LTRACEF("cmd_list/fis mapped to %p, pa %#lx\n", mem_region_, mem_region_paddr_);

    // carve up the pointers into this space
    cmd_list_ = (volatile ahci_cmd_header *)mem_region_;
    fis_ = (volatile uint8_t *)((uintptr_t)mem_region_ + 32 * sizeof(ahci_cmd_header));
    cmd_table_ = (volatile ahci_cmd_table *)(fis_ + 256);

    LTRACEF("command list at %p, FIS at %p, per command table at %p\n",
            cmd_list_, fis_, cmd_table_);

    // set the AHCI port to point to the command header and global fis
    write_port_reg(ahci_port_reg::PxCLB, vaddr_to_paddr((void *)cmd_list_));
    write_port_reg(ahci_port_reg::PxCLBU, vaddr_to_paddr((void *)cmd_list_) >> 32);
    write_port_reg(ahci_port_reg::PxFB, vaddr_to_paddr((void *)fis_));
    write_port_reg(ahci_port_reg::PxFBU, vaddr_to_paddr((void *)fis_) >> 32);

    // set up the command headers
    auto cmd_table_pa = vaddr_to_paddr((void *)cmd_table_);
    for (auto i = 0; i < 32; i++) {
        volatile auto *cmd = &cmd_list_[i];

        // point the cmd header at the corresponding cmd table
        cmd->ctba = (cmd_table_pa + sizeof(ahci_cmd_table) * i) & 0xffffffff;
        cmd->ctbau = (cmd_table_pa + sizeof(ahci_cmd_table) * i) >> 32;
    }

    // restart the port
    cmd_reg |= (1<<4); // set CMD.FRE (fis receive enable)
    write_port_reg(ahci_port_reg::PxCMD, cmd_reg);
    cmd_reg |= (1<<0); // set CMD.ST (start)
    write_port_reg(ahci_port_reg::PxCMD, cmd_reg);

    return NO_ERROR;
}

int ahci_port::find_free_cmdslot() {
    uint32_t all_slots = read_port_reg(ahci_port_reg::PxSACT) |
                         read_port_reg(ahci_port_reg::PxCI);

    LTRACEF("all_slots %#x\n", all_slots);

    if (unlikely(all_slots == 0xffffffff)) {
        // all slots are full
        return -1;
    }

    int avail = __builtin_ffs(~all_slots) - 1;
    LTRACEF("avail %u\n", avail);

    return avail;
}

status_t ahci_port::identify() {
    auto slot = find_free_cmdslot();

    LTRACEF("slot %u\n", slot);

    auto *cmd = &cmd_list_[slot];
    cmd->cmd = (sizeof(FIS_REG_H2D) / sizeof(uint32_t)) | (0<<6); // command fis size in words, read from device
    cmd->prdtl = 1; // 1 prdt

    auto *cmd_table = cmd_table_ptr(slot);
    __ALIGNED(512) static uint8_t identify_data[512];

    auto *prdt = &cmd_table->pdrt[0];
    prdt->dba  = vaddr_to_paddr(identify_data);
    prdt->dbau = vaddr_to_paddr(identify_data) >> 32;
    prdt->byte_count_ioc = (512 - 1) | (1U<<31); // byte count, interrupt on completion

    FIS_REG_H2D fis = {};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.command = ATA_CMD_IDENTIFY;
    fis.device = 0;
    fis.c = 1;

    memcpy((void *)cmd_table->cfis, &fis, sizeof(fis));

    //LTRACEF("cmd_table %p\n", cmd_table);
    //hexdump((const void *)cmd_table, sizeof(*cmd_table) + CMD_TABLE_ENTRY_SIZE);

    // kick the command
    write_port_reg(ahci_port_reg::PxCI, (1U << slot));

    // wait for it to complete

    thread_sleep(1000);

    LTRACEF("identify data:\n");
    hexdump8(identify_data, sizeof(identify_data));

    return NO_ERROR;
}
