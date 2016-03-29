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
#include <pow2.h>
#include <lib/console.h>
#include <kernel/mutex.h>

#define LOCAL_TRACE 0

static struct list_node arena_list = LIST_INITIAL_VALUE(arena_list);
static mutex_t lock = MUTEX_INITIAL_VALUE(lock);

#define PAGE_BELONGS_TO_ARENA(page, arena) \
    (((uintptr_t)(page) >= (uintptr_t)(arena)->page_array) && \
     ((uintptr_t)(page) < ((uintptr_t)(arena)->page_array + (arena)->size / PAGE_SIZE * sizeof(vm_page_t))))

#define PAGE_ADDRESS_FROM_ARENA(page, arena) \
    (paddr_t)(((uintptr_t)page - (uintptr_t)a->page_array) / sizeof(vm_page_t)) * PAGE_SIZE + a->base;

#define ADDRESS_IN_ARENA(address, arena) \
    ((address) >= (arena)->base && (address) <= (arena)->base + (arena)->size - 1)

static inline bool page_is_free(const vm_page_t *page)
{
    return !(page->flags & VM_PAGE_FLAG_NONFREE);
}

paddr_t vm_page_to_paddr(const vm_page_t *page)
{
    pmm_arena_t *a;
    list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
        if (PAGE_BELONGS_TO_ARENA(page, a)) {
            return PAGE_ADDRESS_FROM_ARENA(page, a);
        }
    }
    return -1;
}

vm_page_t *paddr_to_vm_page(paddr_t addr)
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
    LTRACEF("arena %p name '%s' base 0x%lx size 0x%zx\n", arena, arena->name, arena->base, arena->size);

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

size_t pmm_alloc_pages(uint count, struct list_node *list)
{
    LTRACEF("count %u\n", count);

    /* list must be initialized prior to calling this */
    DEBUG_ASSERT(list);

    uint allocated = 0;
    if (count == 0)
        return 0;

    mutex_acquire(&lock);

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
    mutex_release(&lock);
    return allocated;
}

size_t pmm_alloc_range(paddr_t address, uint count, struct list_node *list)
{
    LTRACEF("address 0x%lx, count %u\n", address, count);

    DEBUG_ASSERT(list);

    uint allocated = 0;
    if (count == 0)
        return 0;

    address = ROUNDDOWN(address, PAGE_SIZE);

    mutex_acquire(&lock);

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

    mutex_release(&lock);
    return allocated;
}

size_t pmm_free(struct list_node *list)
{
    LTRACEF("list %p\n", list);

    DEBUG_ASSERT(list);

    mutex_acquire(&lock);

    uint count = 0;
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

    mutex_release(&lock);
    return count;
}

size_t pmm_free_page(vm_page_t *page)
{
    struct list_node list;
    list_initialize(&list);

    list_add_head(&list, &page->node);

    return pmm_free(&list);
}

/* physically allocate a run from arenas marked as KMAP */
void *pmm_alloc_kpages(uint count, struct list_node *list)
{
    LTRACEF("count %u\n", count);

    // XXX do fast path for single page


    paddr_t pa;
    size_t alloc_count = pmm_alloc_contiguous(count, PAGE_SIZE_SHIFT, &pa, list);
    if (alloc_count == 0)
        return NULL;

    return paddr_to_kvaddr(pa);
}

size_t pmm_free_kpages(void *_ptr, uint count)
{
    LTRACEF("ptr %p, count %u\n", _ptr, count);

    uint8_t *ptr = (uint8_t *)_ptr;

    struct list_node list;
    list_initialize(&list);

    while (count > 0) {
        vm_page_t *p = paddr_to_vm_page(vaddr_to_paddr(ptr));
        if (p) {
            list_add_tail(&list, &p->node);
        }

        ptr += PAGE_SIZE;
        count--;
    }

    return pmm_free(&list);
}

