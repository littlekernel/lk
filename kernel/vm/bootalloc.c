/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <kernel/vm.h>
#include "vm_priv.h"

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <trace.h>

#define LOCAL_TRACE 0

/* cheezy allocator that chews up space just after the end of the kernel mapping */

/* track how much memory we've used */
extern int _end;

uintptr_t boot_alloc_start = (uintptr_t)&_end;
uintptr_t boot_alloc_end = (uintptr_t)&_end;

void *boot_alloc_mem(size_t len)
{
    uintptr_t ptr;

    ptr = ALIGN(boot_alloc_end, 8);
    boot_alloc_end = (ptr + ALIGN(len, 8));

    LTRACEF("len %zu, ptr %p\n", len, (void *)ptr);

    return (void *)ptr;
}

