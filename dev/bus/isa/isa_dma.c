/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <dev/bus/isa_dma.h>

#include <arch/x86.h>
#include <kernel/spinlock.h>
#include <lk/err.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

// first (8 bit) controller
#define DMA1_SINGLE_MASK 0x0a
#define DMA1_MODE        0x0b
#define DMA1_CLEAR_FF    0x0c

#define DMA_MODE_SINGLE   0x40
#define DMA_MODE_READ     0x08 // memory to device
#define DMA_MODE_WRITE    0x04 // device to memory

static const uint8_t dma_addr_port[4] = { 0x00, 0x02, 0x04, 0x06 };
static const uint8_t dma_count_port[4] = { 0x01, 0x03, 0x05, 0x07 };
static const uint8_t dma_page_port[4] = { 0x87, 0x83, 0x81, 0x82 };

static spin_lock_t dma_lock = SPIN_LOCK_INITIAL_VALUE;

bool isa_dma_range_ok(paddr_t addr, size_t len) {
    if (len == 0 || len > 0x10000) {
        return false;
    }
    const uint64_t end = (uint64_t)addr + len - 1;
    return end < 0x1000000 && (addr >> 16) == (end >> 16);
}

status_t isa_dma_start(uint channel, paddr_t addr, size_t len, enum isa_dma_dir dir) {
    if (channel > 3) {
        return ERR_INVALID_ARGS;
    }
    if (!isa_dma_range_ok(addr, len)) {
        return ERR_OUT_OF_RANGE;
    }

    LTRACEF("channel %u addr %#lx len %zu dir %d\n", channel, (ulong)addr, len, dir);

    const uint8_t mode = DMA_MODE_SINGLE | (dir == ISA_DMA_TO_MEMORY ? DMA_MODE_WRITE : DMA_MODE_READ) |
                         channel;
    const uint16_t count = (uint16_t)(len - 1);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&dma_lock);

    outp(DMA1_SINGLE_MASK, 0x04 | channel);
    outp(DMA1_CLEAR_FF, 0);
    outp(DMA1_MODE, mode);
    outp(dma_addr_port[channel], addr & 0xff);
    outp(dma_addr_port[channel], (addr >> 8) & 0xff);
    outp(dma_page_port[channel], (addr >> 16) & 0xff);
    outp(DMA1_CLEAR_FF, 0);
    outp(dma_count_port[channel], count & 0xff);
    outp(dma_count_port[channel], count >> 8);
    outp(DMA1_SINGLE_MASK, channel);

    spin_unlock_irqrestore(&dma_lock, state);

    return NO_ERROR;
}

void isa_dma_stop(uint channel) {
    if (channel > 3) {
        return;
    }
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&dma_lock);
    outp(DMA1_SINGLE_MASK, 0x04 | channel);
    spin_unlock_irqrestore(&dma_lock, state);
}
