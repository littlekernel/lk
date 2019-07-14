/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kernel/vm.h>

#include <lk/trace.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include "vm_priv.h"

#define LOCAL_TRACE 0

/* cheezy allocator that chews up space just after the end of the kernel mapping */

/* track how much memory we've used */
extern int _end;

uintptr_t boot_alloc_start = (uintptr_t) &_end;
uintptr_t boot_alloc_end = (uintptr_t) &_end;

void *boot_alloc_mem(size_t len) {
    uintptr_t ptr;

    ptr = ALIGN(boot_alloc_end, 8);
    boot_alloc_end = (ptr + ALIGN(len, 8));

    LTRACEF("len %zu, ptr %p\n", len, (void *)ptr);

    return (void *)ptr;
}

