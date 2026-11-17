/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "gic_v3_its.h"

#include <arch/arm64.h>
#include <arch/ops.h>
#include <assert.h>
#include <inttypes.h>
#include <kernel/mutex.h>
#include <kernel/spinlock.h>
#include <kernel/vm.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform.h>
#include <stdio.h>
#include <string.h>

#include "gic_v3_lpi.h"

#define LOCAL_TRACE 0

#define GITS_CTLR       0x0000
#define GITS_IIDR       0x0004
#define GITS_TYPER      0x0008
#define GITS_CBASER     0x0080
#define GITS_CWRITER    0x0088
#define GITS_CREADR     0x0090
#define GITS_BASER(n)   (0x0100 + (n) * 8)
#define GITS_TRANSLATER 0x10040 // in the translation frame, 64K above the control frame

#define GITS_CTLR_ENABLED   (1u << 0)
#define GITS_CTLR_QUIESCENT (1u << 31)

#define GITS_TYPER_ITT_ENTRY_SIZE(t) ((uint)(((t) >> 4) & 0xf) + 1)
#define GITS_TYPER_IDBITS(t)         ((uint)(((t) >> 8) & 0x1f) + 1)
#define GITS_TYPER_DEVBITS(t)        ((uint)(((t) >> 13) & 0x1f) + 1)
#define GITS_TYPER_PTA               (1ull << 19)

#define GITS_CREADR_STALLED (1u << 0)
#define GITS_CMDQ_OFFSET(v) ((v) & 0xfffe0) // [19:5]

// GITS_BASERn
#define BASER_VALID            (1ull << 63)
#define BASER_INDIRECT         (1ull << 62)
#define BASER_INNERCACHE_MASK  (7ull << 59)
#define BASER_INNERCACHE_NC    (1ull << 59)
#define BASER_INNERCACHE_RAWAWB (7ull << 59)
#define BASER_TYPE(b)          ((uint)(((b) >> 56) & 7))
#define BASER_OUTERCACHE_MASK  (7ull << 53)
#define BASER_ENTRY_SIZE(b)    ((uint)(((b) >> 48) & 0x1f) + 1)
#define BASER_PA_MASK          0x0000fffffffff000ull // [47:12]; [15:12] are PA[51:48] with 64K pages
#define BASER_SHAREABILITY_MASK (3ull << 10)
#define BASER_INNER_SHAREABLE  (1ull << 10)
#define BASER_PAGE_SIZE_SHIFT  8
#define BASER_PAGE_SIZE_MASK   (3ull << 8)
#define BASER_SIZE_MASK        0xffull

#define BASER_TYPE_DEVICE     1
#define BASER_TYPE_COLLECTION 4

// GITS_CBASER: same layout for valid, caches, PA and shareability; Size counts 4K pages
#define CBASER_SIZE_4K_PAGES(n) ((uint64_t)(n) - 1)

#define ITS_CMDQ_SIZE (64 * 1024) // the base is always 64K aligned (spec 5.2.8)
#define ITS_PAGE_64K  (64 * 1024)

#define ITS_CMD_MOVI   0x01
#define ITS_CMD_INT    0x03
#define ITS_CMD_SYNC   0x05
#define ITS_CMD_MAPD   0x08
#define ITS_CMD_MAPC   0x09
#define ITS_CMD_MAPTI  0x0a
#define ITS_CMD_INV    0x0c
#define ITS_CMD_INVALL 0x0d
#define ITS_CMD_DISCARD 0x0f

struct its_cmd {
    uint64_t dw[4];
};
STATIC_ASSERT(sizeof(struct its_cmd) == 32);

struct its_device {
    bool used;
    uint32_t id;
    paddr_t itt_pa;
    uint32_t events; // bit n: event n is mapped
};

// what an allocated LPI is the translation of
struct its_lpi {
    struct its_device *dev;
    uint32_t event;
};

