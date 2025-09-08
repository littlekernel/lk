/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/m68k/mmu.h>

#include <sys/types.h>

struct arch_aspace {
    /* pointer to the translation table */
    paddr_t pgtable_phys;
    void *pgtable_virt;

    uint32_t flags;
};
