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

#include <err.h>
#include <assert.h>
#include <trace.h>
#include <stdlib.h>
#include <string.h>
#include <lk/init.h>
#include <kernel/mutex.h>

#define LOCAL_TRACE 0

struct novm_arena {
    mutex_t lock;
    const char *name;
    size_t pages;
    char *map;
    char *base;
    size_t size;

    // We divide the memory up into pages.  If there is memory we can use before
    // the first aligned page address, then we record it here and the heap will use
    // it.
#define MINIMUM_USEFUL_UNALIGNED_SIZE 64
    void *unaligned_area;
    size_t unaligned_size;
};


/* not a static vm, not using the kernel vm */
extern int _end;
extern int _end_of_ram;

#define MEM_START ((uintptr_t)&_end)
#define MEM_SIZE ((MEMBASE + MEMSIZE) - MEM_START)
#define DEFAULT_MAP_SIZE (MEMSIZE >> PAGE_SIZE_SHIFT)

/* a static list of arenas */
#ifndef NOVM_MAX_ARENAS
#define NOVM_MAX_ARENAS 1
#endif
struct novm_arena arena[NOVM_MAX_ARENAS];

int novm_get_arenas(struct page_range* ranges, int number_of_ranges)
{
    int ranges_found = 0;
    for (int i = 0; i < number_of_ranges && i < NOVM_MAX_ARENAS; i++) {
        if (arena[i].pages > 0) ranges_found = i + 1;
        ranges[i].address = (void*)arena[i].base;
        ranges[i].size = arena[i].pages << PAGE_SIZE_SHIFT;
    }
    return ranges_found;
}

void *novm_alloc_unaligned(size_t *size_return)
{
    /* only do the unaligned thing in the first arena */
    if (arena[0].unaligned_area != NULL) {
        *size_return = arena[0].unaligned_size;
        void *result = arena[0].unaligned_area;
        arena[0].unaligned_area = NULL;
        arena[0].unaligned_size = 0;
        return result;
    }
    *size_return = PAGE_SIZE;
    return novm_alloc_pages(1, NOVM_ARENA_ANY);
}

static bool in_arena(struct novm_arena *n, void *p)
{
    if (n->size == 0)
        return false;

    char *ptr = (char *)p;
    char *base = n->base;
    return ptr >= base && ptr < base + n->size;
}

static void novm_init_helper(struct novm_arena *n, const char *name,
                             uintptr_t arena_start, uintptr_t arena_size,
                             char *default_map, size_t default_map_size)
{
    uintptr_t start = ROUNDUP(arena_start, PAGE_SIZE);
    uintptr_t size = ROUNDDOWN(arena_start + arena_size, PAGE_SIZE) - start;

    mutex_init(&n->lock);

    size_t map_size = size >> PAGE_SIZE_SHIFT;
    char *map = default_map;
    if (map == NULL || default_map_size < map_size) {
        // allocate the map out of the arena itself
        map = (char *)arena_start;

        // Grab enough map for 16Mbyte of arena each time around the loop.
        while (start - arena_start < map_size) {
            start += PAGE_SIZE;
            size -= PAGE_SIZE;
            map_size--;
        }

        if ((char *)start - (map + ROUNDUP(map_size, 4)) >= MINIMUM_USEFUL_UNALIGNED_SIZE) {
            n->unaligned_area = map + ROUNDUP(map_size, 4);
            n->unaligned_size = (char *)start - (map + ROUNDUP(map_size, 4));
        }
    } else if (start - arena_start >= MINIMUM_USEFUL_UNALIGNED_SIZE) {
        n->unaligned_area = (char *)arena_start;
        n->unaligned_size = start - arena_start;
    }
    n->name = name;
    n->map = map;
    memset(n->map, 0, map_size);
    n->pages = map_size;
    n->base = (char *)start;
    n->size = size;
}