static struct {
    bool ready;
    bool flush; // tables or queue came back non-shareable: clean what the ITS reads to PoC
    paddr_t paddr;
    vaddr_t vaddr;
    uint64_t typer;
    bool pta;
    uint devbits;
    uint idbits;
    uint itt_entry_size;

    // command queue
    struct its_cmd *cmdq;
    size_t cmdq_wr; // byte offset of the next command

    // device table: flat, or a level 1 table of 64 bit pointers to one page level 2 tables
    bool dev_indirect;
    uint dev_entry_size;
    size_t dev_page_size;
    uint64_t *dev_l1;
    size_t dev_l1_entries;
    uint32_t dev_ids; // device ids the table covers

    struct its_device devices[ARM_GIC_ITS_MAX_DEVICES];
    struct its_lpi lpis[ARM_GIC_MAX_LPIS];
} its;

static spin_lock_t its_cmd_lock = SPIN_LOCK_INITIAL_VALUE;
static mutex_t its_mutex = MUTEX_INITIAL_VALUE(its_mutex); // device and ITT setup, which allocates

static uint32_t its_read32(uint32_t reg) {
    return mmio_read32((volatile uint32_t *)(its.vaddr + reg));
}

static void its_write32(uint32_t reg, uint32_t val) {
    mmio_write32((volatile uint32_t *)(its.vaddr + reg), val);
}

static uint64_t its_read64(uint32_t reg) {
    return mmio_read64((volatile uint64_t *)(its.vaddr + reg));
}

static void its_write64(uint32_t reg, uint64_t val) {
    mmio_write64((volatile uint64_t *)(its.vaddr + reg), val);
}

// zeroed, physically contiguous, cacheable memory the ITS reads, cleaned to PoC
static void *its_alloc(const char *name, size_t size, uint8_t align_log2, paddr_t *pa) {
    void *ptr;
    size = ROUNDUP(size, PAGE_SIZE);
    if (vmm_alloc_contiguous(vmm_get_kernel_aspace(), name, size, &ptr, align_log2, 0,
                             ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE) != NO_ERROR) {
        return NULL;
    }
    memset(ptr, 0, size);
    arch_clean_cache_range((addr_t)ptr, size);
    *pa = vaddr_to_paddr(ptr);
    return ptr;
}

static void its_make_visible(const void *ptr, size_t len) {
    if (its.flush) {
        arch_clean_cache_range((addr_t)ptr, len);
    }
    DSB;
}

static uint log2_floor(size_t v) {
    uint l = 0;
    while (v >>= 1) {
        l++;
    }
    return l;
}

/* commands */

// Queue commands and wait for the ITS to consume them. QEMU and KVM process the queue
// synchronously and skip a malformed command without stalling, so callers check their
// arguments against the table sizes before building one.
static status_t its_send(const struct its_cmd *cmds, size_t count) {
    status_t err = NO_ERROR;
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&its_cmd_lock);

    for (size_t i = 0; i < count; i++) {
        const size_t next = (its.cmdq_wr + sizeof(struct its_cmd)) % ITS_CMDQ_SIZE;
        // full when the write pointer would catch up with the read pointer
        lk_time_t start = current_time();
        while (GITS_CMDQ_OFFSET(its_read64(GITS_CREADR)) == next) {
            if (current_time() - start > 1000) {
                err = ERR_TIMED_OUT;
                goto out;
            }
        }
        struct its_cmd *slot = &its.cmdq[its.cmdq_wr / sizeof(struct its_cmd)];
        *slot = cmds[i];
        its_make_visible(slot, sizeof(*slot));
        its.cmdq_wr = next;
    }

    its_write64(GITS_CWRITER, its.cmdq_wr);

    lk_time_t start = current_time();
    for (;;) {
        const uint64_t creadr = its_read64(GITS_CREADR);
        if (GITS_CMDQ_OFFSET(creadr) == its.cmdq_wr) {
            break;
        }
        if (creadr & GITS_CREADR_STALLED) {
            printf("GICv3 ITS: command queue stalled at %#" PRIx64 "\n", GITS_CMDQ_OFFSET(creadr));
            err = ERR_IO;
            break;
        }
        if (current_time() - start > 1000) {
            printf("GICv3 ITS: command queue timed out at %#" PRIx64 "\n", GITS_CMDQ_OFFSET(creadr));
            err = ERR_TIMED_OUT;
            break;
        }
    }

