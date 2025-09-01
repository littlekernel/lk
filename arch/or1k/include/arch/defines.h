/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define PAGE_SIZE_SHIFT 13
#define PAGE_SIZE       (1U << PAGE_SIZE_SHIFT)

/* Cache line can be configured, but this is max */
#define CACHE_LINE 32

#define ARCH_DEFAULT_STACK_SIZE PAGE_SIZE
