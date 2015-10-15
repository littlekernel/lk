/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
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
#include <lib/heap.h>

#include <trace.h>
#include <debug.h>
#include <assert.h>
#include <string.h>
#include <err.h>
#include <list.h>
#include <kernel/spinlock.h>
#include <lib/console.h>

#define LOCAL_TRACE 1

/* delayed free list */
struct list_node delayed_free_list = LIST_INITIAL_VALUE(delayed_free_list);
spin_lock_t delayed_free_lock = SPIN_LOCK_INITIAL_VALUE;

#if WITH_LIB_HEAP_MINIHEAP
#include <lib/miniheap.h>

#define HEAP_ALLOC miniheap_alloc
#define HEAP_FREE miniheap_free
static inline void HEAP_INIT(void) { miniheap_init(NULL, 0); }
#define HEAP_DUMP miniheap_dump

#elif WITH_LIB_HEAP_DLMALLOC
#include <lib/dlmalloc.h>

static inline void *HEAP_ALLOC(size_t size, unsigned int alignment) {
    if (alignment == 0)
        return dlmalloc(size);
    else
        return dlmemalign(alignment, size);
}

static inline void HEAP_FREE(void *ptr) { dlfree(ptr); }
static inline void HEAP_INIT(void) {}
static inline void HEAP_DUMP(void) {}

void *dl_sbrk(int incr) {
    LTRACEF("incr %d\n", incr);

    panic("what");
}

#else
#error need to select valid heap implementation or provide wrapper
#endif

#if WITH_KERNEL_VM

#include <kernel/vm.h>

/* we will use kalloc routines to back our heap */
#if !defined(HEAP_GROW_SIZE)
#define HEAP_GROW_SIZE (64 * 1024) /* size the heap grows by when it runs out of memory */
#endif

STATIC_ASSERT(IS_PAGE_ALIGNED(HEAP_GROW_SIZE));

#elif WITH_STATIC_HEAP

#if !defined(HEAP_START) || !defined(HEAP_LEN)
#error WITH_STATIC_HEAP set but no HEAP_START or HEAP_LEN defined
#endif

#else
/* not a static vm, not using the kernel vm */
extern int _end;
extern int _end_of_ram;

/* default to using up the rest of memory after the kernel ends */
/* may be modified by other parts of the system */
uintptr_t _heap_start = (uintptr_t)&_end;
uintptr_t _heap_end = (uintptr_t)&_end_of_ram;

#define HEAP_START ((uintptr_t)_heap_start)
#define HEAP_LEN ((uintptr_t)_heap_end - HEAP_START)
#endif

static void heap_free_delayed_list(void)
{
    struct list_node list;

    list_initialize(&list);

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&delayed_free_lock, state);

    struct list_node *node;
    while ((node = list_remove_head(&delayed_free_list))) {
        list_add_head(&list, node);
    }
    spin_unlock_irqrestore(&delayed_free_lock, state);

    while ((node = list_remove_head(&list))) {
        LTRACEF("freeing node %p\n", node);
        heap_free(node);
    }
}

void *heap_alloc(size_t size, unsigned int alignment)
{
    LTRACEF("size %zd, align %u\n", size, alignment);

    // deal with the pending free list
    if (unlikely(!list_is_empty(&delayed_free_list))) {
        heap_free_delayed_list();
    }

    return HEAP_ALLOC(size, alignment);
}

void heap_free(void *ptr)
{
    LTRACEF("ptr %p\n", ptr);

    HEAP_FREE(ptr);
}

void heap_init(void)
{
    HEAP_INIT();
}

/* critical section time delayed free */
void heap_delayed_free(void *ptr)
{
    LTRACEF("ptr %p\n", ptr);

    /* throw down a structure on the free block */
    /* XXX assumes the free block is large enough to hold a list node */
    struct list_node *node = (struct list_node *)ptr;

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&delayed_free_lock, state);
    list_add_head(&delayed_free_list, node);
    spin_unlock_irqrestore(&delayed_free_lock, state);
}

static void heap_dump(void)
{
    HEAP_DUMP();

    printf("\tdelayed free list:\n");
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&delayed_free_lock, state);
    struct list_node *node;
    list_for_every(&delayed_free_list, node) {
        printf("\t\tnode %p\n", node);
    }
    spin_unlock_irqrestore(&delayed_free_lock, state);
}