out:
    spin_unlock_irqrestore(&its_cmd_lock, state);
    return err;
}

static uint64_t its_rdbase(uint cpu) {
    return arm_gicv3_lpi_rdbase(cpu, its.pta);
}

static struct its_cmd cmd_sync(uint cpu) {
    return (struct its_cmd){{ITS_CMD_SYNC, 0, its_rdbase(cpu) << 16, 0}};
}

static struct its_cmd cmd_mapc(uint icid, uint cpu) {
    return (struct its_cmd){{ITS_CMD_MAPC, 0, (1ull << 63) | (its_rdbase(cpu) << 16) | icid, 0}};
}

static struct its_cmd cmd_mapd(uint32_t dev, paddr_t itt, uint event_bits) {
    return (struct its_cmd){{ITS_CMD_MAPD | (uint64_t)dev << 32, event_bits - 1,
                             (1ull << 63) | (itt & 0x000fffffffffff00ull), 0}};
}

static struct its_cmd cmd_mapti(uint32_t dev, uint32_t event, uint intid, uint icid) {
    return (struct its_cmd){{ITS_CMD_MAPTI | (uint64_t)dev << 32, event | (uint64_t)intid << 32,
                             icid, 0}};
}

static struct its_cmd cmd_dev_event(uint8_t op, uint32_t dev, uint32_t event) {
    return (struct its_cmd){{op | (uint64_t)dev << 32, event, 0, 0}};
}

static struct its_cmd cmd_movi(uint32_t dev, uint32_t event, uint icid) {
    return (struct its_cmd){{ITS_CMD_MOVI | (uint64_t)dev << 32, event, icid, 0}};
}

static struct its_cmd cmd_invall(uint icid) {
    return (struct its_cmd){{ITS_CMD_INVALL, 0, icid, 0}};
}

/* tables */

static size_t page_size_from_code(uint code) {
    switch (code) {
        case 0: return 4096;
        case 1: return 16384;
        default: return 65536;
    }
}

// Write a BASER value asking for inner shareable write-back, and read back what stuck: a
// non-shareable result falls back to non-cacheable (as Linux does) and needs cleaning.
static uint64_t its_write_baser(uint n, uint64_t val) {
    val |= BASER_INNER_SHAREABLE | BASER_INNERCACHE_RAWAWB;
    its_write64(GITS_BASER(n), val);
    uint64_t readback = its_read64(GITS_BASER(n));
    if ((readback & BASER_SHAREABILITY_MASK) == 0) {
        its.flush = true;
        val &= ~(BASER_SHAREABILITY_MASK | BASER_INNERCACHE_MASK);
        val |= BASER_INNERCACHE_NC;
        its_write64(GITS_BASER(n), val);
        readback = its_read64(GITS_BASER(n));
    }
    return readback;
}

// Find the page size the BASER takes: 64K if it will, else whatever it reads back with.
static size_t its_probe_page_size(uint n, uint64_t baser) {
    baser &= ~(BASER_VALID | BASER_PAGE_SIZE_MASK);
    its_write64(GITS_BASER(n), baser | (2ull << BASER_PAGE_SIZE_SHIFT));
    const uint64_t readback = its_read64(GITS_BASER(n));
    return page_size_from_code((uint)((readback & BASER_PAGE_SIZE_MASK) >> BASER_PAGE_SIZE_SHIFT));
}

