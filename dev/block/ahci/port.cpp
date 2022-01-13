//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include "port.h"

#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lk/bits.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <string.h>

#include "disk.h"

#define LOCAL_TRACE 0

ahci_port::ahci_port(ahci &a, uint num) : ahci_(a), index_(num) {
    for (auto &e : cmd_complete_event_) {
        event_init(&e, false, 0);
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
    dprintf(INFO, "ahci%d port %u: ssts %#x device present and interface in active state\n",
            ahci_.unit_num(), index_, ssts);

    auto sig = read_port_reg(ahci_port_reg::PxSIG);
    LTRACEF("port %u: sig %#x\n", index_, sig);

    // if sig is all 1s then it hasn't been scanned yet, so assume it's a disk.
    // Otherwise look for a few known non-disk signatures and skip them.
    if (sig != 0xffffffff &&
        (sig == 0xeb140101 ||  // sata optical drive
         sig == 0xc33c0101 ||  // sata enclosure
         sig == 0x96690101)) { // sata port multiplier
        LTRACEF("skipping unhandled signature %#x\n", sig);
        return ERR_NOT_FOUND;
    }

    LTRACEF("port %u: PxCLB %#x\n", index_, read_port_reg(ahci_port_reg::PxCLB));
    LTRACEF("port %u: PxCMD %#x\n", index_, read_port_reg(ahci_port_reg::PxCMD));

    // stop the port so we can reset addresses
    auto cmd_reg = read_port_reg(ahci_port_reg::PxCMD);
    cmd_reg &= ~((1 << 4) | // clear CMD.FRE (fis receive enable)
                 (1 << 0)); // clear CMD.ST (start)
    write_port_reg(ahci_port_reg::PxCMD, cmd_reg);
    // wait for CMD.FR to stop
    while (read_port_reg(ahci_port_reg::PxCMD) & (1 << 14)) {
        thread_yield();
    }
    // wait for CMD.CR to stop
    while (read_port_reg(ahci_port_reg::PxCMD) & (1 << 15)) {
        thread_yield();
    }

    // allocate a block of contiguous memory for
    // up to 32 command list heads (32 * 0x20)
    // a FIS struct (256 bytes)
    // up to 32 command tables with 16 PRDTs per
    command_slots_ = ahci_.num_command_slots_per_port();
    const size_t size = (command_slots_ * sizeof(ahci_cmd_header)) + 256 +
                        (command_slots_ * CMD_TABLE_ENTRY_SIZE);
    LTRACEF("num command slots %u\n", command_slots_);
    LTRACEF("allocating %#zx bytes for command list / FIS / command tables\n", size);

    // allocate a contiguous block of ram
    char str[32];
    snprintf(str, sizeof(str), "ahci%d.%u cmd/fis", ahci_.unit_num(), index_);
    status_t err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, size,
                                        (void **)&mem_region_, 0, /* vmm_flags */ 0,
                                        ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    memset(const_cast<uint8_t *>(mem_region_), 0, size);
    mem_region_paddr_ = vaddr_to_paddr((void *)mem_region_);

    LTRACEF("cmd_list/fis mapped to %p, pa %#lx\n", mem_region_, mem_region_paddr_);

    // carve up the pointers into this space
    cmd_list_ = reinterpret_cast<volatile ahci_cmd_header *>(mem_region_);
    fis_ = mem_region_ + command_slots_ * sizeof(ahci_cmd_header);
    cmd_table_ = reinterpret_cast<volatile ahci_cmd_table *>(fis_ + 256);

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
    for (uint i = 0; i < command_slots_; i++) {
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
    cmd_reg |= (1 << 4); // set CMD.FRE (fis receive enable)
    write_port_reg(ahci_port_reg::PxCMD, cmd_reg);
    cmd_reg |= (1 << 0); // set CMD.ST (start)
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

status_t ahci_port::find_free_cmdslot(uint *slot_out) {
    DEBUG_ASSERT(spin_lock_held(&lock_));

    uint32_t all_slots = read_port_reg(ahci_port_reg::PxSACT) |
                         read_port_reg(ahci_port_reg::PxCI);

    LTRACEF_LEVEL(2, "all_slots %#x\n", all_slots);

    if (unlikely(all_slots == 0xffffffff)) {
        // all slots are full
        return ERR_NOT_FOUND;
    }

    // mask out all the bits for commands that are still pending
    all_slots |= cmd_pending_;

    uint avail = __builtin_clz(~all_slots);
    LTRACEF_LEVEL(2, "avail %u\n", avail);

    *slot_out = avail;
    return NO_ERROR;

    // static int next_slot = 0;
    // return next_slot++ % command_slots_;
}

struct ahci_mem_run {
    paddr_t address;
    size_t length;
};

namespace {

// convert a virtual buffer into a list of physical runs suitable for programming into AHCI PRDT entries.
// TODO: consider moving into some sort of shared library if needed elsewhere.
status_t virtual_to_pa_runs(const void *buf, const size_t len,
                            struct ahci_mem_run *runs,
                            size_t *run_count) {
    const size_t max_runs = *run_count;
    *run_count = 0;

    const uint8_t *ptr = static_cast<const uint8_t *>(buf);

    LTRACEF("buf %p len %zu\n", buf, len);

    size_t remaining = len;
    size_t index = 0;
    while (remaining > 0) {
        const paddr_t pa = vaddr_to_paddr((void *)ptr);
        if (pa == 0) {
            LTRACEF("virtual_to_pa_runs: could not map va %p to pa\n", ptr);
            return ERR_INVALID_ARGS;
        }
        const size_t run_len = MIN(PAGE_SIZE - (pa & (PAGE_SIZE - 1)), remaining);

        // see if we can merge with the previous run
        if (index > 0 &&
            runs[index - 1].address + runs[index - 1].length == pa &&
            runs[index - 1].length + run_len <= ahci_port::MAX_PRDT_RUN_LENGTH) {
            runs[index - 1].length += run_len;
            LTRACEF_LEVEL(2, "  run %zu: addr %#lx len %zu - merged\n", index - 1, runs[index - 1].address, runs[index - 1].length);
        } else {
            // set up a new run
            if (index >= max_runs) {
                return ERR_NOT_ENOUGH_BUFFER;
            }
            runs[index].address = pa;
            runs[index].length = run_len;
            LTRACEF_LEVEL(2, "  run %zu: addr %#lx len %zu - new\n", index, runs[index].address, runs[index].length);
            index++;
        }

        ptr += run_len;
        remaining -= run_len;
    }
    *run_count = index;

    return NO_ERROR;
}

} // namespace

// Queue a command to the AHCI port, finding a slot, setting up the PRDT entries, and kicking the command engine.
// Returns the slot number used in slot_out.
status_t ahci_port::queue_command(const void *fis, size_t fis_len, void *buf, size_t buf_len, bool write, int *slot_out) {
    LTRACEF("fis %p len %zu buf %p len %zu write %d\n", fis, fis_len, buf, buf_len, write);

    DEBUG_ASSERT(fis);
    DEBUG_ASSERT(fis_len > 0 && fis_len <= 64 && IS_ALIGNED(fis_len, 4));
    DEBUG_ASSERT(buf || buf_len == 0);

    // build a list of physical memory runs
    ahci_mem_run runs[PRD_PER_CMD];
    size_t run_count = countof(runs);
    status_t err = virtual_to_pa_runs(buf, buf_len, runs, &run_count);
    if (err != NO_ERROR) {
        return err;
    }
    DEBUG_ASSERT(buf_len == 0 || run_count != 0);
    DEBUG_ASSERT(run_count <= PRD_PER_CMD);

    AutoSpinLock guard(&lock_);

    // Allocate a slot
    uint slot;
    status_t status = find_free_cmdslot(&slot);
    if (status != NO_ERROR) {
        return status;
    }

    DEBUG_ASSERT((command_slots_ & (1U << slot)) == 0);
    // TODO: handle the case where we couldn't find a free slot

    LTRACEF("slot %u\n", slot);

    // clear interrupt status for this port
    write_port_reg(ahci_port_reg::PxIS, 0xf);

    auto *cmd_table = cmd_table_ptr(slot);

    // set up physical descriptors for runs of memory
    auto *prdt = &cmd_table->pdrt[0];
    for (size_t i = 0; i < run_count; i++, prdt++) {
        DEBUG_ASSERT(runs[i].length > 0 && runs[i].length <= MAX_PRDT_RUN_LENGTH); // 4MB max per entry
        prdt->dba = runs[i].address;
#if __INTPTR_WIDTH__ == 64
        prdt->dbau = runs[i].address >> 32;
#else
        prdt->dbau = 0;
#endif
        prdt->byte_count_ioc = runs[i].length - 1; // byte count
        if (i == run_count - 1) {
            prdt->byte_count_ioc |= (1U << 31); // interrupt on completion
        }
    }

    // copy command into the command table
    for (size_t i = 0; i < fis_len / 4; i++) {
        cmd_table->cfis[i] = static_cast<const uint32_t *>(fis)[i];
    }

    // set up the command header
    volatile ahci_cmd_header *cmd = &cmd_list_[slot];
    cmd->cmd = (fis_len / sizeof(uint32_t)) | // command fis size in words
               (write ? (1 << 6) : 0);        // read/write from device
    cmd->prdtl = run_count;                   // number of prdt entries

    // LTRACEF("cmd header %p\n", cmd);
    // hexdump8((const void *)cmd, sizeof(*cmd));

    // barrier here
    wmb();

    // unsignal the command complete event for this slot
    event_unsignal(&cmd_complete_event_[slot]);

    LTRACEF("IS %#x (before kick)\n", read_port_reg(ahci_port_reg::PxIS));

    cmd_pending_ |= (1U << slot);

    // kick the command
    write_port_reg(ahci_port_reg::PxCI, (1U << slot));

    *slot_out = slot;

    return NO_ERROR;
}

// Wait for a previously queued command to complete on the passed in slot.
status_t ahci_port::wait_for_completion(uint slot, uint32_t *error_status) {
    DEBUG_ASSERT(slot < command_slots_);
    DEBUG_ASSERT(is_command_queued(slot));

    // TODO: handle timeout
    auto err = event_wait(&cmd_complete_event_[slot]);

    AutoSpinLock guard(&lock_);

    // Return the last error/status register if requested from the
    // Port x Task File Data register.
    if (error_status) {
        *error_status = read_port_reg(ahci_port_reg::PxTFD);
    }

    // mark the command as not pending anymore
    cmd_pending_ &= ~(1u << slot);

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

        DEBUG_ASSERT(cmd_slot < command_slots_);

        LTRACEF("slot %zu completed\n", cmd_slot);

        // this slot completed
        event_signal(&cmd_complete_event_[cmd_slot], false);
        ret = INT_RESCHEDULE;

        // move to the next pending slot (if any)
        cmd_complete_bitmap &= ~(1U << cmd_slot);
    }

    // ack everything for now
    write_port_reg(ahci_port_reg::PxIS, is);

    return ret;
}
