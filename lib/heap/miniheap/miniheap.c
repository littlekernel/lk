/*
 * Copyright (c) 2008-2009,2012-2015 Travis Geiselbrecht
 * Copyright (c) 2009 Corey Tabaka
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
#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <err.h>
#include <list.h>
#include <rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/mutex.h>
#include <lib/miniheap.h>
#include <lib/heap.h>
#include <lib/page_alloc.h>

#define LOCAL_TRACE 0

#define DEBUG_HEAP 0
#define ALLOC_FILL 0x99
#define FREE_FILL 0x77
#define PADDING_FILL 0x55
#define PADDING_SIZE 64

// whether or not the heap will try to trim itself every time a free happens
#ifndef MINIHEAP_AUTOTRIM
#define MINIHEAP_AUTOTRIM 0
#endif

#define HEAP_MAGIC (0x48454150)  // 'HEAP'

struct free_heap_chunk {
    struct list_node node;
    size_t len;
};

struct heap {
    void *base;
    size_t len;
    size_t remaining;
    size_t low_watermark;
    mutex_t lock;
    struct list_node free_list;
};

// heap static vars
static struct heap theheap;

// structure placed at the beginning every allocation
struct alloc_struct_begin {
#if LK_DEBUGLEVEL > 1
    unsigned int magic;
#endif
    void *ptr;
    size_t size;
#if DEBUG_HEAP
    void *padding_start;
    size_t padding_size;
#endif
};

static ssize_t heap_grow(size_t len);

static void dump_free_chunk(struct free_heap_chunk *chunk)
{
    dprintf(INFO, "\t\tbase %p, end 0x%lx, len 0x%zx\n", chunk, (vaddr_t)chunk + chunk->len, chunk->len);
}

void miniheap_dump(void)
{
    dprintf(INFO, "Heap dump (using miniheap):\n");
    dprintf(INFO, "\tbase %p, len 0x%zx\n", theheap.base, theheap.len);
    dprintf(INFO, "\tfree list:\n");

    mutex_acquire(&theheap.lock);

    struct free_heap_chunk *chunk;
    list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
        dump_free_chunk(chunk);
    }
    mutex_release(&theheap.lock);

}

// try to insert this free chunk into the free list, consuming the chunk by merging it with
// nearby ones if possible. Returns base of whatever chunk it became in the list.
static struct free_heap_chunk *heap_insert_free_chunk(struct free_heap_chunk *chunk)
{
#if LK_DEBUGLEVEL > INFO
    vaddr_t chunk_end = (vaddr_t)chunk + chunk->len;
#endif

    LTRACEF("chunk ptr %p, size 0x%zx\n", chunk, chunk->len);

    struct free_heap_chunk *next_chunk;
    struct free_heap_chunk *last_chunk;

    mutex_acquire(&theheap.lock);

    theheap.remaining += chunk->len;

    // walk through the list, finding the node to insert before
    list_for_every_entry(&theheap.free_list, next_chunk, struct free_heap_chunk, node) {
        if (chunk < next_chunk) {
            DEBUG_ASSERT(chunk_end <= (vaddr_t)next_chunk);

            list_add_before(&next_chunk->node, &chunk->node);

            goto try_merge;
        }
    }

    // walked off the end of the list, add it at the tail
    list_add_tail(&theheap.free_list, &chunk->node);

    // try to merge with the previous chunk
try_merge:
    last_chunk = list_prev_type(&theheap.free_list, &chunk->node, struct free_heap_chunk, node);
    if (last_chunk) {
        if ((vaddr_t)last_chunk + last_chunk->len == (vaddr_t)chunk) {
            // easy, just extend the previous chunk
            last_chunk->len += chunk->len;

            // remove ourself from the list
            list_delete(&chunk->node);

            // set the chunk pointer to the newly extended chunk, in case
            // it needs to merge with the next chunk below
            chunk = last_chunk;
        }
    }

    // try to merge with the next chunk
    if (next_chunk) {
        if ((vaddr_t)chunk + chunk->len == (vaddr_t)next_chunk) {
            // extend our chunk
            chunk->len += next_chunk->len;

            // remove them from the list
            list_delete(&next_chunk->node);
        }
    }

    mutex_release(&theheap.lock);

    return chunk;
}

static struct free_heap_chunk *heap_create_free_chunk(void *ptr, size_t len, bool allow_debug)
{
    DEBUG_ASSERT((len % sizeof(void *)) == 0); // size must be aligned on pointer boundary

#if DEBUG_HEAP
    if (allow_debug)
        memset(ptr, FREE_FILL, len);
#endif

    struct free_heap_chunk *chunk = (struct free_heap_chunk *)ptr;
    chunk->len = len;

    return chunk;
}

void *miniheap_alloc(size_t size, unsigned int alignment)
{
    void *ptr;
#if DEBUG_HEAP
    size_t original_size = size;
#endif

    LTRACEF("size %zd, align %d\n", size, alignment);

    // alignment must be power of 2
    if (alignment & (alignment - 1))
        return NULL;

    // we always put a size field + base pointer + magic in front of the allocation
    size += sizeof(struct alloc_struct_begin);
#if DEBUG_HEAP
    size += PADDING_SIZE;
#endif

    // make sure we allocate at least the size of a struct free_heap_chunk so that
    // when we free it, we can create a struct free_heap_chunk struct and stick it
    // in the spot
    if (size < sizeof(struct free_heap_chunk))
        size = sizeof(struct free_heap_chunk);

    // round up size to a multiple of native pointer size
    size = ROUNDUP(size, sizeof(void *));

    // deal with nonzero alignments
    if (alignment > 0) {
        if (alignment < 16)
            alignment = 16;

        // add alignment for worst case fit
        size += alignment;
    }

    int retry_count = 0;
retry:
    mutex_acquire(&theheap.lock);

    // walk through the list
    ptr = NULL;
    struct free_heap_chunk *chunk;
    list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
        DEBUG_ASSERT((chunk->len % sizeof(void *)) == 0); // len should always be a multiple of pointer size

        // is it big enough to service our allocation?
        if (chunk->len >= size) {
            ptr = chunk;

            // remove it from the list
            struct list_node *next_node = list_next(&theheap.free_list, &chunk->node);
            list_delete(&chunk->node);

            if (chunk->len > size + sizeof(struct free_heap_chunk)) {
                // there's enough space in this chunk to create a new one after the allocation
                struct free_heap_chunk *newchunk = heap_create_free_chunk((uint8_t *)ptr + size, chunk->len - size, true);

                // truncate this chunk
                chunk->len -= chunk->len - size;

                // add the new one where chunk used to be
                if (next_node)
                    list_add_before(next_node, &newchunk->node);
                else
                    list_add_tail(&theheap.free_list, &newchunk->node);
            }

            // the allocated size is actually the length of this chunk, not the size requested
            DEBUG_ASSERT(chunk->len >= size);
            size = chunk->len;

#if DEBUG_HEAP
            memset(ptr, ALLOC_FILL, size);
#endif

            ptr = (void *)((addr_t)ptr + sizeof(struct alloc_struct_begin));

            // align the output if requested
            if (alignment > 0) {
                ptr = (void *)ROUNDUP((addr_t)ptr, (addr_t)alignment);
            }

            struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
            as--;
#if LK_DEBUGLEVEL > 1
            as->magic = HEAP_MAGIC;
#endif
            as->ptr = (void *)chunk;
            as->size = size;
            theheap.remaining -= size;

            if (theheap.remaining < theheap.low_watermark) {
                theheap.low_watermark = theheap.remaining;
            }
#if DEBUG_HEAP
            as->padding_start = ((uint8_t *)ptr + original_size);
            as->padding_size = (((addr_t)chunk + size) - ((addr_t)ptr + original_size));
//          printf("padding start %p, size %u, chunk %p, size %u\n", as->padding_start, as->padding_size, chunk, size);

            memset(as->padding_start, PADDING_FILL, as->padding_size);
#endif

            break;
        }
    }

    mutex_release(&theheap.lock);

    /* try to grow the heap if we can */
    if (ptr == NULL && retry_count == 0) {
        ssize_t err = heap_grow(size);
        if (err >= 0) {
            retry_count++;
            goto retry;
        }
    }

    LTRACEF("returning ptr %p\n", ptr);

    return ptr;
}

