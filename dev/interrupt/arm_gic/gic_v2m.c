/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "gic_v2m.h"

#include <dev/interrupt/arm_gic.h>
#include <inttypes.h>
#include <kernel/spinlock.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform/gic.h>
#include <stdio.h>

#define LOCAL_TRACE 0

#define V2M_MSI_TYPER     0x008
#define V2M_MSI_SETSPI_NS 0x040

// MSI_TYPER: first SPI (an INTID, not an SPI index) in [25:16], number of SPIs in [9:0]
#define V2M_TYPER_BASE(t)  (((t) >> 16) & 0x3ff)
#define V2M_TYPER_COUNT(t) ((t) & 0x3ff)

struct v2m_frame {
    paddr_t paddr;
    uint spi_base;
    uint spi_count;
    uint64_t allocated[(0x3ff + 63) / 64]; // bit n is spi_base + n
};

static struct v2m_frame frames[ARM_GIC_MAX_MSI_FRAMES];
static size_t frame_count;
static spin_lock_t v2m_lock = SPIN_LOCK_INITIAL_VALUE;

status_t arm_gic_v2m_add_frame(paddr_t paddr, vaddr_t vaddr) {
    if (frame_count == ARM_GIC_MAX_MSI_FRAMES) {
        return ERR_NO_RESOURCES;
    }

    const uint32_t typer = mmio_read32((volatile uint32_t *)(vaddr + V2M_MSI_TYPER));
    uint base = V2M_TYPER_BASE(typer);
    uint count = V2M_TYPER_COUNT(typer);
    LTRACEF("frame %#" PRIxPTR " typer %#x: spis %u..%u\n", (uintptr_t)paddr, typer, base,
            base + count - 1);

    if (count == 0 || base < GIC_BASE_SPI || base >= MAX_INT) {
        printf("GICv2m: frame at %#" PRIxPTR " has no usable SPIs (typer %#x)\n", (uintptr_t)paddr,
               typer);
        return ERR_NOT_FOUND;
    }
    if (base + count > MAX_INT) {
        printf("GICv2m: frame at %#" PRIxPTR " SPIs %u..%u, using %u..%u (MAX_INT %u)\n",
               (uintptr_t)paddr, base, base + count - 1, base, MAX_INT - 1, MAX_INT);
        count = MAX_INT - base;
    }

    frames[frame_count++] = (struct v2m_frame){
        .paddr = paddr,
        .spi_base = base,
        .spi_count = count,
    };
    dprintf(INFO, "GICv2m: frame at %#" PRIxPTR ", SPIs %u..%u\n", (uintptr_t)paddr, base,
            base + count - 1);
    return NO_ERROR;
}

static struct v2m_frame *find_frame(paddr_t paddr) {
    for (size_t i = 0; i < frame_count; i++) {
        if (frames[i].paddr == paddr) {
            return &frames[i];
        }
    }
    return NULL;
}

status_t arm_gic_v2m_alloc(paddr_t paddr, unsigned int *vector, uint64_t *msg_addr,
                           uint32_t *msg_data) {
    struct v2m_frame *f = find_frame(paddr);
    if (!f) {
        return ERR_NOT_FOUND;
    }

    int found = -1;
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&v2m_lock);
    for (uint i = 0; i < f->spi_count; i++) {
        if ((f->allocated[i / 64] & (1ULL << (i % 64))) == 0) {
            f->allocated[i / 64] |= 1ULL << (i % 64);
            found = (int)i;
            break;
        }
    }
    spin_unlock_irqrestore(&v2m_lock, state);
    if (found < 0) {
        return ERR_NO_RESOURCES;
    }

    *vector = f->spi_base + (uint)found;
    *msg_addr = (uint64_t)f->paddr + V2M_MSI_SETSPI_NS;
    *msg_data = *vector;
    LTRACEF("frame %#" PRIxPTR " vector %u\n", (uintptr_t)paddr, *vector);
    return NO_ERROR;
}

status_t arm_gic_v2m_free(paddr_t paddr, unsigned int vector) {
    struct v2m_frame *f = find_frame(paddr);
    if (!f) {
        return ERR_NOT_FOUND;
    }
    if (vector < f->spi_base || vector - f->spi_base >= f->spi_count) {
        return ERR_INVALID_ARGS;
    }

    const uint i = vector - f->spi_base;
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&v2m_lock);
    f->allocated[i / 64] &= ~(1ULL << (i % 64));
    spin_unlock_irqrestore(&v2m_lock, state);
    return NO_ERROR;
}
