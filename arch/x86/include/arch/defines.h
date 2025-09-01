/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define PAGE_SIZE_SHIFT 12
#define PAGE_SIZE       (1UL << PAGE_SIZE_SHIFT)

#define CACHE_LINE 64

#define ARCH_DEFAULT_STACK_SIZE (PAGE_SIZE * 2)
#define DEFAULT_TSS             PAGE_SIZE

/* based on how start.S sets up the physmap */
#if ARCH_X86_64
#define PHYSMAP_SIZE (64ULL * 1024 * 1024 * 1024)
#elif X86_LEGACY
/* Only map the first 16MB on legacy x86 due to page table usage
 * due to lack of 4MB pages. */
#define PHYSMAP_SIZE (16ULL * 1024 * 1024)
#elif ARCH_X86_32
/* Map 1GB by default for x86-32 */
#define PHYSMAP_SIZE (1ULL * 1024 * 1024 * 1024)
#endif