static uint64_t page_size_code(size_t psz) {
    return (psz == 4096 ? 0ull : psz == 16384 ? 1ull : 2ull) << BASER_PAGE_SIZE_SHIFT;
}

// Allocate and program the table of BASER n. A device table goes two level when it would take
// more than a page flat and the ITS takes the Indirect bit.
static status_t its_setup_baser(uint n, uint64_t baser) {
    const uint type = BASER_TYPE(baser);
    const uint esz = BASER_ENTRY_SIZE(baser);
    const size_t psz = its_probe_page_size(n, baser);

    const size_t ids = (type == BASER_TYPE_DEVICE) ? (1ul << its.devbits) : SMP_MAX_CPUS;
    size_t bytes = ids * esz;
    bool indirect = false;
    if (type == BASER_TYPE_DEVICE && bytes > psz) {
        its_write64(GITS_BASER(n), (baser & ~BASER_VALID) | BASER_INDIRECT | page_size_code(psz));
        indirect = (its_read64(GITS_BASER(n)) & BASER_INDIRECT) != 0;
        if (indirect) {
            // one 64 bit level 1 entry per level 2 page
            bytes = DIV_ROUND_UP(ids, psz / esz) * sizeof(uint64_t);
        }
    }
    size_t pages = DIV_ROUND_UP(bytes, psz);
    if (pages > 256) {
        pages = 256; // the most Size can describe; device ids past it are refused
    }

    paddr_t pa;
    void *table = its_alloc(type == BASER_TYPE_DEVICE ? "gic its devices" : "gic its collections",
                            pages * psz, (uint8_t)log2_floor(psz), &pa);
    if (!table) {
        return ERR_NO_MEMORY;
    }

    uint64_t val = baser & ~(BASER_VALID | BASER_INDIRECT | BASER_INNERCACHE_MASK |
                             BASER_OUTERCACHE_MASK | BASER_PA_MASK | BASER_SHAREABILITY_MASK |
                             BASER_PAGE_SIZE_MASK | BASER_SIZE_MASK);
    val |= BASER_VALID | (indirect ? BASER_INDIRECT : 0) | (pa & BASER_PA_MASK) |
           page_size_code(psz) | (pages - 1);
    const uint64_t readback = its_write_baser(n, val);

    // what stuck has to be what the table was sized for
    if (!(readback & BASER_VALID) || (readback & BASER_PA_MASK) != (pa & BASER_PA_MASK) ||
        (readback & BASER_PAGE_SIZE_MASK) != page_size_code(psz) ||
        (readback & BASER_SIZE_MASK) != pages - 1 ||
        ((readback & BASER_INDIRECT) != 0) != indirect) {
        printf("GICv3 ITS: BASER%u wrote %#" PRIx64 ", read back %#" PRIx64 "\n", n, val, readback);
        return ERR_NOT_SUPPORTED;
    }

    if (type == BASER_TYPE_DEVICE) {
        its.dev_indirect = indirect;
        its.dev_entry_size = esz;
        its.dev_page_size = psz;
        if (indirect) {
            its.dev_l1 = table;
            its.dev_l1_entries = pages * psz / sizeof(uint64_t);
            its.dev_ids = (uint32_t)MIN(ids, its.dev_l1_entries * (psz / esz));
        } else {
            its.dev_ids = (uint32_t)MIN(ids, pages * psz / esz);
        }
    }
    LTRACEF("BASER%u type %u esz %u psz %zu pages %zu%s: %#" PRIx64 "\n", n, type, esz, psz,
            pages, indirect ? " indirect" : "", readback);
    return NO_ERROR;
}

