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
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <list.h>
#include <kernel/spinlock.h>
#include <lib/console.h>
#include <lib/page_alloc.h>

#define LOCAL_TRACE 0

/* heap tracing */
#if LK_DEBUGLEVEL > 0
static bool heap_trace = false;
#else
#define heap_trace (false)
#endif

/* delayed free list */
struct list_node delayed_free_list = LIST_INITIAL_VALUE(delayed_free_list);
spin_lock_t delayed_free_lock = SPIN_LOCK_INITIAL_VALUE;

#if WITH_LIB_HEAP_MINIHEAP
/* miniheap implementation */
#include <lib/miniheap.h>

static inline void *HEAP_MALLOC(size_t s) { return miniheap_alloc(s, 0); }
static inline void *HEAP_REALLOC(void *ptr, size_t s) { return miniheap_realloc(ptr, s); }
static inline void *HEAP_MEMALIGN(size_t boundary, size_t s) { return miniheap_alloc(s, boundary); }
#define HEAP_FREE miniheap_free
static inline void *HEAP_CALLOC(size_t n, size_t s)
{
    size_t realsize = n * s;

    void *ptr = miniheap_alloc(realsize, 0);
    if (likely(ptr))
        memset(ptr, 0, realsize);
    return ptr;
}
static inline void HEAP_INIT(void)
{
    /* start the heap off with some spare memory in the page allocator */
    size_t len;
    void *ptr = page_first_alloc(&len);
    miniheap_init(ptr, len);
}
#define HEAP_DUMP miniheap_dump
#define HEAP_TRIM miniheap_trim

/* end miniheap implementation */
#elif WITH_LIB_HEAP_CMPCTMALLOC
/* cmpctmalloc implementation */
#include <lib/cmpctmalloc.h>

#define HEAP_MEMALIGN(boundary, s) cmpct_memalign(s, boundary)
#define HEAP_MALLOC cmpct_alloc
#define HEAP_REALLOC cmpct_realloc
#define HEAP_FREE cmpct_free
#define HEAP_INIT cmpct_init
#define HEAP_DUMP cmpct_dump
#define HEAP_TRIM cmpct_trim
static inline void *HEAP_CALLOC(size_t n, size_t s)
{
    size_t realsize = n * s;

    void *ptr = cmpct_alloc(realsize);
    if (likely(ptr))
        memset(ptr, 0, realsize);
    return ptr;
}

/* end cmpctmalloc implementation */
#elif WITH_LIB_HEAP_DLMALLOC
/* dlmalloc implementation */
#include <lib/dlmalloc.h>

#define HEAP_MALLOC(s) dlmalloc(s)
#define HEAP_CALLOC(n, s) dlcalloc(n, s)
#define HEAP_MEMALIGN(b, s) dlmemalign(b, s)
#define HEAP_REALLOC(p, s) dlrealloc(p, s)
#define HEAP_FREE(p) dlfree(p)
static inline void HEAP_INIT(void) {}

static inline void HEAP_DUMP(void)
{
    struct mallinfo minfo = dlmallinfo();

    printf("\tmallinfo (dlmalloc):\n");
    printf("\t\tarena space 0x%zx\n", minfo.arena);
    printf("\t\tfree chunks 0x%zx\n", minfo.ordblks);
    printf("\t\tspace in mapped regions 0x%zx\n", minfo.hblkhd);
    printf("\t\tmax total allocated 0x%zx\n", minfo.usmblks);
    printf("\t\ttotal allocated 0x%zx\n", minfo.uordblks);
    printf("\t\tfree 0x%zx\n", minfo.fordblks);
    printf("\t\treleasable space 0x%zx\n", minfo.keepcost);

    printf("\theap block list:\n");
    void dump_callback(void *start, void *end, size_t used_bytes, void *arg) {
        printf("\t\tstart %p end %p used_bytes %zu\n", start, end, used_bytes);
    }

    dlmalloc_inspect_all(&dump_callback, NULL);
}

static inline void HEAP_TRIM(void) { dlmalloc_trim(0); }

/* end dlmalloc implementation */
#else
#error need to select valid heap implementation or provide wrapper
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
        HEAP_FREE(node);
    }
}

void heap_init(void)
{
    HEAP_INIT();
}

void heap_trim(void)
{
    // deal with the pending free list
    if (unlikely(!list_is_empty(&delayed_free_list))) {
        heap_free_delayed_list();
    }

    HEAP_TRIM();
}

void *malloc(size_t size)
{
    LTRACEF("size %zd\n", size);

    // deal with the pending free list
    if (unlikely(!list_is_empty(&delayed_free_list))) {
        heap_free_delayed_list();
    }

    void *ptr = HEAP_MALLOC(size);
    if (heap_trace)
        printf("caller %p malloc %zu -> %p\n", __GET_CALLER(), size, ptr);
    return ptr;
}

