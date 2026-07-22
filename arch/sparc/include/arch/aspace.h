/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>
#include <arch/sparc/mmu.h>

__BEGIN_CDECLS

struct arch_aspace {
    int magic;
    uint context;        // SPMMU context number

    uint flags;
    vaddr_t base;
    size_t size;
};

#define SPARC_ASPACE_MAGIC 'SPAS'

__END_CDECLS