/* called back from the heap implementation to allocate another block of memory */
ssize_t heap_grow_memory(void **ptr, size_t size)
{
    LTRACEF("ptr %p, size 0x%zx\n", ptr, size);

#if WITH_KERNEL_VM && !WITH_STATIC_HEAP
    size = ROUNDUP(size, PAGE_SIZE);
    LTRACEF("size now 0x%zx\n", size);

    *ptr = pmm_alloc_kpages(size / PAGE_SIZE, NULL);
    if (!*ptr) {
        TRACEF("failed to grow kernel heap by 0x%zx bytes\n", size);
        return ERR_NO_MEMORY;
    }
#else
    static bool have_asked_for_memory = false;

    if (have_asked_for_memory)
        return ERR_NO_MEMORY;

    // XXX dont return all of the range on the first call
    *ptr = (void *)HEAP_START;
    size = HEAP_LEN;
    have_asked_for_memory = true;
#endif

    LTRACEF("returning %p, size 0x%zx\n", *ptr, size);

    return size;
}

void heap_free_memory(void *ptr, size_t len)
{
    LTRACEF("ptr %p, len 0x%zx\n", ptr, len);

#if WITH_KERNEL_VM && !WITH_STATIC_HEAP
    DEBUG_ASSERT(IS_PAGE_ALIGNED((uintptr_t)ptr));
    DEBUG_ASSERT(IS_PAGE_ALIGNED(len));

    pmm_free_kpages(ptr, len / PAGE_SIZE);
#endif
}

#if 0
static void heap_test(void)
{
    void *ptr[16];

    ptr[0] = heap_alloc(8, 0);
    ptr[1] = heap_alloc(32, 0);
    ptr[2] = heap_alloc(7, 0);
    ptr[3] = heap_alloc(0, 0);
    ptr[4] = heap_alloc(98713, 0);
    ptr[5] = heap_alloc(16, 0);

    heap_free(ptr[5]);
    heap_free(ptr[1]);
    heap_free(ptr[3]);
    heap_free(ptr[0]);
    heap_free(ptr[4]);
    heap_free(ptr[2]);

    heap_dump();

    int i;
    for (i=0; i < 16; i++)
        ptr[i] = 0;

    for (i=0; i < 32768; i++) {
        unsigned int index = (unsigned int)rand() % 16;

        if ((i % (16*1024)) == 0)
            printf("pass %d\n", i);

//      printf("index 0x%x\n", index);
        if (ptr[index]) {
//          printf("freeing ptr[0x%x] = %p\n", index, ptr[index]);
            heap_free(ptr[index]);
            ptr[index] = 0;
        }
        unsigned int align = 1 << ((unsigned int)rand() % 8);
        ptr[index] = heap_alloc((unsigned int)rand() % 32768, align);
//      printf("ptr[0x%x] = %p, align 0x%x\n", index, ptr[index], align);

        DEBUG_ASSERT(((addr_t)ptr[index] % align) == 0);
//      heap_dump();
    }

    for (i=0; i < 16; i++) {
        if (ptr[i])
            heap_free(ptr[i]);
    }

    heap_dump();
}
#endif


#if LK_DEBUGLEVEL > 1
#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int cmd_heap(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("heap", "heap debug commands", &cmd_heap)
STATIC_COMMAND_END(heap);

static int cmd_heap(int argc, const cmd_args *argv)
{
    if (argc < 2) {
notenoughargs:
        printf("not enough arguments\n");
usage:
        printf("usage:\n");
        printf("\t%s info\n", argv[0].str);
        printf("\t%s alloc <size> [alignment]\n", argv[0].str);
        printf("\t%s free <address>\n", argv[0].str);
        return -1;
    }

    if (strcmp(argv[1].str, "info") == 0) {
        heap_dump();
    } else if (strcmp(argv[1].str, "alloc") == 0) {
        if (argc < 3) goto notenoughargs;

        void *ptr = heap_alloc(argv[2].u, (argc >= 4) ? argv[3].u : 0);
        printf("heap_alloc returns %p\n", ptr);
    } else if (strcmp(argv[1].str, "free") == 0) {
        if (argc < 2) goto notenoughargs;

        heap_free((void *)argv[2].u);
    } else {
        printf("unrecognized command\n");
        goto usage;
    }

    return 0;
}

#endif
#endif


