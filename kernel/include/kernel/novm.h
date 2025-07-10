// Copyright (c) 2015 Google, Inc. All rights reserved
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <arch.h>
#include <stddef.h>
#include <stdlib.h>
#include <lk/compiler.h>

// This file defines the non-VM memory allocation API for LK.
// Used in platforms and architectures that have no MMU support.

__BEGIN_CDECLS

#define PAGE_ALIGN(x) ALIGN(x, PAGE_SIZE)
#define IS_PAGE_ALIGNED(x) IS_ALIGNED(x, PAGE_SIZE)

// arena bitmaps for novm_alloc_pages
#define NOVM_ARENA_ANY (UINT32_MAX)
#define NOVM_ARENA_MAIN (1<<0)
#define NOVM_ARENA_SECONDARY (~NOVM_ARENA_MAIN)

// Allocate a number of pages from the non-VM memory pool.
void *novm_alloc_pages(size_t pages, uint32_t arena_bitmap);

// Free a number of pages back to the non-VM memory pool.
void novm_free_pages(void *address, size_t pages);

// Allocate a specific number of pages at a specific address.
// Returns an error status if the allocation fails.
status_t novm_alloc_specific_pages(void *address, size_t pages);

// You can call this once and it will give you some possibly unaligned memory
// that would otherwise go to waste.  The memory can't be freed.
void *novm_alloc_unaligned(size_t *size_return);

// Add a new arena for non-VM memory allocation.
// The arena is identified by a name, start address, and size.
void novm_add_arena(const char *name, uintptr_t arena_start, uintptr_t arena_size);

struct page_range {
    void *address;
    size_t size;
};

// Return a copy of the list of arenas available for non-VM memory allocation.
// Returns the number of arenas filled in the provided ranges array.
int novm_get_arenas(struct page_range *ranges, int number_of_ranges);

__END_CDECLS
