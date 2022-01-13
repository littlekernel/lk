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
#include <string.h>

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
    dprintf(INFO, "ahci%d port %u: device present and interface in active state\n", ahci_.get_unit_num(), num_);

    auto sig = read_port_reg(ahci_port_reg::PxSIG);
    LTRACEF("port %u: sig %#x\n", num_, sig);

    if (sig != 0x101) { // we can only handle SATA drives now
        TRACEF("skipping unhandled signature %#x\n", sig);
        return ERR_NOT_FOUND;
    }

    LTRACEF("port %u: PxCLB %#x\n", num_, read_port_reg(ahci_port_reg::PxCLB));

    // allocate a block of contiguous memory for
    // 32 command list heads (32 * 0x20)
    // a FIS struct (256 bytes)
    // 32 command tables with 16 PRDTs per
    const size_t size = (CMD_COUNT * sizeof(ahci_cmd_header)) + 256 +
        (CMD_COUNT * (sizeof(ahci_cmd_table) + sizeof(ahci_prd) * PRD_PER_CMD));

    // allocate a 0x500 byte block of ram for the command list * 32 and FIS structure
    char str[32];
    snprintf(str, sizeof(str), "ahci%d.%u cmd/fis", ahci_.get_unit_num(), num_);
    status_t err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, size,
                             (void **)&mem_region_, 0, /* vmm_flags */ 0,
                             ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    memset(mem_region_, 0, size);

    LTRACEF("cmd_list/fis mapped to %p\n", mem_region_);

    cmd_list_ = (volatile ahci_cmd_header *)mem_region_;
    fis_ = (volatile uint8_t *)((uintptr_t)mem_region_ + 32 * sizeof(ahci_cmd_header));
    cmd_table_ = (volatile ahci_cmd_table *)(fis_ + 256);

    LTRACEF("command list at %p, FIS at %p, per command table at %p\n",
            cmd_list_, fis_, cmd_table_);

    return NO_ERROR;
}

