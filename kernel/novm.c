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
#include <trace.h>
#include <stdlib.h>
#include <string.h>
#include <lk/init.h>
#include <err.h>

struct novm_arena {
    mutex_t lock;
    size_t pages;
    char *map;
    char *base;
    size_t size;
};

/* not a static vm, not using the kernel vm */
extern int _end;
extern int _end_of_ram;

#define MEM_START ((uintptr_t)&_end)
#define MEM_SIZE ((MEMBASE + MEMSIZE) - MEM_START)
#define DEFAULT_MAP_SIZE (MEMSIZE >> PAGE_SIZE_SHIFT)

static char allocation_map[DEFAULT_MAP_SIZE];

struct novm_arena mem_arena;
struct novm_arena sdram_arena;

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

static bool in_arena(struct novm_arena *n, void* p)
{
    char *ptr = (char *)p;
    char *base = n->base;
    return ptr >= base && ptr < base + n->size;
}

void novm_init_helper(
    struct novm_arena* n, uintptr_t heap_start,
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
    n->base = (char *)start;
    n->size = size;
}

void novm_init_preheap(uint level)
{
    novm_init_helper(&mem_arena, MEM_START, MEM_SIZE, allocation_map, DEFAULT_MAP_SIZE);

#ifdef SDRAM_BASE
#define SDRAM_MAP_SIZE (SDRAM_SIZE >> PAGE_SIZE_SHIFT)
    static char sdram_map[SDRAM_MAP_SIZE];
    novm_init_helper(&sdram_arena, SDRAM_BASE, SDRAM_SIZE, sdram_map, SDRAM_MAP_SIZE);
#endif

#if 0
    /* mark pieces of the novm as used */
    novm_alloc_specific_pages(&__data_start,
            ROUNDUP((uintptr_t)&_end - (uintptr_t)&__data_start, PAGE_SIZE) / PAGE_SIZE);
#endif
}

LK_INIT_HOOK(novm_preheap, &novm_init_preheap, LK_INIT_LEVEL_HEAP - 1);

void *novm_alloc_helper(struct novm_arena *n, size_t pages)
{
    if (pages > n->pages) return NULL;  // Unsigned types!
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
            return n->base + (i << PAGE_SIZE_SHIFT);
        }
    }
    mutex_release(&n->lock);
    return NULL;
}

void* novm_alloc_pages(size_t pages)
{
    void* result = novm_alloc_helper(&mem_arena, pages);
    if (result != NULL) return result;
#ifdef SDRAM_BASE
    return novm_alloc_helper(&sdram_arena, pages);
#endif
    return NULL;
}

void novm_free_pages(void* address, size_t pages)
{
#ifdef SDRAM_BASE
    struct novm_arena *n = in_arena(&mem_arena, address) ? &mem_arena : &sdram_arena;
#else
    struct novm_arena *n = &mem_arena;
#endif

    DEBUG_ASSERT(in_arena(n, address));

    size_t index = ((char *)address - (char*)(n->base)) >> PAGE_SIZE_SHIFT;
    char *map = n->map;

    mutex_acquire(&n->lock);
    for (size_t i = 0; i < pages; i++) map[index + i] = 0;
    mutex_release(&n->lock);
}

status_t novm_alloc_specific_pages(void *address, size_t pages)
{
    TRACEF("address %p, pages %zu\n", address, pages);

    struct novm_arena *n = in_arena(&mem_arena, address) ? &mem_arena : NULL;
#ifdef SDRAM_BASE
    if (!n)
        n = in_arena(&sdram_arena, address) ? &sdram_arena : NULL;
#endif
    if (!n)
        return ERR_NOT_FOUND;

    size_t index = ((char *)address - (char*)(n->base)) >> PAGE_SIZE_SHIFT;
    char *map = n->map;

    status_t err = NO_ERROR;

    mutex_acquire(&n->lock);
    for (size_t i = 0; i < pages; i++) {
        if (map[index + i] != 0) {
            err = ERR_NO_MEMORY;
            break;
        }
        map[index + i] = 1;
    }
    mutex_release(&n->lock);

    return err;
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
    if (argc < 2) {
notenoughargs:
        printf("not enough arguments\n");
usage:
        printf("usage:\n");
        printf("\t%s info\n", argv[0].str);
        printf("\t%s alloc <numberofpages>\n", argv[0].str);
        printf("\t%s free address <numberofpages>\n", argv[0].str);
        return -1;
    }

    if (strcmp(argv[1].str, "info") == 0) {
        novm_dump();
	} else if (strcmp(argv[1].str, "alloc") == 0) {
		if (argc < 3) goto notenoughargs;

		void *ptr = novm_alloc_pages(argv[2].u);
		printf("novm_alloc_pages returns %p\n", ptr);
	} else if (strcmp(argv[1].str, "free") == 0) {
		if (argc < 3) goto notenoughargs;
		size_t pages = (argc >= 4) ? argv[3].u : 1;
		novm_free_pages(argv[2].p, pages);
		printf("novm_free_pages: %zd pages at %p\n", pages, argv[2].p);
    } else {
        printf("unrecognized command\n");
        goto usage;
    }

    return 0;
}

static void novm_dump_area(struct novm_arena *n)
{
    mutex_acquire(&n->lock);
    printf("  %d pages, each %zdk (%zdk in all)\n", n->pages, PAGE_SIZE >> 10, (PAGE_SIZE * n->pages) >> 10);
    printf("  %p-%p\n", (void *)n->base, (char *)n->base + n->size);
    unsigned i;
    size_t in_use = 0;
    for (i = 0; i < n->pages; i++) if (n->map[i] != 0) in_use++;
    printf("  %zd/%zd in use\n", in_use, n->pages);
#define MAX_PRINT 1024u
    for (i = 0; i < MAX_PRINT && i < n->pages; i++) {
        if ((i & 63) == 0) printf("    ");
        printf("%c", n->map[i] ? '*' : '.');
        if ((i & 63) == 63) printf("\n");
    }
    if (i == MAX_PRINT && n->pages > MAX_PRINT) {
        printf("    etc., %zd more pages.", n->pages - MAX_PRINT);
    }
    printf("\n");
    mutex_release(&n->lock);
}

static void novm_dump(void)
{
    printf("main memory arena:\n");
    novm_dump_area(&mem_arena);
#ifdef SDRAM_BASE
    printf("SDRAM arena:\n");
    novm_dump_area(&sdram_arena);
#endif
}

#endif
#endif

