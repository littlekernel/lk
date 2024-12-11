/*
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>
#include <arch/x86/mmu.h>

__BEGIN_CDECLS

struct arch_aspace {
    /* pointer to the root page table */
    paddr_t cr3_phys;
    map_addr_t *cr3;

    uint flags;

    /* range of address space */
    vaddr_t base;
    size_t size;
};

__END_CDECLS