void *miniheap_realloc(void *ptr, size_t size)
{
    /* slow implementation */
    if (!ptr)
        return miniheap_alloc(size, 0);
    if (size == 0) {
        miniheap_free(ptr);
        return NULL;
    }

    // XXX better implementation
    void *p = miniheap_alloc(size, 0);
    if (!p)
        return NULL;

    memcpy(p, ptr, size); // XXX wrong
    miniheap_free(ptr);

    return p;
}

void miniheap_free(void *ptr)
{
    if (!ptr)
        return;

    LTRACEF("ptr %p\n", ptr);

    // check for the old allocation structure
    struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
    as--;

    DEBUG_ASSERT(as->magic == HEAP_MAGIC);

#if DEBUG_HEAP
    {
        uint i;
        uint8_t *pad = (uint8_t *)as->padding_start;

        for (i = 0; i < as->padding_size; i++) {
            if (pad[i] != PADDING_FILL) {
                printf("free at %p scribbled outside the lines:\n", ptr);
                hexdump(pad, as->padding_size);
                panic("die\n");
            }
        }
    }
#endif

    LTRACEF("allocation was %zd bytes long at ptr %p\n", as->size, as->ptr);

    // looks good, create a free chunk and add it to the pool
    heap_insert_free_chunk(heap_create_free_chunk(as->ptr, as->size, true));

#if MINIHEAP_AUTOTRIM
    miniheap_trim();
#endif
}

