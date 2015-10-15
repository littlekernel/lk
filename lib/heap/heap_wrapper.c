/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <string.h>
#include <list.h>
#include <kernel/spinlock.h>
#include <lib/console.h>
#include <lib/miniheap.h>

#define LOCAL_TRACE 0

struct list_node delayed_free_list = LIST_INITIAL_VALUE(delayed_free_list);
spin_lock_t delayed_free_lock = SPIN_LOCK_INITIAL_VALUE;

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
    LTRACEF("size %zd, align %d\n", size, alignment);

    // deal with the pending free list
    if (unlikely(!list_is_empty(&delayed_free_list))) {
        heap_free_delayed_list();
    }

    return miniheap_alloc(size, alignment);
}

void heap_free(void *ptr)
{
    LTRACEF("ptr %p\n", ptr);

    miniheap_free(ptr);
}

void heap_init(void)
{
    miniheap_init();
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
    miniheap_dump();

    printf("\tdelayed free list:\n");
    spin_lock_saved_state_t state;
    spin_lock_irqsave(&delayed_free_lock, state);
    struct list_node *node;
    list_for_every(&delayed_free_list, node) {
        printf("\t\tnode %p\n", node);
    }
    spin_unlock_irqrestore(&delayed_free_lock, state);
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
        heap_dump(); // XXX
    } else if (strcmp(argv[1].str, "alloc") == 0) {
        if (argc < 3) goto notenoughargs;

        void *ptr = heap_alloc(argv[2].u, (argc >= 3) ? argv[3].u : 0);
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


