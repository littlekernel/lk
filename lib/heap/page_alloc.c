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

#include <lib/page_alloc.h>

#include <debug.h>
#include <assert.h>
#include <string.h>
#include <trace.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#else
#include <kernel/novm.h>
#endif

/* A simple page-aligned wrapper around the pmm or novm implementation of
 * the underlying physical page allocator. Used by system heaps or any
 * other user that wants pages of memory but doesn't want to use LK
 * specific apis.
 */
#define LOCAL_TRACE 0

#if WITH_STATIC_HEAP

#error "fix static heap post page allocator and novm stuff"

#if !defined(HEAP_START) || !defined(HEAP_LEN)
#error WITH_STATIC_HEAP set but no HEAP_START or HEAP_LEN defined
#endif

#endif

void *page_alloc(size_t pages, int arena)
{
#if WITH_KERNEL_VM
    void *result = pmm_alloc_kpages(pages, NULL);
    return result;
#else
    void *result = novm_alloc_pages(pages, arena);
    return result;
#endif
}

void page_free(void *ptr, size_t pages)
{
#if WITH_KERNEL_VM
    DEBUG_ASSERT(IS_PAGE_ALIGNED((uintptr_t)ptr));

    pmm_free_kpages(ptr, pages);
#else
    novm_free_pages(ptr, pages);
#endif
}

int page_get_arenas(struct page_range *ranges, int number_of_ranges)
{
#if WITH_KERNEL_VM
    ranges[0].address = kvaddr_get_range(&ranges[0].size);
    return 1;
#else
    return novm_get_arenas(ranges, number_of_ranges);
#endif  // WITH_KERNEL_VM
}

void *page_first_alloc(size_t *size_return)
{
#if WITH_KERNEL_VM
    *size_return = PAGE_SIZE;
    return page_alloc(1, PAGE_ALLOC_ANY_ARENA);
#else
    return novm_alloc_unaligned(size_return);
#endif
}

#if LK_DEBUGLEVEL > 1
#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int cmd_page_alloc(int argc, const cmd_args *argv);
static void page_alloc_dump(void);

STATIC_COMMAND_START
STATIC_COMMAND("page_alloc", "page allocator debug commands", &cmd_page_alloc)
STATIC_COMMAND_END(page_alloc);

static int cmd_page_alloc(int argc, const cmd_args *argv)
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
        page_alloc_dump();
    } else {
        printf("unrecognized command\n");
        goto usage;
    }

    return 0;
}

static void page_alloc_dump(void)
{
#ifdef WITH_KERNEL_VM
    dprintf(INFO, "Page allocator is based on pmm\n");
#else
    dprintf(INFO, "Page allocator is based on novm\n");
#endif
}

#endif
#endif