void novm_add_arena(const char *name, uintptr_t arena_start, uintptr_t arena_size)
{
    for (uint i = 0; i < NOVM_MAX_ARENAS; i++) {
        if (arena[i].pages == 0) {
            novm_init_helper(&arena[i], name, arena_start, arena_size, NULL, 0);
            return;
        }
    }
    panic("novm_add_arena: too many arenas added, bump NOVM_MAX_ARENAS!\n");
}

static void novm_init(uint level)
{
    static char mem_allocation_map[DEFAULT_MAP_SIZE];
    novm_init_helper(&arena[0], "main", MEM_START, MEM_SIZE, mem_allocation_map, DEFAULT_MAP_SIZE);
}

LK_INIT_HOOK(novm, &novm_init, LK_INIT_LEVEL_PLATFORM_EARLY - 1);

void *novm_alloc_helper(struct novm_arena *n, size_t pages)
{
    if (pages == 0 || pages > n->pages)
        return NULL;

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

void *novm_alloc_pages(size_t pages, uint32_t arena_bitmap)
{
    LTRACEF("pages %zu\n", pages);

    /* allocate from any arena */
    for (uint i = 0; i < NOVM_MAX_ARENAS; i++) {
        if (arena_bitmap & (1U << i)) {
            void *result = novm_alloc_helper(&arena[i], pages);
            if (result)
                return result;
        }
    }

    return NULL;
}

void novm_free_pages(void *address, size_t pages)
{
    LTRACEF("address %p, pages %zu\n", address, pages);

    struct novm_arena *n = NULL;
    for (uint i = 0; i < NOVM_MAX_ARENAS; i++) {
        if (in_arena(&arena[i], address)) {
            n = &arena[i];
            break;
        }
    }
    if (!n)
        return;

    DEBUG_ASSERT(in_arena(n, address));

    size_t index = ((char *)address - (char *)(n->base)) >> PAGE_SIZE_SHIFT;
    char *map = n->map;

    mutex_acquire(&n->lock);
    for (size_t i = 0; i < pages; i++) map[index + i] = 0;
    mutex_release(&n->lock);
}

status_t novm_alloc_specific_pages(void *address, size_t pages)
{
    LTRACEF("address %p, pages %zu\n", address, pages);

    struct novm_arena *n = NULL;
    for (uint i = 0; i < NOVM_MAX_ARENAS; i++) {
        if (in_arena(&arena[i], address)) {
            n = &arena[i];
            break;
        }
    }
    if (!n)
        return ERR_NOT_FOUND;

    size_t index = ((char *)address - (char *)(n->base)) >> PAGE_SIZE_SHIFT;
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
        printf("\t%s alloc <numberofpages> [arena bitmap]\n", argv[0].str);
        printf("\t%s free <address> [numberofpages]\n", argv[0].str);
        return -1;
    }

    if (strcmp(argv[1].str, "info") == 0) {
        novm_dump();
    } else if (strcmp(argv[1].str, "alloc") == 0) {
        if (argc < 3) goto notenoughargs;

        uint32_t arena_bitmap = (argc >= 4) ? argv[3].u : NOVM_ARENA_ANY;
        void *ptr = novm_alloc_pages(argv[2].u, arena_bitmap);
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

static void novm_dump_arena(struct novm_arena *n)
{
    if (n->pages == 0) {
        return;
    }

    mutex_acquire(&n->lock);
    printf("name '%s', %d pages, each %zdk (%zdk in all)\n", n->name, n->pages, PAGE_SIZE >> 10, (PAGE_SIZE * n->pages) >> 10);
    printf("  range: %p-%p\n", (void *)n->base, (char *)n->base + n->size);
    printf("  unaligned range: %p-%p\n", n->unaligned_area, n->unaligned_area + n->unaligned_size);
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
    for (uint i = 0; i < NOVM_MAX_ARENAS; i++) {
        novm_dump_arena(&arena[i]);
    }
}

#endif
#endif