size_t pmm_alloc_contiguous(uint count, uint8_t alignment_log2, paddr_t *pa, struct list_node *list)
{
    LTRACEF("count %u, align %u\n", count, alignment_log2);

    if (count == 0)
        return 0;
    if (alignment_log2 < PAGE_SIZE_SHIFT)
        alignment_log2 = PAGE_SIZE_SHIFT;

    mutex_acquire(&lock);

    pmm_arena_t *a;
    list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
        // XXX make this a flag to only search kmap?
        if (a->flags & PMM_ARENA_FLAG_KMAP) {
            /* walk the list starting at alignment boundaries.
             * calculate the starting offset into this arena, based on the
             * base address of the arena to handle the case where the arena
             * is not aligned on the same boundary requested.
             */
            paddr_t rounded_base = ROUNDUP(a->base, 1UL << alignment_log2);
            if (rounded_base < a->base || rounded_base > a->base + a->size - 1)
                continue;

            uint aligned_offset = (rounded_base - a->base) / PAGE_SIZE;
            uint start = aligned_offset;
            LTRACEF("starting search at aligned offset %u\n", start);
            LTRACEF("arena base 0x%lx size %zu\n", a->base, a->size);

retry:
            /* search while we're still within the arena and have a chance of finding a slot
               (start + count < end of arena) */
            while ((start < a->size / PAGE_SIZE) &&
                    ((start + count) <= a->size / PAGE_SIZE)) {
                vm_page_t *p = &a->page_array[start];
                for (uint i = 0; i < count; i++) {
                    if (p->flags & VM_PAGE_FLAG_NONFREE) {
                        /* this run is broken, break out of the inner loop.
                         * start over at the next alignment boundary
                         */
                        start = ROUNDUP(start - aligned_offset + i + 1, 1UL << (alignment_log2 - PAGE_SIZE_SHIFT)) + aligned_offset;
                        goto retry;
                    }
                    p++;
                }

                /* we found a run */
                LTRACEF("found run from pn %u to %u\n", start, start + count);

                /* remove the pages from the run out of the free list */
                for (uint i = start; i < start + count; i++) {
                    p = &a->page_array[i];
                    DEBUG_ASSERT(!(p->flags & VM_PAGE_FLAG_NONFREE));
                    DEBUG_ASSERT(list_in_list(&p->node));

                    list_delete(&p->node);
                    p->flags |= VM_PAGE_FLAG_NONFREE;
                    a->free_count--;

                    if (list)
                        list_add_tail(list, &p->node);
                }

                if (pa)
                    *pa = a->base + start * PAGE_SIZE;

                mutex_release(&lock);

                return count;
            }
        }
    }

    mutex_release(&lock);

    LTRACEF("couldn't find run\n");
    return 0;
}

static void dump_page(const vm_page_t *page)
{
    printf("page %p: address 0x%lx flags 0x%x\n", page, vm_page_to_paddr(page), page->flags);
}

static void dump_arena(const pmm_arena_t *arena, bool dump_pages)
{
    printf("arena %p: name '%s' base 0x%lx size 0x%zx priority %u flags 0x%x\n",
           arena, arena->name, arena->base, arena->size, arena->priority, arena->flags);
    printf("\tpage_array %p, free_count %zu\n",
           arena->page_array, arena->free_count);

    /* dump all of the pages */
    if (dump_pages) {
        for (size_t i = 0; i < arena->size / PAGE_SIZE; i++) {
            dump_page(&arena->page_array[i]);
        }
    }

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
        printf("%s alloc_contig <count> <alignment>\n", argv[0].str);
        printf("%s dump_alloced\n", argv[0].str);
        printf("%s free_alloced\n", argv[0].str);
        return ERR_GENERIC;
    }

    static struct list_node allocated = LIST_INITIAL_VALUE(allocated);

    if (!strcmp(argv[1].str, "arenas")) {
        pmm_arena_t *a;
        list_for_every_entry(&arena_list, a, pmm_arena_t, node) {
            dump_arena(a, false);
        }
    } else if (!strcmp(argv[1].str, "alloc")) {
        if (argc < 3) goto notenoughargs;

        struct list_node list;
        list_initialize(&list);

        uint count = pmm_alloc_pages(argv[2].u, &list);
        printf("alloc returns %u\n", count);

        vm_page_t *p;
        list_for_every_entry(&list, p, vm_page_t, node) {
            printf("\tpage %p, address 0x%lx\n", p, vm_page_to_paddr(p));
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
        list_initialize(&list);

        uint count = pmm_alloc_range(argv[2].u, argv[3].u, &list);
        printf("alloc returns %u\n", count);

        vm_page_t *p;
        list_for_every_entry(&list, p, vm_page_t, node) {
            printf("\tpage %p, address 0x%lx\n", p, vm_page_to_paddr(p));
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
    } else if (!strcmp(argv[1].str, "alloc_contig")) {
        if (argc < 4) goto notenoughargs;

        struct list_node list;
        list_initialize(&list);

        paddr_t pa;
        size_t ret = pmm_alloc_contiguous(argv[2].u, argv[3].u, &pa, &list);
        printf("pmm_alloc_contiguous returns %zu, address 0x%lx\n", ret, pa);
        printf("address %% align = 0x%lx\n", pa % argv[3].u);

        /* add the pages to the local allocated list */
        struct list_node *node;
        while ((node = list_remove_head(&list))) {
            list_add_tail(&allocated, node);
        }
    } else if (!strcmp(argv[1].str, "free_alloced")) {
        size_t err = pmm_free(&allocated);
        printf("pmm_free returns %zu\n", err);
    } else {
        printf("unknown command\n");
        goto usage;
    }

    return NO_ERROR;
}

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 0
STATIC_COMMAND("pmm", "physical memory manager", &cmd_pmm)
#endif
STATIC_COMMAND_END(pmm);