// Make sure the level 2 table that holds device id exists. Called with its_mutex held.
static status_t its_ensure_device_table(uint32_t dev) {
    if (!its.dev_indirect) {
        return NO_ERROR;
    }
    const size_t index = dev / (its.dev_page_size / its.dev_entry_size);
    DEBUG_ASSERT(index < its.dev_l1_entries);
    if (its.dev_l1[index] & BASER_VALID) {
        return NO_ERROR;
    }
    paddr_t pa;
    if (!its_alloc("gic its devices l2", its.dev_page_size, (uint8_t)log2_floor(its.dev_page_size), &pa)) {
        return ERR_NO_MEMORY;
    }
    its.dev_l1[index] = BASER_VALID | pa;
    its_make_visible(&its.dev_l1[index], sizeof(uint64_t));
    return NO_ERROR;
}

/* setup */

status_t arm_gicv3_its_init(paddr_t paddr, vaddr_t vaddr) {
    if (its.ready) {
        return NO_ERROR;
    }
    its.paddr = paddr;
    its.vaddr = vaddr;

    // disable and wait for quiescence before touching any base register
    uint32_t ctlr = its_read32(GITS_CTLR);
    if (ctlr & GITS_CTLR_ENABLED) {
        its_write32(GITS_CTLR, ctlr & ~GITS_CTLR_ENABLED);
    }
    lk_time_t start = current_time();
    while (!(its_read32(GITS_CTLR) & GITS_CTLR_QUIESCENT)) {
        if (current_time() - start > 1000) {
            printf("GICv3 ITS: at %#" PRIxPTR " never went quiescent\n", (uintptr_t)paddr);
            return ERR_TIMED_OUT;
        }
    }

    its.typer = its_read64(GITS_TYPER);
    its.pta = (its.typer & GITS_TYPER_PTA) != 0;
    its.devbits = MIN(GITS_TYPER_DEVBITS(its.typer), 16u); // PCI requester ids are 16 bits
    its.idbits = GITS_TYPER_IDBITS(its.typer);
    its.itt_entry_size = GITS_TYPER_ITT_ENTRY_SIZE(its.typer);
    if ((1u << its.idbits) < ARM_GIC_ITS_EVENTS_PER_DEVICE) {
        printf("GICv3 ITS: only %u event id bits\n", its.idbits);
        return ERR_NOT_SUPPORTED;
    }

    bool have_device = false, have_collection = false;
    for (uint n = 0; n < 8; n++) {
        const uint64_t baser = its_read64(GITS_BASER(n));
        const uint type = BASER_TYPE(baser);
        if (type != BASER_TYPE_DEVICE && type != BASER_TYPE_COLLECTION) {
            continue; // unimplemented, or a vPE table
        }
        status_t err = its_setup_baser(n, baser);
        if (err != NO_ERROR) {
            return err;
        }
        have_device |= type == BASER_TYPE_DEVICE;
        have_collection |= type == BASER_TYPE_COLLECTION;
    }
    if (!have_device || !have_collection) {
        printf("GICv3 ITS: no %s table\n", have_device ? "collection" : "device");
        return ERR_NOT_SUPPORTED;
    }

    paddr_t cmdq_pa;
    its.cmdq = its_alloc("gic its commands", ITS_CMDQ_SIZE, 16, &cmdq_pa);
    if (!its.cmdq) {
        return ERR_NO_MEMORY;
    }
    uint64_t cbaser = BASER_VALID | BASER_INNERCACHE_RAWAWB | BASER_INNER_SHAREABLE |
                      (cmdq_pa & 0x000ffffffffff000ull) | CBASER_SIZE_4K_PAGES(ITS_CMDQ_SIZE / 4096);
    its_write64(GITS_CBASER, cbaser);
    if ((its_read64(GITS_CBASER) & BASER_SHAREABILITY_MASK) == 0) {
        its.flush = true;
        cbaser = (cbaser & ~(BASER_SHAREABILITY_MASK | BASER_INNERCACHE_MASK)) | BASER_INNERCACHE_NC;
        its_write64(GITS_CBASER, cbaser);
    }
    its.cmdq_wr = 0;
    its_write64(GITS_CWRITER, 0);

    its_write32(GITS_CTLR, its_read32(GITS_CTLR) | GITS_CTLR_ENABLED);
    if (!(its_read32(GITS_CTLR) & GITS_CTLR_ENABLED)) {
        // KVM ignores the enable unless the device and collection tables and the queue are valid
        printf("GICv3 ITS: refused to enable (CBASER %#" PRIx64 ")\n", its_read64(GITS_CBASER));
        return ERR_NOT_SUPPORTED;
    }

    its.ready = true;
    dprintf(INFO, "GICv3 ITS: at %#" PRIxPTR ", %u device id bits (%u covered), %u byte ITT entries%s%s\n",
            (uintptr_t)paddr, GITS_TYPER_DEVBITS(its.typer), its.dev_ids, its.itt_entry_size,
            its.dev_indirect ? ", two level device table" : "", its.flush ? ", non-shareable" : "");
    return NO_ERROR;
}

