/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "arch/x86/apic.h"

#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <lk/init.h>
#include <assert.h>
#include <arch/x86.h>
#include <kernel/spinlock.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0

enum ioapic_mmio_regs {
    IOAPIC_REGSEL = 0x00,
    IOAPIC_IOWIN = 0x10 / 4,
};

enum ioapic_regs {
    IOAPIC_ID = 0x00,
    IOAPIC_VERSION = 0x01,
    IOAPIC_ARB = 0x02,
    IOAPIC_REDIR_TABLE_BASE = 0x10,
};

struct ioapic {
    paddr_t phys_addr;
    volatile uint32_t *mmio;
    uint apic_id;
    uint gsi_base;
    uint num_redir_entries;

    // TODO: spinlock for this ioapic
};

static struct ioapic *ioapics = NULL;
static size_t num_ioapics = 0;

static uint32_t ioapic_read(struct ioapic *ioapic, enum ioapic_regs reg) {
    mmio_write32(ioapic->mmio + IOAPIC_REGSEL, reg);
    return mmio_read32(ioapic->mmio + IOAPIC_IOWIN);
}

static uint32_t ioapic_write(struct ioapic *ioapic, enum ioapic_regs reg, uint32_t val) {
    mmio_write32(ioapic->mmio + IOAPIC_REGSEL, reg);
    mmio_write32(ioapic->mmio + IOAPIC_IOWIN, val);
    return 0;
}

status_t ioapic_init(int index, paddr_t phys_addr, uint apic_id, uint gsi_base) {
    LTRACEF("%d: phys_addr %#lx apic_id %u gsi_base %u\n", index, phys_addr, apic_id, gsi_base);

    {
        struct ioapic *new_ioapics = realloc(ioapics, sizeof(struct ioapic) * (num_ioapics + 1));
        if (!new_ioapics) {
            return ERR_NO_MEMORY;
        }
        ioapics = new_ioapics;
    }
    struct ioapic *ioapic = &ioapics[num_ioapics];

    LTRACEF("mapping lapic into kernel\n");
    status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "ioapic", PAGE_SIZE, (void **)&ioapic->mmio, 0,
                            phys_addr, /* vmm_flags */ 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        // TODO: free up the newly extended ioapic struct
        return err;
    }
    DEBUG_ASSERT(ioapic->mmio != NULL);

    ioapic->phys_addr = phys_addr;
    ioapic->apic_id = apic_id;
    ioapic->gsi_base = gsi_base;

    // probe the capabilities of the ioapic
    const uint32_t id = ioapic_read(ioapic, IOAPIC_ID) >> 24;
    uint32_t version = ioapic_read(ioapic, IOAPIC_VERSION);
    const uint32_t max_redir = (version >> 16) & 0xff;
    dprintf(INFO, "X86: ioapic %d id %#x version %#x max redir %u\n", index, id, version & 0xff, max_redir);

    ioapic->num_redir_entries = max_redir + 1;

    if (LOCAL_TRACE) {
        for (uint i = 0; i < ioapic->num_redir_entries; i++) {
            // read and dump the current entry
            uint32_t lo = ioapic_read(ioapic, IOAPIC_REDIR_TABLE_BASE + i * 2);
            uint32_t hi = ioapic_read(ioapic, IOAPIC_REDIR_TABLE_BASE + i * 2 + 1);
            dprintf(INFO, "X86:     redir %2u hi %#x lo %#x\n", i, hi, lo);
        }
    }

    // add it to the global list of ioapics
    num_ioapics++;

    return NO_ERROR;
}
