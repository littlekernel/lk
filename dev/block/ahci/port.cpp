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
#include "disk.h"

#define LOCAL_TRACE 1

ahci_port::ahci_port(ahci &a, uint num) : ahci_(a), num_(num) {
    for (auto &e : cmd_complete_event_) {
        event_init(&e, false, EVENT_FLAG_AUTOUNSIGNAL);
    }
}

ahci_port::~ahci_port() {
    if (mem_region_) {
        vmm_free_region(vmm_get_kernel_aspace(), (vaddr_t)mem_region_);
    }
    for (auto &e : cmd_complete_event_) {
        event_destroy(&e);
    }
}

status_t ahci_port::probe(ahci_disk **found_disk) {
    // mask all IRQS on this port regardless if we want to use it
    write_port_reg(ahci_port_reg::PxIE, 0);

    // clear any pending bits
    write_port_reg(ahci_port_reg::PxIS, 0xffffffff);

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
    write_port_reg(ahci_port_reg::PxFB, vaddr_to_paddr((void *)fis_));
#if __INTPTR_WIDTH__ == 64
    write_port_reg(ahci_port_reg::PxCLBU, vaddr_to_paddr((void *)cmd_list_) >> 32);
    write_port_reg(ahci_port_reg::PxFBU, vaddr_to_paddr((void *)fis_) >> 32);
#else
    write_port_reg(ahci_port_reg::PxCLBU, 0);
    write_port_reg(ahci_port_reg::PxFBU, 0);
#endif

    // set up the command headers
    auto cmd_table_pa = vaddr_to_paddr((void *)cmd_table_);
    for (auto i = 0; i < 32; i++) {
        volatile auto *cmd = &cmd_list_[i];

        // point the cmd header at the corresponding cmd table
        cmd->ctba = (cmd_table_pa + sizeof(ahci_cmd_table) * i) & 0xffffffff;
#if __INTPTR_WIDTH__ == 64
        cmd->ctbau = (cmd_table_pa + sizeof(ahci_cmd_table) * i) >> 32;
#else
        cmd->ctbau = 0;
#endif
    }

    // restart the port
    cmd_reg |= (1<<4); // set CMD.FRE (fis receive enable)
    write_port_reg(ahci_port_reg::PxCMD, cmd_reg);
    cmd_reg |= (1<<0); // set CMD.ST (start)
    write_port_reg(ahci_port_reg::PxCMD, cmd_reg);

    // unmask some irqs on this port
    // TODO: unmask more if needed
    uint32_t ie = (1U << 5) | // Descriptor Processed (DPS)
                  (1U << 3) | // Set device bits interrupt (SDBS)
                  (1U << 2) | // DMA setup FIS (DSS)
                  (1U << 1) | // PIO setup FIS (PSS)
                  (1U << 0);  // Device to Host Register FIS (DHRS)
    write_port_reg(ahci_port_reg::PxIE, ie);

    // we found a disk above, create an object and pass it back
    auto *disk = new ahci_disk(*this);
    *found_disk = disk;

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

status_t ahci_port::queue_command(const void *fis, size_t fis_len, void *buf, size_t buf_len, bool write, int *slot_out) {
    LTRACEF("fis %p len %zu buf %p len %zu write %d\n", fis, fis_len, buf, buf_len, write);

    DEBUG_ASSERT(fis);
    DEBUG_ASSERT(fis_len > 0 && fis_len <= 64 && IS_ALIGNED(fis_len, 4));
    DEBUG_ASSERT(buf || buf_len == 0);

    AutoSpinLock guard(&lock_);

    auto slot = find_free_cmdslot();

    LTRACEF("slot %u\n", slot);

    // clear interrupt status for this port
    write_port_reg(ahci_port_reg::PxIS, 0xf);

    auto *cmd_table = cmd_table_ptr(slot);

    // set up physical descriptor of a run of memory
    // XXX for now assume single run
    auto *prdt = &cmd_table->pdrt[0];
    auto buf_pa = vaddr_to_paddr(buf);
    prdt->dba  = buf_pa;
#if __INTPTR_WIDTH__ == 64
    prdt->dbau = buf_pa >> 32;
#else
    prdt->dbau = 0;
#endif
    prdt->byte_count_ioc = (buf_len - 1) | (1U<<31); // byte count, interrupt on completion

    // copy command into the command table
    // TODO: replace with wordwise copy
    memcpy((void *)cmd_table->cfis, fis, fis_len);

    // set up the command header
    auto *cmd = &cmd_list_[slot];
    cmd->cmd = (fis_len / sizeof(uint32_t)) | (write ? (1<<6) : (0<<6)); // command fis size in words, read/write from device
    cmd->prdtl = 1; // 1 prdt

    //LTRACEF("cmd_table %p\n", cmd_table);
    //hexdump((const void *)cmd_table, sizeof(*cmd_table) + CMD_TABLE_ENTRY_SIZE);

    // TODO: barrier here
    // rmb();

    LTRACEF("IS %#x (before kick)\n", read_port_reg(ahci_port_reg::PxIS));

    cmd_pending_ |= (1U << slot);

    // kick the command
    write_port_reg(ahci_port_reg::PxCI, (1U << slot)); // TODO: RMW?

    *slot_out = slot;

    return NO_ERROR;
}

status_t ahci_port::wait_for_completion(int slot) {
    DEBUG_ASSERT(slot >= 0 && slot < (int)CMD_COUNT);

    auto err = event_wait(&cmd_complete_event_[slot]);

    return err;
}

handler_return ahci_port::irq_handler() {
    LTRACE_ENTRY;

    AutoSpinLockNoIrqSave guard(&lock_);

    const auto raw_is = read_port_reg(ahci_port_reg::PxIS);
    const auto is = raw_is & read_port_reg(ahci_port_reg::PxIE); // filter by things we're masking

    LTRACEF("raw is %#x is %#x\n", raw_is, is);

    // see if any commands completed
    const auto ci = read_port_reg(ahci_port_reg::PxCI);
    auto cmd_complete_bitmap = cmd_pending_ & ~ci;

    LTRACEF("command complete bitmap %#x\n", cmd_complete_bitmap);

    handler_return ret = INT_NO_RESCHEDULE;
    while (cmd_complete_bitmap != 0) {
        const size_t cmd_slot = __builtin_ctz(cmd_complete_bitmap);

        DEBUG_ASSERT(cmd_slot < CMD_COUNT);

        LTRACEF("slot %zu completed\n", cmd_slot);

        // this slot completed
        event_signal(&cmd_complete_event_[cmd_slot], false);
        ret = INT_RESCHEDULE;

        // mark the command as not pending anymore
        cmd_pending_ &= ~(1U << cmd_slot);

        // move to the next pending slot (if any)
        cmd_complete_bitmap &= ~(1U << cmd_slot);
    }

    // ack everything for now
    write_port_reg(ahci_port_reg::PxIS, is);

    return ret;
}
