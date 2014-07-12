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

#include <trace.h>
#include <assert.h>
#include <list.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <lib/console.h>

#define LOCAL_TRACE 1

static struct list_node arena_list = LIST_INITIAL_VALUE(arena_list);

#define PAGE_BELONGS_TO_ARENA(page, arena) \
    (((uintptr_t)(page) >= (uintptr_t)(arena)->page_array) && \
     ((uintptr_t)(page) < ((uintptr_t)(arena)->page_array + (arena)->size / PAGE_SIZE * sizeof(vm_page_t))))

#define PAGE_ADDRESS_FROM_ARENA(page, arena) \
    (paddr_t)(((uintptr_t)page - (uintptr_t)a->page_array) / sizeof(vm_page_t)) * PAGE_SIZE + a->base;

#define ADDRESS_IN_ARENA(address, arena) \
    ((address) >= (arena)->base && (address) <= (arena)->base + (arena)->size)

static inline bool page_is_free(const vm_page_t *page)
{
    return !(page->flags & VM_PAGE_FLAG_NONFREE);
}

paddr_t page_to_address(const vm_page_t *page)
{
    pmm_arena_t *a;
    list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
        if (PAGE_BELONGS_TO_ARENA(page, a)) {
            return PAGE_ADDRESS_FROM_ARENA(page, a);
        }
    }
    return -1;
}

vm_page_t *address_to_page(paddr_t addr)
{
    pmm_arena_t *a;
    list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
        if (addr >= a->base && addr <= a->base + a->size - 1) {
            size_t index = (addr - a->base) / PAGE_SIZE;
            return &a->page_array[index];
        }
    }
    return NULL;
}

status_t pmm_add_arena(pmm_arena_t *arena)
{
    LTRACEF("arena %p name '%s' base 0x%lx size 0x%x\n", arena, arena->name, arena->base, arena->size);

    DEBUG_ASSERT(IS_PAGE_ALIGNED(arena->base));
    DEBUG_ASSERT(IS_PAGE_ALIGNED(arena->size));
    DEBUG_ASSERT(arena->size > 0);

    /* walk the arena list and add arena based on priority order */
    pmm_arena_t *a;
    list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
        if (a->priority > arena->priority) {
            list_add_before(&a->node, &arena->node);
            goto done_add;
        }
    }

    /* walked off the end, add it to the end of the list */
    list_add_tail(&arena_list, &arena->node);

done_add:

    /* zero out some of the structure */
    arena->free_count = 0;
    list_initialize(&arena->free_list);

    /* allocate an array of pages to back this one */
    size_t page_count = arena->size / PAGE_SIZE;
    arena->page_array = boot_alloc_mem(page_count * sizeof(vm_page_t));

    /* initialize all of the pages */
    memset(arena->page_array, 0, page_count * sizeof(vm_page_t));

    /* add them to the free list */
    for (size_t i = 0; i < page_count; i++) {
        vm_page_t *p = &arena->page_array[i];

        list_add_tail(&arena->free_list, &p->node);

        arena->free_count++;
    }

    return NO_ERROR;
}

uint pmm_alloc_pages(uint count, struct list_node *list)
{
    LTRACEF("count %u\n", count);

    DEBUG_ASSERT(list);

    /* build a list of pages we have pulled from the arena(s) */
    list_initialize(list);

    uint allocated = 0;
    if (count == 0)
        return 0;

    /* walk the arenas in order, allocating as many pages as we can from each */
    pmm_arena_t *a;
    list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
        while (allocated < count) {
            vm_page_t *page = list_remove_head_type(&a->free_list, vm_page_t, node);
            if (!page)
                goto done;

            a->free_count--;

            page->flags |= VM_PAGE_FLAG_NONFREE;
            list_add_tail(list, &page->node);

            allocated++;
        }
    }

done:
    return allocated;
}

uint pmm_alloc_range(paddr_t address, uint count, struct list_node *list)
{
    LTRACEF("address 0x%lx, count %u\n", address, count);

    DEBUG_ASSERT(list);

    uint allocated = 0;
    if (count == 0)
        return 0;

    address = ROUNDDOWN(address, PAGE_SIZE);

    /* build a list of pages we have pulled from the arena(s) */
    list_initialize(list);

    /* walk through the arenas, looking to see if the physical page belongs to it */
    pmm_arena_t *a;
    list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
        while (allocated < count && ADDRESS_IN_ARENA(address, a)) {
            size_t index = (address - a->base) / PAGE_SIZE;

            DEBUG_ASSERT(index < a->size / PAGE_SIZE);

            vm_page_t *page = &a->page_array[index];
            if (page->flags & VM_PAGE_FLAG_NONFREE) {
                /* we hit an allocated page */
                break;
            }

            DEBUG_ASSERT(list_in_list(&page->node));

            list_delete(&page->node);
            page->flags |= VM_PAGE_FLAG_NONFREE;
            list_add_tail(list, &page->node);

            a->free_count--;
            allocated++;
            address += PAGE_SIZE;
        }

        if (allocated == count)
            break;
    }

    return allocated;
}

int pmm_free(struct list_node *list)
{
    LTRACEF("list %p\n", list);

    DEBUG_ASSERT(list);

    int count = 0;
    while (!list_is_empty(list)) {
        vm_page_t *page = list_remove_head_type(list, vm_page_t, node);

        DEBUG_ASSERT(!list_in_list(&page->node));
        DEBUG_ASSERT(page->flags & VM_PAGE_FLAG_NONFREE);

        /* see which arena this page belongs to and add it */
        pmm_arena_t *a;
        list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
            if (PAGE_BELONGS_TO_ARENA(page, a)) {
                page->flags &= ~VM_PAGE_FLAG_NONFREE;

                list_add_head(&a->free_list, &page->node);
                a->free_count++;
                count++;
                break;
            }
        }
    }

    return count;
}

