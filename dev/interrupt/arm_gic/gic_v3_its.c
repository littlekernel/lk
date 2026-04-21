/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <arch/ops.h>
#include <inttypes.h>
#include <kernel/vm.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdint.h>
#include <string.h>

#include "arm_gic_common.h"
#include "gic_v3_its.h"

/* GICv3 ITS registers */
#define GITS_CTLR       (0x0000)
#define GITS_TYPER      (0x0008)
#define GITS_CBASER     (0x0080)
#define GITS_CWRITER    (0x0088)
#define GITS_CREADR     (0x0090)
#define GITS_BASER(n)   (0x0100 + ((n) * 8))

#define GITS_CTLR_ENABLED     (1U << 0)
#define GITS_CBASER_VALID     (1ULL << 63)
#define GITS_BASER_VALID      (1ULL << 63)
#define GITS_BASER_TYPE_SHIFT (56)
#define GITS_BASER_TYPE_MASK  (0x7ULL << GITS_BASER_TYPE_SHIFT)
#define GITS_ADDR_MASK        (0x0000FFFFFFFFF000ULL)

#define GITS_BASER_TYPE_DEVICE     (1U)
#define GITS_BASER_TYPE_COLLECTION (4U)

#define GICV3_ITS_CMD_QUEUE_SIZE        ((size_t)64 * 1024)
#define GICV3_ITS_DEVICE_TABLE_SIZE     ((size_t)64 * 1024)
#define GICV3_ITS_COLLECTION_TABLE_SIZE ((size_t)64 * 1024)

#define GICV3_ITS_POLL_RETRIES 1000000U

struct gicv3_its_region {
    void *vaddr;
    paddr_t paddr;
    size_t size;
};

static struct {
    bool initialized;
    vaddr_t regs_vaddr;
    size_t regs_size;
    struct gicv3_its_region cmd_queue;
    struct gicv3_its_region device_table;
    struct gicv3_its_region collection_table;
} gicv3_its;

static inline uint32_t gits_read32(uint32_t reg) {
    volatile uint32_t *ptr = (volatile uint32_t *)(gicv3_its.regs_vaddr + reg);
    return mmio_read32(ptr);
}

static inline void gits_write32(uint32_t reg, uint32_t val) {
    volatile uint32_t *ptr = (volatile uint32_t *)(gicv3_its.regs_vaddr + reg);
    mmio_write32(ptr, val);
}

static inline uint64_t gits_read64(uint32_t reg) {
    volatile uint64_t *ptr = (volatile uint64_t *)(gicv3_its.regs_vaddr + reg);
    return mmio_read64(ptr);
}

static inline void gits_write64(uint32_t reg, uint64_t val) {
    volatile uint64_t *ptr = (volatile uint64_t *)(gicv3_its.regs_vaddr + reg);
    mmio_write64(ptr, val);
}

static status_t gicv3_its_wait_ctlr_state(bool enabled) {
    for (uint32_t i = 0; i < GICV3_ITS_POLL_RETRIES; ++i) {
        uint32_t ctlr = gits_read32(GITS_CTLR);
        if (((ctlr & GITS_CTLR_ENABLED) != 0) == enabled) {
            return NO_ERROR;
        }
    }

    return ERR_TIMED_OUT;
}

static status_t gicv3_its_alloc_region(const char *name,
                                       size_t requested_size,
                                       struct gicv3_its_region *region) {
    size_t size = ROUND_UP(requested_size, PAGE_SIZE);
    void *vaddr = NULL;

    status_t ret = vmm_alloc_contiguous(vmm_get_kernel_aspace(),
                                        name,
                                        size,
                                        &vaddr,
                                        0,
                                        0,
                                        ARCH_MMU_FLAG_UNCACHED | ARCH_MMU_FLAG_PERM_NO_EXECUTE);
    if (ret != NO_ERROR) {
        return ret;
    }

    paddr_t paddr = vaddr_to_paddr(vaddr);
    if (paddr == 0) {
        return ERR_NOT_FOUND;
    }

    memset(vaddr, 0, size);

    region->vaddr = vaddr;
    region->paddr = paddr;
    region->size = size;

    return NO_ERROR;
}

