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

#include "kernel/novm.h"

#include <assert.h>
#include <kernel/mutex.h>
#include <stdlib.h>
#include <string.h>

#if WITH_STATIC_HEAP

#define DEFAULT_MAP_SIZE (HEAP_LEN >> PAGE_SIZE_SHIFT)

#else

/* not a static vm, not using the kernel vm */
extern int _end;
extern int _end_of_ram;

/* default to using up the rest of memory after the kernel ends */
/* may be modified by other parts of the system */

#define HEAP_START ((uintptr_t)&_end)
#define HEAP_LEN ((uintptr_t)&_end_of_ram - HEAP_START)

#define MAP_SIZE (HEAP_LEN >> PAGE_SIZE_SHIFT)
// The map is one byte per page, but we can't statically size that because
// the C compiler doesn't think the difference between two addresses is a
// compile time constant.
// Instead, the default bytemap starts with this many bytes, which covers a
// quarter megabyte if page size is 4k.  If that's not enough, we steal the
// first of the plentiful pages for the map.
#define DEFAULT_MAP_SIZE 64

#endif

static char allocation_map[DEFAULT_MAP_SIZE];

struct novm {
    mutex_t lock;
    size_t pages;
    char* map;
    char* heap_base;
    uintptr_t heap_size;
};

static bool in_heap(struct novm *n, void* p)
{
    char *ptr = (char *)p;
    char *base = n->heap_base;
    return ptr >= base && ptr < base + n->heap_size;
}

struct novm sram_heap;
struct novm sdram_heap;

// We divide the memory up into pages.  If there is memory we can use before
// the first aligned page address, then we record it here and the heap will use
// it.
#define MINIMUM_USEFUL_UNALIGNED_SIZE 64
static void* unaligned_area = NULL;
static size_t unaligned_size = 0;

void *novm_alloc_unaligned(size_t *size_return)
{
    if (unaligned_area != NULL) {
        *size_return = unaligned_size;
        void *result = unaligned_area;
        unaligned_area = NULL;
        return result;
    }
    *size_return = PAGE_SIZE;
    return novm_alloc_pages(1);
}

void novm_init_helper(
    struct novm* n, uintptr_t heap_start,
    uintptr_t heap_size, char* default_map, size_t default_map_size)
{
    uintptr_t start = ROUNDUP(heap_start, PAGE_SIZE);
    uintptr_t size = ROUNDDOWN(heap_start + heap_size, PAGE_SIZE) - start;
    mutex_init(&n->lock);
    size_t map_size = size >> PAGE_SIZE_SHIFT;
    char* map = default_map;
    if (map == NULL || default_map_size < map_size) {
        map = (char *)heap_start;
        // Grab enough map for 16Mbyte of heap each time around the loop.
        while (start - heap_start < map_size) {
            start += PAGE_SIZE;
            size -= PAGE_SIZE;
            map_size--;
        }
        if ((char *)start - (map + map_size) >= MINIMUM_USEFUL_UNALIGNED_SIZE) {
            unaligned_area = map + map_size;
            unaligned_size = (char *)start - (map + map_size);
        }
    } else if (start - heap_start >= MINIMUM_USEFUL_UNALIGNED_SIZE) {
        unaligned_area = (char *)heap_start;
        unaligned_size = start - heap_start;
    }
    n->map = map;
    memset(n->map, 0, map_size);
    n->pages = map_size;
    n->heap_base = (char *)start;
    n->heap_size = size;
}

void novm_init(void)
{
    novm_init_helper(&sram_heap, HEAP_START, HEAP_LEN, allocation_map, DEFAULT_MAP_SIZE);

#ifdef SDRAM_BASE
#define SDRAM_MAP_SIZE (SDRAM_SIZE >> PAGE_SIZE_SHIFT)
    static char sdram_map[SDRAM_MAP_SIZE];
    novm_init_helper(&sdram_heap, SDRAM_BASE, SDRAM_SIZE, sdram_map, SDRAM_MAP_SIZE);
#endif
}

void *novm_alloc_helper(struct novm *n, size_t pages)
{
    mutex_acquire(&n->lock);
    for (size_t i = 0; i <= n->pages - pages; i++) {
        bool found = true;
        for (size_t j = 0; j < pages; j++) {
            if (n->map[i + j] != 0) {
                i += j;
                found = false;
                break;
            }
        }
        if (found) {
            memset(n->map + i, 1, pages);
            mutex_release(&n->lock);
            return n->heap_base + (i << PAGE_SIZE_SHIFT);
        }
    }
    mutex_release(&n->lock);
    return NULL;
}

void* novm_alloc_pages(size_t pages)
{
    void* result = novm_alloc_helper(&sram_heap, pages);
    if (result != NULL) return result;
#ifdef SDRAM_BASE
    return novm_alloc_helper(&sdram_heap, pages);
#endif
    return NULL;
}


void novm_free_pages(void* address, size_t pages)
{
#ifdef SDRAM_BASE
    struct novm *n = in_heap(&sram_heap, address) ? &sram_heap : &sdram_heap;
#else
    struct novm *n = &sram_heap;
#endif

    DEBUG_ASSERT(in_heap(n, address));

    size_t index = ((char *)address - (char*)(n->heap_base)) >> PAGE_SIZE_SHIFT;
    char *map = n->map;

    mutex_acquire(&n->lock);
    for (size_t i = 0; i < pages; i++) map[index + i] = 0;
    mutex_release(&n->lock);
}

#if LK_DEBUGLEVEL > 1
#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int cmd_novm(int argc, const cmd_args *argv);
static void novm_dump(void);

STATIC_COMMAND_START
STATIC_COMMAND("novm", "page allocator (for devices without VM support) debug commands", &cmd_novm)
STATIC_COMMAND_END(novm);

static int cmd_novm(int argc, const cmd_args *argv)
{
    if (argc != 2) {
notenoughargs:
        printf("not enough arguments\n");
usage:
        printf("usage:\n");
        printf("\t%s info\n", argv[0].str);
        return -1;
    }

    if (strcmp(argv[1].str, "info") == 0) {
        novm_dump();
    } else {
        printf("unrecognized command\n");
        goto usage;
    }

    return 0;
}

static void novm_dump_area(struct novm *n)
{
    mutex_acquire(&n->lock);
    printf("  %d pages, each %zdk (%zdk in all)\n", n->pages, PAGE_SIZE >> 10, (PAGE_SIZE * n->pages) >> 10);
    printf("  %p-%p\n", (void *)n->heap_base, (char *)n->heap_base + n->heap_size);
#define MAX_PRINT 1024u
    unsigned i;
    for (i = 0; i < MAX_PRINT && i < n->pages; i++) {
        if ((i & 63) == 0) printf("    ");
        printf("%c", n->map[i] ? '*' : '.');
        if ((i & 63) == 63) printf("\n");
    }
    if (i == MAX_PRINT && n->pages > MAX_PRINT) printf("    ...");
    printf("\n");
    mutex_release(&n->lock);
}

static void novm_dump(void)
{
    printf("SRAM area:\n");
    novm_dump_area(&sram_heap);
#ifdef SDRAM_BASE
    printf("SDRAM area:\n");
    novm_dump_area(&sdram_heap);
#endif
}

#endif
#endif

/* vim: set ts=4 sw=4 noexpandtab: */

