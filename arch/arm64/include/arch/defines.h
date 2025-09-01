/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define SHIFT_4K  (12)
#define SHIFT_16K (14)
#define SHIFT_64K (16)

/* arm specific stuff */
#ifdef ARM64_LARGE_PAGESIZE_64K
#define PAGE_SIZE_SHIFT (SHIFT_64K)
#elif ARM64_LARGE_PAGESIZE_16K
#define PAGE_SIZE_SHIFT (SHIFT_16K)
#else
#define PAGE_SIZE_SHIFT (SHIFT_4K)
#endif
#define USER_PAGE_SIZE_SHIFT PAGE_SIZE_SHIFT

#define PAGE_SIZE      (1UL << PAGE_SIZE_SHIFT)
#define USER_PAGE_SIZE (1UL << USER_PAGE_SIZE_SHIFT)

// TODO: for all practical purposes the default should be 64
#if ARM64_CPU_CORTEX_A53 || ARM64_CPU_CORTEX_A57 || ARM64_CPU_CORTEX_A72
#define CACHE_LINE 64
#else
#define CACHE_LINE 32
#endif

#define ARCH_DEFAULT_STACK_SIZE PAGE_SIZE