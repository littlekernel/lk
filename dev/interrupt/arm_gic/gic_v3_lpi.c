/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "gic_v3_lpi.h"

#include <arch/arm64.h>
#include <arch/arm64/mp.h>
#include <arch/ops.h>
#include <assert.h>
#include <inttypes.h>
#include <kernel/spinlock.h>
#include <kernel/vm.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>
#include <string.h>

#define LOCAL_TRACE 0

#define GICD_TYPER_LPIS      (1u << 17)
#define GICD_TYPER_IDBITS(t) ((((t) >> 19) & 0x1f) + 1)

#define GICR_CTLR_ENABLE_LPIS   (1u << 0)
#define GICR_TYPER_PLPIS        (1ull << 0)
#define GICR_TYPER_PROC_NUM(t)  (((t) >> 8) & 0xffff)
#define GICR_TYPER_AFFINITY(t)  ((uint32_t)((t) >> 32))

// GICR_PROPBASER / GICR_PENDBASER attribute fields
#define GICR_BASER_SHAREABILITY_MASK (3ull << 10)
#define GICR_BASER_INNER_SHAREABLE   (1ull << 10)
#define GICR_BASER_INNERCACHE_MASK   (7ull << 7)
#define GICR_BASER_INNERCACHE_NC     (1ull << 7)
#define GICR_BASER_INNERCACHE_RAWAWB (7ull << 7)

// LPI INTIDs below 1 << 14, the smallest an IDbits field can describe: 8192 LPIs, of which LK
// hands out the first ARM_GIC_MAX_LPIS
#define LPI_ID_BITS   14
#define LPI_PROP_SIZE ((1u << LPI_ID_BITS) - GIC_LPI_BASE)
#define LPI_PEND_SIZE ((1u << LPI_ID_BITS) / 8)

// property table entry: priority [7:2], bit 1 RES1, enable bit 0
#define LPI_PROP_PRIO    0xa0
#define LPI_PROP_GROUP1  (1u << 1)
#define LPI_PROP_ENABLED (1u << 0)

static struct {
    bool ready;
    bool flush; // the tables came back non-shareable: clean what the GIC reads to PoC
    uint8_t *prop;
    paddr_t prop_pa;
    paddr_t pend_pa[SMP_MAX_CPUS];
    bool cpu_enabled[SMP_MAX_CPUS];
    uint64_t allocated[ARM_GIC_MAX_LPIS / 64];
    struct int_handler_struct handlers[ARM_GIC_MAX_LPIS];
} lpi;

STATIC_ASSERT(ARM_GIC_MAX_LPIS % 64 == 0);
STATIC_ASSERT(ARM_GIC_MAX_LPIS <= LPI_PROP_SIZE);

static spin_lock_t lpi_lock = SPIN_LOCK_INITIAL_VALUE;

// zeroed, physically contiguous, cacheable memory the GIC reads, cleaned to PoC
static void *alloc_table(const char *name, size_t size, uint8_t align_log2, paddr_t *pa) {
    void *ptr;
    size = ROUNDUP(size, PAGE_SIZE);
    status_t err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), name, size, &ptr, align_log2, 0,
                                        ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE);
    if (err != NO_ERROR) {
        return NULL;
    }
    memset(ptr, 0, size);
    arch_clean_cache_range((addr_t)ptr, size);
    *pa = vaddr_to_paddr(ptr);
    return ptr;
}

status_t arm_gicv3_lpi_init(void) {
    if (lpi.ready) {
        return NO_ERROR;
    }

    const uint32_t typer = gicd_read(0, GICD_TYPER);
    if (!(typer & GICD_TYPER_LPIS)) {
        printf("GICv3: distributor has no LPI support (TYPER %#x)\n", typer);
        return ERR_NOT_SUPPORTED;
    }
    if (GICD_TYPER_IDBITS(typer) < LPI_ID_BITS) {
        printf("GICv3: distributor INTIDs are %u bits, LPIs need %u\n", GICD_TYPER_IDBITS(typer),
               LPI_ID_BITS);
        return ERR_NOT_SUPPORTED;
    }

    // the redistributors take the property table with 4K alignment, a pending table with 64K
    lpi.prop = alloc_table("gic lpi config", LPI_PROP_SIZE, 16, &lpi.prop_pa);
    if (!lpi.prop) {
        return ERR_NO_MEMORY;
    }
    memset(lpi.prop, LPI_PROP_PRIO | LPI_PROP_GROUP1, LPI_PROP_SIZE);
    arch_clean_cache_range((addr_t)lpi.prop, LPI_PROP_SIZE);

    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (!alloc_table("gic lpi pending", LPI_PEND_SIZE, 16, &lpi.pend_pa[cpu])) {
            return ERR_NO_MEMORY;
        }
    }

    lpi.ready = true;
    dprintf(INFO, "GICv3: LPIs %u..%u, config table at %#" PRIxPTR "\n", GIC_LPI_BASE,
            GIC_LPI_BASE + ARM_GIC_MAX_LPIS - 1, (uintptr_t)lpi.prop_pa);
    return NO_ERROR;
}

bool arm_gicv3_lpi_ready(void) {
    return lpi.ready;
}

// Write a PROPBASER/PENDBASER value asking for inner shareable write-back and fall back to
// non-cacheable if the redistributor makes it non-shareable, as Linux does. Returns false if
// it ended up non-shareable, so table writes need cleaning.
static bool write_lpi_baser(uint cpu, uint32_t reg, uint64_t val) {
    val |= GICR_BASER_INNER_SHAREABLE | GICR_BASER_INNERCACHE_RAWAWB;
    gicr_write64(0, cpu, reg, val);
    uint64_t readback = gicr_read64(0, cpu, reg);
    if ((readback & GICR_BASER_SHAREABILITY_MASK) != 0) {
        return true;
    }
    val &= ~(GICR_BASER_SHAREABILITY_MASK | GICR_BASER_INNERCACHE_MASK);
    val |= GICR_BASER_INNERCACHE_NC;
    gicr_write64(0, cpu, reg, val);
    return false;
}