bool arm_gicv3_its_ready(void) {
    return its.ready;
}

paddr_t arm_gicv3_its_paddr(void) {
    return its.ready ? its.paddr : 0;
}

void arm_gicv3_its_init_percpu(uint cpu) {
    if (!its.ready || !arm_gicv3_lpi_cpu_enabled(cpu)) {
        return;
    }
    const struct its_cmd cmds[] = {
        cmd_mapc(cpu, cpu),
        cmd_sync(cpu),
        cmd_invall(cpu),
        cmd_sync(cpu),
    };
    status_t err = its_send(cmds, countof(cmds));
    if (err != NO_ERROR) {
        printf("GICv3 ITS: mapping cpu %u's collection failed (%d)\n", cpu, err);
    }
}

/* MSIs */

static struct its_device *its_find_device(uint32_t id) {
    for (size_t i = 0; i < countof(its.devices); i++) {
        if (its.devices[i].used && its.devices[i].id == id) {
            return &its.devices[i];
        }
    }
    return NULL;
}

// Find or map device id. Called with its_mutex held.
static status_t its_get_device(uint32_t id, struct its_device **out) {
    struct its_device *dev = its_find_device(id);
    if (dev) {
        *out = dev;
        return NO_ERROR;
    }
    for (size_t i = 0; i < countof(its.devices); i++) {
        if (!its.devices[i].used) {
            dev = &its.devices[i];
            break;
        }
    }
    if (!dev) {
        return ERR_NO_RESOURCES;
    }

    status_t err = its_ensure_device_table(id);
    if (err != NO_ERROR) {
        return err;
    }

    // the ITT: one entry per event, 256 byte aligned (a page here)
    paddr_t itt_pa;
    if (!its_alloc("gic its itt", ARM_GIC_ITS_EVENTS_PER_DEVICE * its.itt_entry_size, 0, &itt_pa)) {
        return ERR_NO_MEMORY;
    }

    const struct its_cmd cmds[] = {
        cmd_mapd(id, itt_pa, log2_floor(ARM_GIC_ITS_EVENTS_PER_DEVICE)),
        cmd_sync(0),
    };
    err = its_send(cmds, countof(cmds));
    if (err != NO_ERROR) {
        return err;
    }

    *dev = (struct its_device){.used = true, .id = id, .itt_pa = itt_pa, .events = 0};
    LTRACEF("device %#x itt %#" PRIxPTR "\n", id, (uintptr_t)itt_pa);
    *out = dev;
    return NO_ERROR;
}

