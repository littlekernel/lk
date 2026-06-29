/*
 * Copyright (c) 2015 Google, Inc. All rights reserved
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <kernel/mutex.h>
#include <lib/cmpctmalloc.h>
#include <stddef.h>
#include <stdint.h>

#define HEAP_ALIGN       (sizeof(void *) * 2)
#define HEAP_ALIGN_SHIFT (sizeof(void *) == 8 ? 4 : 3)

#define HEAP_ALLOC_VIRTUAL_BITS 22

// Buckets for allocations.  The smallest 15 buckets are HEAP_ALIGN-spaced up to
// 15 * HEAP_ALIGN bytes. After that we round up to the nearest size that can be written
// /^0*1...0*$/, giving 8 buckets per order of binary magnitude.  The freelist
// entries in a given bucket have at least the given size, plus the header
// size.
// The logarithmic rows start at row HEAP_ALIGN_SHIFT + 1. Since the maximum allocation
// size is limited by HEAP_ALLOC_VIRTUAL_BITS, the number of logarithmic rows is
// HEAP_ALLOC_VIRTUAL_BITS - (HEAP_ALIGN_SHIFT + 4).
// This dynamically allocates 136 buckets on 32-bit platforms and 128 buckets on 64-bit platforms.
#define NUMBER_OF_BUCKETS (1 + 15 + (HEAP_ALLOC_VIRTUAL_BITS - (HEAP_ALIGN_SHIFT + 4)) * 8)

// All individual memory areas on the heap start with this.
typedef struct header_struct {
    struct header_struct *left; // Pointer to the previous area in memory order.
    size_t size;
} header_t;

typedef struct free_struct {
    header_t header;
    struct free_struct *next;
    struct free_struct *prev;
} free_t;

struct heap {
    size_t size;
    size_t remaining;
    mutex_t lock;
    free_t *free_lists[NUMBER_OF_BUCKETS];
    // We have some 32 bit words that tell us whether there is an entry in the
    // freelist.
#define BUCKET_WORDS (((NUMBER_OF_BUCKETS) + 31) >> 5)
    uint32_t free_list_bits[BUCKET_WORDS];
};

extern struct heap theheap;

int size_to_index_allocating(size_t size, size_t *rounded_up_out);
int size_to_index_freeing(size_t size);