void miniheap_trim(void)
{
    LTRACE_ENTRY;

    mutex_acquire(&theheap.lock);

    // walk through the list, finding free chunks that can be returned to the page allocator
    struct free_heap_chunk *chunk;
    struct free_heap_chunk *next_chunk;
    list_for_every_entry_safe(&theheap.free_list, chunk, next_chunk, struct free_heap_chunk, node) {
        LTRACEF("looking at chunk %p, len 0x%zx\n", chunk, chunk->len);

        uintptr_t start = (uintptr_t)chunk;
        uintptr_t end = start + chunk->len;
        DEBUG_ASSERT(end > start); // make sure it doesn't wrap the address space and has a positive len

        // compute the page aligned region in this free block (if any)
        uintptr_t start_page = ROUNDUP(start, PAGE_SIZE);
        uintptr_t end_page = ROUNDDOWN(end, PAGE_SIZE);
        DEBUG_ASSERT(end_page <= end);
        DEBUG_ASSERT(start_page >= start);

        LTRACEF("start page 0x%lx, end page 0x%lx\n", start_page, end_page);

retry:
        // see if the free block encompasses at least one page
        if (unlikely(end_page > start_page)) {
            LTRACEF("could trim: start 0x%lx, end 0x%lx\n", start_page, end_page);

            // cases where the start of the block is already page aligned
            if (start_page == start) {
                // look for special case, we're going to completely remove the chunk
                if (end_page == end) {
                    LTRACEF("special case, free chunk completely covers page(s)\n");
                    list_delete(&chunk->node);
                    goto free_chunk;
                }
            } else {
                // start of block is not page aligned,
                // will there be enough space before the block if we trim?
                if (start_page - start < sizeof(struct free_heap_chunk)) {
                    LTRACEF("not enough space for free chunk before\n");
                    start_page += PAGE_SIZE;
                    goto retry;
                }
            }

            // do we need to split the free block and create a new block afterwards?
            if (end_page < end) {
                size_t new_chunk_size = end - end_page;
                LTRACEF("will have to split, new chunk will be 0x%zx bytes long\n", new_chunk_size);

                // if there's not enough space afterwards for a free chunk, we can't free the last page
                if (new_chunk_size < sizeof(struct free_heap_chunk)) {
                    LTRACEF("not enough space for free chunk afterwards\n");
                    end_page -= PAGE_SIZE;
                    goto retry;
                }

                // trim the new space off the end of the current chunk
                chunk->len -= new_chunk_size;
                end = end_page;

                // create a new chunk after the one we're trimming
                struct free_heap_chunk *new_chunk = heap_create_free_chunk((void *)end_page, new_chunk_size, false);

                // link it with the current block
                list_add_after(&chunk->node, &new_chunk->node);
            }

            // check again to see if we are now completely covering a block
            if (start_page == start && end_page == end) {
                LTRACEF("special case, after splitting off new chunk, free chunk completely covers page(s)\n");
                list_delete(&chunk->node);
                goto free_chunk;
            }

            // trim the size of the block
            chunk->len -= end_page - start_page;

free_chunk:
            // return it to the allocator
            LTRACEF("returning %p size 0x%lx to the page allocator\n", (void *)start_page, end_page - start_page);
            page_free((void *)start_page, (end_page - start_page) / PAGE_SIZE);

            // tweak accounting
            theheap.remaining -= end_page - start_page;
        }
    }

    mutex_release(&theheap.lock);
}

void miniheap_get_stats(struct miniheap_stats *ptr)
{
    struct free_heap_chunk *chunk;

    ptr->heap_start = theheap.base;
    ptr->heap_len = theheap.len;
    ptr->heap_free=0;
    ptr->heap_max_chunk = 0;

    mutex_acquire(&theheap.lock);

    list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
        ptr->heap_free += chunk->len;

        if (chunk->len > ptr->heap_max_chunk) {
            ptr->heap_max_chunk = chunk->len;
        }
    }

    ptr->heap_low_watermark = theheap.low_watermark;

    mutex_release(&theheap.lock);
}

static ssize_t heap_grow(size_t size)
{
    size = ROUNDUP(size, PAGE_SIZE);
    void *ptr = page_alloc(size / PAGE_SIZE, PAGE_ALLOC_ANY_ARENA);
    if (!ptr) {
        TRACEF("failed to grow kernel heap by 0x%zx bytes\n", size);
        return ERR_NO_MEMORY;
    }

    LTRACEF("growing heap by 0x%zx bytes, new ptr %p\n", size, ptr);

    heap_insert_free_chunk(heap_create_free_chunk(ptr, size, true));

    /* change the heap start and end variables */
    if ((uintptr_t)ptr < (uintptr_t)theheap.base || theheap.base == 0)
        theheap.base = ptr;

    uintptr_t endptr = (uintptr_t)ptr + size;
    if (endptr > (uintptr_t)theheap.base + theheap.len) {
        theheap.len = (uintptr_t)endptr - (uintptr_t)theheap.base;
    }

    return size;
}

void miniheap_init(void *ptr, size_t len)
{
    LTRACEF("ptr %p, len %zu\n", ptr, len);

    // create a mutex
    mutex_init(&theheap.lock);

    // initialize the free list
    list_initialize(&theheap.free_list);

    // set the heap range
    theheap.base = ptr;
    theheap.len = len;
    theheap.remaining = 0; // will get set by heap_insert_free_chunk()
    theheap.low_watermark = 0;

    // if passed a default range, use it
    if (len > 0)
        heap_insert_free_chunk(heap_create_free_chunk(ptr, len, true));
}

