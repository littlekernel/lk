/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define PAGE_SIZE_SHIFT 12
#define PAGE_SIZE       (1UL << PAGE_SIZE_SHIFT)

// XXX is this right?
#define CACHE_LINE 32

#if ARCH_RISCV_EMBEDDED
#define ARCH_DEFAULT_STACK_SIZE 1024
#else
#define ARCH_DEFAULT_STACK_SIZE PAGE_SIZE
#endif
