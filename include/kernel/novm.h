/*
 * Copyright (c) 2015 Google, Inc. All rights reserved
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
#ifndef __KERNEL_NOVM_H
#define __KERNEL_NOVM_H

#include <stddef.h>
#include <stdlib.h>
#include <arch.h>

#define PAGE_ALIGN(x) ALIGN(x, PAGE_SIZE)
#define IS_PAGE_ALIGNED(x) IS_ALIGNED(x, PAGE_SIZE)

// arena bitmaps for novm_alloc_pages
#define NOVM_ARENA_ANY (UINT32_MAX)
#define NOVM_ARENA_MAIN (1<<0)
#define NOVM_ARENA_SECONDARY (~NOVM_ARENA_MAIN)

void *novm_alloc_pages(size_t pages, uint32_t arena_bitmap);
void novm_free_pages(void *address, size_t pages);
status_t novm_alloc_specific_pages(void *address, size_t pages);

// You can call this once and it will give you some possibly unaligned memory
// that would otherwise go to waste.  The memory can't be freed.
void *novm_alloc_unaligned(size_t *size_return);

void novm_add_arena(const char *name, uintptr_t arena_start, uintptr_t arena_size);

struct page_range {
    void* address;
    size_t size;
};

int novm_get_arenas(struct page_range* ranges, int number_of_ranges);

#endif