static status_t gicv3_its_program_baser(unsigned int baser_index,
                                        unsigned int table_type,
                                        const struct gicv3_its_region *region) {
    if (!region || region->size < PAGE_SIZE) {
        return ERR_INVALID_ARGS;
    }

    uint64_t pages = region->size / PAGE_SIZE;
    if (pages == 0 || pages > 256) {
        return ERR_OUT_OF_RANGE;
    }

    uint64_t baser = gits_read64(GITS_BASER(baser_index));
    baser &= ~(GITS_BASER_VALID | GITS_ADDR_MASK | 0xffULL | GITS_BASER_TYPE_MASK);
    baser |= GITS_BASER_VALID;
    baser |= (region->paddr & GITS_ADDR_MASK);
    baser |= (pages - 1U) & 0xffULL;
    baser |= (((uint64_t)table_type << GITS_BASER_TYPE_SHIFT) & GITS_BASER_TYPE_MASK);
    gits_write64(GITS_BASER(baser_index), baser);

    return NO_ERROR;
}

status_t arm_gicv3_its_init(void) {
    if (arm_gics[0].its_count == 0) {
        return NO_ERROR;
    }

    if (arm_gics[0].its[0].vaddr == 0) {
        return ERR_NOT_FOUND;
    }

    if (arm_gics[0].its_count > 1) {
        dprintf(INFO, "GICv3 ITS: %zu ITS frames present, initializing frame 0 only\n",
                arm_gics[0].its_count);
    }

    memset(&gicv3_its, 0, sizeof(gicv3_its));
    gicv3_its.regs_vaddr = arm_gics[0].its[0].vaddr;
    gicv3_its.regs_size = arm_gics[0].its[0].size;

    status_t ret = gicv3_its_alloc_region("gicv3_its_cmdq",
                                          GICV3_ITS_CMD_QUEUE_SIZE,
                                          &gicv3_its.cmd_queue);
    if (ret != NO_ERROR) {
        return ret;
    }

    ret = gicv3_its_alloc_region("gicv3_its_devtab",
                                 GICV3_ITS_DEVICE_TABLE_SIZE,
                                 &gicv3_its.device_table);
    if (ret != NO_ERROR) {
        return ret;
    }

    ret = gicv3_its_alloc_region("gicv3_its_coltab",
                                 GICV3_ITS_COLLECTION_TABLE_SIZE,
                                 &gicv3_its.collection_table);
    if (ret != NO_ERROR) {
        return ret;
    }

    /* Disable ITS before reprogramming aperture registers. */
    gits_write32(GITS_CTLR, 0);
    ret = gicv3_its_wait_ctlr_state(false);
    if (ret != NO_ERROR) {
        return ret;
    }

    uint64_t cmdq_pages = gicv3_its.cmd_queue.size / PAGE_SIZE;
    uint64_t cbaser = GITS_CBASER_VALID |
                      (gicv3_its.cmd_queue.paddr & GITS_ADDR_MASK) |
                      ((cmdq_pages - 1U) & 0xffULL);
    gits_write64(GITS_CBASER, cbaser);
    gits_write64(GITS_CWRITER, 0);

    ret = gicv3_its_program_baser(0, GITS_BASER_TYPE_DEVICE, &gicv3_its.device_table);
    if (ret != NO_ERROR) {
        return ret;
    }

    ret = gicv3_its_program_baser(1, GITS_BASER_TYPE_COLLECTION, &gicv3_its.collection_table);
    if (ret != NO_ERROR) {
        return ret;
    }

    smp_wmb();
    gits_write32(GITS_CTLR, GITS_CTLR_ENABLED);
    ret = gicv3_its_wait_ctlr_state(true);
    if (ret != NO_ERROR) {
        return ret;
    }

    gicv3_its.initialized = true;

    dprintf(INFO,
            "GICv3 ITS: aperture vaddr %#" PRIxPTR " cmdq pa %#" PRIxPTR " (%zu bytes)"
            " devtab pa %#" PRIxPTR " coltab pa %#" PRIxPTR " typer %#" PRIx64 "\n",
            gicv3_its.regs_vaddr,
            (uintptr_t)gicv3_its.cmd_queue.paddr,
            gicv3_its.cmd_queue.size,
            (uintptr_t)gicv3_its.device_table.paddr,
            (uintptr_t)gicv3_its.collection_table.paddr,
            gits_read64(GITS_TYPER));

    return NO_ERROR;
}