void *memalign(size_t boundary, size_t size)
{
    LTRACEF("boundary %zu, size %zd\n", boundary, size);

    // deal with the pending free list
    if (unlikely(!list_is_empty(&delayed_free_list))) {
        heap_free_delayed_list();
    }

    void *ptr = HEAP_MEMALIGN(boundary, size);
    if (heap_trace)
        printf("caller %p memalign %zu, %zu -> %p\n", __GET_CALLER(), boundary, size, ptr);
    return ptr;
}

void *calloc(size_t count, size_t size)
{
    LTRACEF("count %zu, size %zd\n", count, size);

    // deal with the pending free list
    if (unlikely(!list_is_empty(&delayed_free_list))) {
        heap_free_delayed_list();
    }

    void *ptr = HEAP_CALLOC(count, size);
    if (heap_trace)
        printf("caller %p calloc %zu, %zu -> %p\n", __GET_CALLER(), count, size, ptr);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    LTRACEF("ptr %p, size %zd\n", ptr, size);

    // deal with the pending free list
    if (unlikely(!list_is_empty(&delayed_free_list))) {
        heap_free_delayed_list();
    }

    void *ptr2 = HEAP_REALLOC(ptr, size);
    if (heap_trace)
        printf("caller %p realloc %p, %zu -> %p\n", __GET_CALLER(), ptr, size, ptr2);
    return ptr2;
}

void free(void *ptr)
{
    LTRACEF("ptr %p\n", ptr);
    if (heap_trace)
        printf("caller %p free %p\n", __GET_CALLER(), ptr);

    HEAP_FREE(ptr);
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

static void heap_test(void)
{
#if WITH_LIB_HEAP_CMPCTMALLOC
    cmpct_test();
#else
    void *ptr[16];

    ptr[0] = HEAP_MALLOC(8);
    ptr[1] = HEAP_MALLOC(32);
    ptr[2] = HEAP_MALLOC(7);
    ptr[3] = HEAP_MALLOC(0);
    ptr[4] = HEAP_MALLOC(98713);
    ptr[5] = HEAP_MALLOC(16);

    HEAP_FREE(ptr[5]);
    HEAP_FREE(ptr[1]);
    HEAP_FREE(ptr[3]);
    HEAP_FREE(ptr[0]);
    HEAP_FREE(ptr[4]);
    HEAP_FREE(ptr[2]);

    HEAP_DUMP();

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
            HEAP_FREE(ptr[index]);
            ptr[index] = 0;
        }
        unsigned int align = 1 << ((unsigned int)rand() % 8);
        ptr[index] = HEAP_MEMALIGN(align, (unsigned int)rand() % 32768);
//      printf("ptr[0x%x] = %p, align 0x%x\n", index, ptr[index], align);

        DEBUG_ASSERT(((addr_t)ptr[index] % align) == 0);
//      heap_dump();
    }

    for (i=0; i < 16; i++) {
        if (ptr[i])
            HEAP_FREE(ptr[i]);
    }

    HEAP_DUMP();
#endif
}


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
        printf("\t%s trace\n", argv[0].str);
        printf("\t%s trim\n", argv[0].str);
        printf("\t%s alloc <size> [alignment]\n", argv[0].str);
        printf("\t%s realloc <ptr> <size>\n", argv[0].str);
        printf("\t%s free <address>\n", argv[0].str);
        return -1;
    }

    if (strcmp(argv[1].str, "info") == 0) {
        heap_dump();
    } else if (strcmp(argv[1].str, "test") == 0) {
        heap_test();
    } else if (strcmp(argv[1].str, "trace") == 0) {
        heap_trace = !heap_trace;
        printf("heap trace is now %s\n", heap_trace ? "on" : "off");
    } else if (strcmp(argv[1].str, "trim") == 0) {
        heap_trim();
    } else if (strcmp(argv[1].str, "alloc") == 0) {
        if (argc < 3) goto notenoughargs;

        void *ptr = memalign((argc >= 4) ? argv[3].u : 0, argv[2].u);
        printf("memalign returns %p\n", ptr);
    } else if (strcmp(argv[1].str, "realloc") == 0) {
        if (argc < 4) goto notenoughargs;

        void *ptr = realloc(argv[2].p, argv[3].u);
        printf("realloc returns %p\n", ptr);
    } else if (strcmp(argv[1].str, "free") == 0) {
        if (argc < 2) goto notenoughargs;

        free(argv[2].p);
    } else {
        printf("unrecognized command\n");
        goto usage;
    }

    return 0;
}

#endif
#endif


