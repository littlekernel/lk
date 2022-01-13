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

#define LOCAL_TRACE 1

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

    // allocate a 0x500 byte block of ram for the command list * 32 and FIS structure
    char str[32];
    snprintf(str, sizeof(str), "ahci%d.%u cmd/fis", ahci_.get_unit_num(), num_);
    status_t err = vmm_alloc(vmm_get_kernel_aspace(), str, 0x400 + 0x100, 
                             (void **)&cmd_list_, 0, /* vmm_flags */ 0,
                             ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }

    LTRACEF("cmd_list/fis mapped to %p\n", cmd_list_);

    static_assert(sizeof(ahci_cmd_list) * 32 == 0x400, "");
    fis_ = (volatile uint8_t *)((uintptr_t)cmd_list_ + 32 * sizeof(ahci_cmd_list));

    return NO_ERROR;
}

