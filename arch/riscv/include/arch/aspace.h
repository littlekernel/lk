/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <lk/list.h>
#include <arch/riscv/mmu.h>

__BEGIN_CDECLS

struct arch_aspace {
    int magic;

    // pointer to the translation table
    paddr_t pt_phys;
    volatile riscv_pte_t *pt_virt;

    uint flags;

    // list of page tables allocated for this aspace
    struct list_node pt_list;

    // range of address space
    vaddr_t base;
    size_t size;
};

#define RISCV_ASPACE_MAGIC 'RVAS'

__END_CDECLS