status_t arm_gicv3_lpi_init_percpu(uint cpu) {
    if (!lpi.ready || cpu >= SMP_MAX_CPUS) {
        return ERR_NOT_READY;
    }
    if (lpi.cpu_enabled[cpu]) {
        return NO_ERROR;
    }

    const uint64_t typer = gicr_read64(0, cpu, GICR_TYPER);
    if (!(typer & GICR_TYPER_PLPIS)) {
        printf("GICv3: cpu %u redistributor has no LPI support\n", cpu);
        return ERR_NOT_SUPPORTED;
    }

    // redistributor frames are indexed by cpu number; make sure this one is the cpu's
    const uint64_t mpidr = arm64_cpu_num_to_mpidr(cpu);
    const uint32_t aff = (uint32_t)(mpidr & 0xffffff) | (uint32_t)((mpidr >> 32) & 0xff) << 24;
    if (GICR_TYPER_AFFINITY(typer) != aff) {
        printf("GICv3: cpu %u redistributor frame has affinity %#x, cpu is %#x; no LPIs on it\n",
               cpu, GICR_TYPER_AFFINITY(typer), aff);
        return ERR_NOT_FOUND;
    }

    const uint32_t ctlr = gicr_read(0, cpu, GICR_CTRL);
    if (ctlr & GICR_CTLR_ENABLE_LPIS) {
        // the table bases cannot change once LPIs are on, and these are not ours
        printf("GICv3: cpu %u already has LPIs enabled, leaving its redistributor alone\n", cpu);
        return ERR_BAD_STATE;
    }

    bool shareable = write_lpi_baser(cpu, GICR_PROPBASER, lpi.prop_pa | (LPI_ID_BITS - 1));
    shareable &= write_lpi_baser(cpu, GICR_PENDBASER, lpi.pend_pa[cpu]);
    if (!shareable) {
        lpi.flush = true;
    }

    DSB;
    gicr_write(0, cpu, GICR_CTRL, ctlr | GICR_CTLR_ENABLE_LPIS);
    DSB;

    lpi.cpu_enabled[cpu] = true;
    LTRACEF("cpu %u: LPIs enabled, proc num %u%s\n", cpu, (uint)GICR_TYPER_PROC_NUM(typer),
            shareable ? "" : ", tables non-shareable");
    return NO_ERROR;
}

bool arm_gicv3_lpi_cpu_enabled(uint cpu) {
    return cpu < SMP_MAX_CPUS && lpi.cpu_enabled[cpu];
}

uint64_t arm_gicv3_lpi_rdbase(uint cpu, bool pta) {
    if (pta) {
        return (arm_gics[0].gicr_paddr + GICR_CPU_OFFSET(0, cpu)) >> 16;
    }
    return GICR_TYPER_PROC_NUM(gicr_read64(0, cpu, GICR_TYPER));
}

status_t arm_gicv3_lpi_alloc(uint *intid) {
    if (!lpi.ready) {
        return ERR_NOT_READY;
    }

    int found = -1;
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lpi_lock);
    for (uint i = 0; i < ARM_GIC_MAX_LPIS; i++) {
        if ((lpi.allocated[i / 64] & (1ULL << (i % 64))) == 0) {
            lpi.allocated[i / 64] |= 1ULL << (i % 64);
            lpi.handlers[i] = (struct int_handler_struct){};
            found = (int)i;
            break;
        }
    }
    spin_unlock_irqrestore(&lpi_lock, state);
    if (found < 0) {
        return ERR_NO_RESOURCES;
    }

    *intid = GIC_LPI_BASE + (uint)found;
    arm_gicv3_lpi_set_enable(*intid, false);
    return NO_ERROR;
}

void arm_gicv3_lpi_free(uint intid) {
    if (intid < GIC_LPI_BASE || intid - GIC_LPI_BASE >= ARM_GIC_MAX_LPIS) {
        return;
    }
    const uint i = intid - GIC_LPI_BASE;
    arm_gicv3_lpi_set_enable(intid, false);
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lpi_lock);
    lpi.handlers[i] = (struct int_handler_struct){};
    lpi.allocated[i / 64] &= ~(1ULL << (i % 64));
    spin_unlock_irqrestore(&lpi_lock, state);
}

void arm_gicv3_lpi_set_enable(uint intid, bool enable) {
    DEBUG_ASSERT(lpi.ready);
    DEBUG_ASSERT(intid >= GIC_LPI_BASE && intid - GIC_LPI_BASE < ARM_GIC_MAX_LPIS);

    volatile uint8_t *entry = &lpi.prop[intid - GIC_LPI_BASE];
    *entry = LPI_PROP_PRIO | LPI_PROP_GROUP1 | (enable ? LPI_PROP_ENABLED : 0);
    if (lpi.flush) {
        arch_clean_cache_range((addr_t)entry, 1);
    }
    DSB;
}

struct int_handler_struct *arm_gicv3_lpi_handler(uint intid) {
    if (intid < GIC_LPI_BASE || intid - GIC_LPI_BASE >= ARM_GIC_MAX_LPIS) {
        return NULL;
    }
    const uint i = intid - GIC_LPI_BASE;
    if ((lpi.allocated[i / 64] & (1ULL << (i % 64))) == 0) {
        return NULL;
    }
    return &lpi.handlers[i];
}
