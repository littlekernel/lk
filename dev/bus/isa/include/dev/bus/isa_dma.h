/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

// The PC/AT's first 8237 DMA controller: the 8 bit channels 0-3 (2 is the floppy). A
// transfer must sit below 16MB and must not cross a 64KB boundary, since the page
// register supplies the top address bits and the 8237 itself only counts 16. Callers
// keep one bounce buffer that satisfies both, allocated once at init:
//
//   paddr_t pa;
//   // 32KB aligned to 32KB never crosses a 64KB boundary
//   if (pmm_alloc_contiguous(8, 15, &pa, &list) != 8) ...
//   isa_dma_start(2, pa, len, ISA_DMA_TO_MEMORY);
//   ... device transfers, raises its irq ...
//   isa_dma_stop(2);

__BEGIN_CDECLS

enum isa_dma_dir {
    ISA_DMA_TO_MEMORY,   // device to memory (an 8237 "write" transfer)
    ISA_DMA_FROM_MEMORY, // memory to device (a "read" transfer)
};

// program and unmask a single transfer of len bytes (1 to 65536) on channel 0-3
status_t isa_dma_start(uint channel, paddr_t addr, size_t len, enum isa_dma_dir dir);

// mask the channel again
void isa_dma_stop(uint channel);

// true if [addr, addr + len) is usable for an ISA DMA transfer
bool isa_dma_range_ok(paddr_t addr, size_t len);

__END_CDECLS