/* physically allocate a run from arenas marked as KMAP */
void *pmm_alloc_kpages(uint count, struct list_node *list)
{
    LTRACEF("count %u\n", count);

    if (count == 0)
        return NULL;

    pmm_arena_t *a;
    list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
        if (a->flags & PMM_ARENA_FLAG_KMAP) {
            uint find_start = -1;
            for (uint i = 0; i < a->size / PAGE_SIZE; i++) {
                vm_page_t *p = &a->page_array[i];
                if (p->flags & VM_PAGE_FLAG_NONFREE) {
                    /* page was not free, reset the start counter */
                    find_start = -1;
                    continue;
                }

                if (find_start == (uint)-1) {
                    /* start of a new run */
                    find_start = i;
                }

                if (i - find_start == count - 1) {
                    /* we found a run */
                    LTRACEF("found run from pn %u to %u\n", find_start, i);

                    if (list)
                        list_initialize(list);

                    /* remove the pages from the run out of the free list */
                    for (uint j = find_start; j <= i; j++) {
                        p = &a->page_array[j];
                        DEBUG_ASSERT(list_in_list(&p->node));

                        list_delete(&p->node);
                        p->flags |= VM_PAGE_FLAG_NONFREE;
                        a->free_count--;

                        if (list)
                            list_add_tail(list, &p->node);
                    }

                    return paddr_to_kvaddr(a->base + find_start * PAGE_SIZE);
                }
            }
        }
    }

    LTRACEF("couldn't find run\n");
    return NULL;
}

static void dump_arena(const pmm_arena_t *arena)
{
    printf("arena %p: name '%s' base 0x%lx size 0x%x priority %u flags 0x%x\n",
           arena, arena->name, arena->base, arena->size, arena->priority, arena->flags);
    printf("\tpage_array %p, free_count %zu\n",
           arena->page_array, arena->free_count);

    /* dump the free pages */
    printf("\tfree ranges:\n");
    ssize_t last = -1;
    for (size_t i = 0; i < arena->size / PAGE_SIZE; i++) {
        if (page_is_free(&arena->page_array[i])) {
            if (last == -1) {
                last = i;
            }
        } else {
            if (last != -1) {
                printf("\t\t0x%lx - 0x%lx\n", arena->base + last * PAGE_SIZE, arena->base + i * PAGE_SIZE);
            }
            last = -1;
        }
    }

    if (last != -1) {
        printf("\t\t0x%lx - 0x%lx\n",  arena->base + last * PAGE_SIZE, arena->base + arena->size);
    }
}

static void dump_page(const vm_page_t *page)
{
    printf("page %p: address 0x%lx flags 0x%x\n", page, page_to_address(page), page->flags);
}

static int cmd_pmm(int argc, const cmd_args *argv)
{
    if (argc < 2) {
notenoughargs:
        printf("not enough arguments\n");
usage:
        printf("usage:\n");
        printf("%s arenas\n", argv[0].str);
        printf("%s alloc <count>\n", argv[0].str);
        printf("%s alloc_range <address> <count>\n", argv[0].str);
        printf("%s alloc_kpages <count>\n", argv[0].str);
        printf("%s dump_alloced\n", argv[0].str);
        printf("%s free_alloced\n", argv[0].str);
        return ERR_GENERIC;
    }

    static struct list_node allocated = LIST_INITIAL_VALUE(allocated);

    if (!strcmp(argv[1].str, "arenas")) {
        pmm_arena_t *a;
        list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
            dump_arena(a);
        }
    } else if (!strcmp(argv[1].str, "alloc")) {
        if (argc < 3) goto notenoughargs;

        struct list_node list;
        uint count = pmm_alloc_pages(argv[2].u, &list);
        printf("alloc returns %u\n", count);

        vm_page_t *p;
        list_for_every_entry(&list, p, vm_page_t, node) {
            printf("\tpage %p, address 0x%lx\n", p, page_to_address(p));
        }

        /* add the pages to the local allocated list */
        struct list_node *node;
        while ((node = list_remove_head(&list))) {
            list_add_tail(&allocated, node);
        }
    } else if (!strcmp(argv[1].str, "dump_alloced")) {
        vm_page_t *page;

        list_for_every_entry(&allocated, page, vm_page_t, node) {
            dump_page(page);
        }
    } else if (!strcmp(argv[1].str, "alloc_range")) {
        if (argc < 4) goto notenoughargs;

        struct list_node list;
        uint count = pmm_alloc_range(argv[2].u, argv[3].u, &list);
        printf("alloc returns %u\n", count);

        vm_page_t *p;
        list_for_every_entry(&list, p, vm_page_t, node) {
            printf("\tpage %p, address 0x%lx\n", p, page_to_address(p));
        }

        /* add the pages to the local allocated list */
        struct list_node *node;
        while ((node = list_remove_head(&list))) {
            list_add_tail(&allocated, node);
        }
    } else if (!strcmp(argv[1].str, "alloc_kpages")) {
        if (argc < 3) goto notenoughargs;

        void *ptr = pmm_alloc_kpages(argv[2].u, NULL);
        printf("pmm_alloc_kpages returns %p\n", ptr);
    } else if (!strcmp(argv[1].str, "free_alloced")) {
        int err = pmm_free(&allocated);
        printf("pmm_free returns %d\n", err);
    } else {
        printf("unknown command\n");
        goto usage;
    }

    return NO_ERROR;
}

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 0
{ "pmm", "physical memory manager", &cmd_pmm },
#endif
STATIC_COMMAND_END(pmm);




