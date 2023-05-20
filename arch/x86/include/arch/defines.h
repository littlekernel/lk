/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define PAGE_SIZE 4096
#define PAGE_SIZE_SHIFT 12

#define CACHE_LINE 64

#define ARCH_DEFAULT_STACK_SIZE 8192
#define DEFAULT_TSS 4096

/* based on how start.S sets up the physmap */
#if ARCH_X86_64
#define PHYSMAP_SIZE (64ULL*GB)
#elif ARCH_X86_32
#define PHYSMAP_SIZE (1ULL*GB)
#endif