status_t arm_gicv3_its_msi_alloc(paddr_t paddr, uint32_t device_id, uint *vector,
                                 uint64_t *msg_addr, uint32_t *msg_data) {
    if (!its.ready || paddr != its.paddr) {
        return ERR_NOT_FOUND;
    }
    if (device_id >= its.dev_ids) {
        printf("GICv3 ITS: device id %#x is past the %u the device table covers\n", device_id,
               its.dev_ids);
        return ERR_OUT_OF_RANGE;
    }

    mutex_acquire(&its_mutex);

    struct its_device *dev;
    status_t err = its_get_device(device_id, &dev);
    if (err != NO_ERROR) {
        goto out;
    }

    uint32_t event = 0;
    while (event < ARM_GIC_ITS_EVENTS_PER_DEVICE && (dev->events & (1u << event))) {
        event++;
    }
    if (event == ARM_GIC_ITS_EVENTS_PER_DEVICE) {
        err = ERR_NO_RESOURCES;
        goto out;
    }

    uint intid;
    err = arm_gicv3_lpi_alloc(&intid);
    if (err != NO_ERROR) {
        goto out;
    }

    const struct its_cmd cmds[] = {
        cmd_mapti(device_id, event, intid, 0),
        cmd_sync(0),
    };
    err = its_send(cmds, countof(cmds));
    if (err != NO_ERROR) {
        arm_gicv3_lpi_free(intid);
        goto out;
    }

    dev->events |= 1u << event;
    its.lpis[intid - GIC_LPI_BASE] = (struct its_lpi){.dev = dev, .event = event};

    *vector = intid;
    *msg_addr = (uint64_t)its.paddr + GITS_TRANSLATER;
    *msg_data = event;
    LTRACEF("device %#x event %u -> LPI %u\n", device_id, event, intid);

out:
    mutex_release(&its_mutex);
    return err;
}

static struct its_lpi *its_lookup_lpi(uint intid) {
    if (!its.ready || intid < GIC_LPI_BASE || intid - GIC_LPI_BASE >= ARM_GIC_MAX_LPIS) {
        return NULL;
    }
    struct its_lpi *l = &its.lpis[intid - GIC_LPI_BASE];
    return l->dev ? l : NULL;
}

status_t arm_gicv3_its_msi_free(paddr_t paddr, uint32_t device_id, uint vector) {
    if (!its.ready || paddr != its.paddr) {
        return ERR_NOT_FOUND;
    }
    mutex_acquire(&its_mutex);
    struct its_lpi *l = its_lookup_lpi(vector);
    status_t err = ERR_INVALID_ARGS;
    if (l && l->dev->id == device_id) {
        const struct its_cmd cmds[] = {
            cmd_dev_event(ITS_CMD_DISCARD, device_id, l->event),
            cmd_sync(0),
        };
        err = its_send(cmds, countof(cmds));
        l->dev->events &= ~(1u << l->event);
        *l = (struct its_lpi){};
        arm_gicv3_lpi_free(vector);
    }
    mutex_release(&its_mutex);
    return err;
}

status_t arm_gicv3_its_lpi_invalidate(uint intid) {
    struct its_lpi *l = its_lookup_lpi(intid);
    if (!l) {
        return ERR_INVALID_ARGS;
    }
    const struct its_cmd cmds[] = {
        cmd_dev_event(ITS_CMD_INV, l->dev->id, l->event),
        cmd_sync(0),
    };
    return its_send(cmds, countof(cmds));
}

status_t arm_gicv3_its_inject(uint intid) {
    struct its_lpi *l = its_lookup_lpi(intid);
    if (!l) {
        return ERR_INVALID_ARGS;
    }
    const struct its_cmd cmds[] = {
        cmd_dev_event(ITS_CMD_INT, l->dev->id, l->event),
        cmd_sync(0),
    };
    return its_send(cmds, countof(cmds));
}

status_t arm_gicv3_its_move(uint intid, uint cpu) {
    struct its_lpi *l = its_lookup_lpi(intid);
    if (!l || !arm_gicv3_lpi_cpu_enabled(cpu)) {
        return ERR_INVALID_ARGS;
    }
    const struct its_cmd cmds[] = {
        cmd_movi(l->dev->id, l->event, cpu),
        cmd_sync(cpu),
    };
    return its_send(cmds, countof(cmds));
}
