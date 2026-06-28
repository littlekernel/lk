/*
 * Copyright (c) 2008-2009,2012-2015 Travis Geiselbrecht
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/trace.h>
#include <assert.h>
#include <lk/err.h>
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

struct free_heap_chunk {
    struct free_heap_chunk *next;
    size_t len;
};

struct heap {
    void *base;
    size_t len;
    size_t remaining;
    size_t low_watermark;
    mutex_t lock;
    struct free_heap_chunk *free_list;  // singly-linked, kept in address order, NULL-terminated
};

// heap static vars
static struct heap theheap;

// structure placed at the beginning every allocation
struct alloc_struct_begin {
    void *ptr;
    size_t size;
#if DEBUG_HEAP
    void *padding_start;
    size_t padding_size;
#endif
};

// The heap alignment unit: two native pointer-widths.
// This is the minimum alignment malloc() must return so that any
// fundamental scalar type (including uint64_t / double on 32-bit) can be
// stored at the returned address.  We deliberately cap at 2*sizeof(void*)
// rather than alignof(max_align_t) because some toolchains include __float128
// in max_align_t (pushing it to 16 on rv32), which would wastefully inflate
// every allocation on 32-bit targets.
#define HEAP_ALIGN (2 * sizeof(void *))

// Both structs must be exactly HEAP_ALIGN bytes so that:
//   - every chunk boundary is naturally aligned for malloc() return values
//   - sizeof(free_heap_chunk) == sizeof(alloc_struct_begin), keeping the
//     minimum-allocation-size invariant clean
STATIC_ASSERT(sizeof(struct alloc_struct_begin) == HEAP_ALIGN);
STATIC_ASSERT(sizeof(struct free_heap_chunk)    == HEAP_ALIGN);

static ssize_t heap_grow(size_t len);

static bool heap_range_contains(uintptr_t addr, size_t len) {
    const uintptr_t heap_start = (uintptr_t)theheap.base;
    const uintptr_t heap_end = heap_start + theheap.len;

    // Is the heap range valid?
    if (unlikely(heap_end < heap_start))
        return false;

    // Is the start of the range within the heap?
    if (unlikely(addr < heap_start || addr > heap_end))
        return false;

    // Is the end of the range within the heap?
    if (unlikely(addr + len > heap_end))
        return false;

    return true;
}

static bool heap_validate_allocation(const struct alloc_struct_begin *as) {
    return heap_range_contains((uintptr_t)as->ptr, as->size);
}

static void dump_free_chunk(struct free_heap_chunk *chunk) {
    dprintf(INFO, "\t\tbase %p, end 0x%lx, len 0x%zx\n", chunk, (vaddr_t)chunk + chunk->len, chunk->len);
}

void miniheap_dump(void) {
    dprintf(INFO, "Heap dump (using miniheap):\n");
    dprintf(INFO, "\tbase %p, len 0x%zx\n", theheap.base, theheap.len);
    dprintf(INFO, "\tfree list:\n");

    mutex_acquire(&theheap.lock);

    for (struct free_heap_chunk *chunk = theheap.free_list; chunk; chunk = chunk->next) {
        dump_free_chunk(chunk);
    }
    mutex_release(&theheap.lock);

}

// try to insert this free chunk into the free list, consuming the chunk by merging it with
// nearby ones if possible. Returns base of whatever chunk it became in the list.
static struct free_heap_chunk *heap_insert_free_chunk(struct free_heap_chunk *chunk) {
    vaddr_t chunk_end = (vaddr_t)chunk + chunk->len;

    LTRACEF("chunk ptr %p, size 0x%zx\n", chunk, chunk->len);

    mutex_acquire(&theheap.lock);

    theheap.remaining += chunk->len;

    // Walk the singly-linked list (kept in ascending address order) to find
    // the insertion point, tracking the previous node as we go.
    struct free_heap_chunk **prevp = &theheap.free_list;
    struct free_heap_chunk *next_chunk = theheap.free_list;
    while (next_chunk && next_chunk < chunk) {
        prevp = &next_chunk->next;
        next_chunk = next_chunk->next;
    }
    DEBUG_ASSERT(!next_chunk || chunk_end <= (vaddr_t)next_chunk);

    // Insert chunk between *prevp and next_chunk.
    chunk->next = next_chunk;
    *prevp = chunk;

    // Try to merge with the previous chunk (the one whose ->next we just overwrote).
    // prevp either points to theheap.free_list (no prev) or to prev_chunk->next.
    // Recover prev_chunk by working backwards from prevp.
    struct free_heap_chunk *prev_chunk =
        (prevp == &theheap.free_list)
            ? NULL
            : (struct free_heap_chunk *)((char *)prevp - offsetof(struct free_heap_chunk, next));
    if (prev_chunk && (vaddr_t)prev_chunk + prev_chunk->len == (vaddr_t)chunk) {
        // Extend prev_chunk to absorb chunk; remove chunk from the list.
        prev_chunk->len += chunk->len;
        prev_chunk->next = chunk->next;
        chunk = prev_chunk;  // chunk now refers to the merged block
    }

    // Try to merge with the next chunk.
    if (chunk->next && (vaddr_t)chunk + chunk->len == (vaddr_t)chunk->next) {
        struct free_heap_chunk *merged_next = chunk->next;
        chunk->len += merged_next->len;
        chunk->next = merged_next->next;
    }

    mutex_release(&theheap.lock);

    return chunk;
}

static struct free_heap_chunk *heap_create_free_chunk(void *ptr, size_t len, bool allow_debug) {
    DEBUG_ASSERT((len % HEAP_ALIGN) == 0); // size must be a multiple of the heap alignment unit
    DEBUG_ASSERT(len >= sizeof(struct free_heap_chunk));

#if DEBUG_HEAP
    if (allow_debug)
        memset(ptr, FREE_FILL, len);
#endif

    struct free_heap_chunk *chunk = (struct free_heap_chunk *)ptr;
    chunk->len = len;

    return chunk;
}

void *miniheap_alloc(size_t size, unsigned int alignment) {
    void *ptr;
    size_t original_size = size;

    LTRACEF("size %zd, align %d\n", size, (int)alignment);

    // alignment must be power of 2
    if (alignment & (alignment - 1))
        return NULL;

    // we always put a base pointer + size in front of the allocation
    size += sizeof(struct alloc_struct_begin);
#if DEBUG_HEAP
    size += PADDING_SIZE;
#endif

    // make sure we allocate at least the size of a struct free_heap_chunk so that
    // when we free it, we can create a struct free_heap_chunk struct and stick it
    // in the spot
    if (size < sizeof(struct free_heap_chunk))
        size = sizeof(struct free_heap_chunk);

    // round up size to HEAP_ALIGN so every chunk boundary stays
    // naturally aligned and malloc() return values meet the C standard guarantee
    size = ROUNDUP(size, HEAP_ALIGN);

    // If the requested alignment exceeds our natural HEAP_ALIGN guarantee,
    // add slack for the worst-case offset needed to hit the next aligned boundary.
    // Alignments <= HEAP_ALIGN are already satisfied for free: chunks are
    // HEAP_ALIGN-aligned and the header is exactly HEAP_ALIGN bytes.
    //
    // The pre-ROUNDUP payload pointer is always HEAP_ALIGN-aligned (chunk start
    // is HEAP_ALIGN-aligned, header is HEAP_ALIGN bytes).  Since alignment is a
    // power-of-2 multiple of HEAP_ALIGN, the worst-case distance to the next
    // alignment boundary is (alignment - HEAP_ALIGN), not a full alignment.
    if (alignment > HEAP_ALIGN) {
        size += alignment - HEAP_ALIGN;
    }

    // check that the size additions above didn't wrap around
    if (size < original_size)
        return NULL;

    int retry_count = 0;
retry:
    mutex_acquire(&theheap.lock);

    // walk through the list
    ptr = NULL;
    struct free_heap_chunk **prevp = &theheap.free_list;
    struct free_heap_chunk *chunk;
    for (chunk = theheap.free_list; chunk; prevp = &chunk->next, chunk = chunk->next) {
        DEBUG_ASSERT((chunk->len % HEAP_ALIGN) == 0); // len must be a multiple of the heap alignment unit

        // is it big enough to service our allocation?
        if (chunk->len >= size) {
            ptr = chunk;

            // remove it from the singly-linked list
            struct free_heap_chunk *next_chunk = chunk->next;
            *prevp = next_chunk;

            if (chunk->len > size + sizeof(struct free_heap_chunk)) {
                // there's enough space in this chunk to create a new one after the allocation
                struct free_heap_chunk *newchunk = heap_create_free_chunk((uint8_t *)ptr + size, chunk->len - size, true);

                // truncate this chunk
                chunk->len -= chunk->len - size;

                // insert the remainder right after the gap we just created
                newchunk->next = next_chunk;
                *prevp = newchunk;
            }

            // the allocated size is actually the length of this chunk, not the size requested
            DEBUG_ASSERT(chunk->len >= size);
            size = chunk->len;

#if DEBUG_HEAP
            memset(ptr, ALLOC_FILL, size);
#endif

            ptr = (void *)((addr_t)ptr + sizeof(struct alloc_struct_begin));

            // Round up to the requested alignment if it exceeds our natural guarantee.
            if (alignment > HEAP_ALIGN) {
                ptr = (void *)ROUNDUP((addr_t)ptr, (addr_t)alignment);
            }

            struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
            as--;
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

void *miniheap_realloc(void *ptr, size_t size) {
    /* slow implementation */
    if (!ptr)
        return miniheap_alloc(size, 0);
    if (size == 0) {
        miniheap_free(ptr);
        return NULL;
    }

    // determine how many bytes the old allocation holds so we don't read past it
    struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
    as--;

    DEBUG_ASSERT(heap_range_contains((uintptr_t)as, sizeof(struct alloc_struct_begin)));
    DEBUG_ASSERT(heap_validate_allocation(as));

    DEBUG_ASSERT((addr_t)ptr >= (addr_t)as->ptr);
    size_t ptr_offset = (addr_t)ptr - (addr_t)as->ptr;
    DEBUG_ASSERT(as->size >= ptr_offset);
    size_t old_usable = as->size - ptr_offset;
    size_t copy_size = (size < old_usable) ? size : old_usable;

    void *p = miniheap_alloc(size, 0);
    if (!p)
        return NULL;

    memcpy(p, ptr, copy_size);
    miniheap_free(ptr);

    return p;
}

void miniheap_free(void *ptr) {
    if (!ptr)
        return;

    LTRACEF("ptr %p\n", ptr);

    // check for the old allocation structure
    struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
    as--;

    DEBUG_ASSERT(heap_range_contains((uintptr_t)as, sizeof(struct alloc_struct_begin)));
    DEBUG_ASSERT(heap_validate_allocation(as));

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

void miniheap_trim(void) {
    LTRACE_ENTRY;

    mutex_acquire(&theheap.lock);

    // Walk the singly-linked free list, finding free chunks that can be
    // returned to the page allocator.  Track prevp so we can splice out
    // or insert nodes without a doubly-linked list.
    struct free_heap_chunk **prevp = &theheap.free_list;
    struct free_heap_chunk *chunk = theheap.free_list;
    while (chunk) {
        // Pre-fetch next before we potentially modify chunk->next.
        struct free_heap_chunk *next_chunk = chunk->next;

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
                    // Splice out of singly-linked list.
                    *prevp = next_chunk;
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

                // create a new chunk after the one we're trimming and splice it in
                struct free_heap_chunk *new_chunk = heap_create_free_chunk((void *)end_page, new_chunk_size, false);
                new_chunk->next = next_chunk;
                chunk->next = new_chunk;
                next_chunk = new_chunk;  // update so the outer loop advances correctly
            }

            // check again to see if we are now completely covering a block
            if (start_page == start && end_page == end) {
                LTRACEF("special case, after splitting off new chunk, free chunk completely covers page(s)\n");
                *prevp = next_chunk;
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

        // Advance: if we spliced out chunk, prevp stays the same; otherwise advance it.
        if (*prevp == chunk) {
            prevp = &chunk->next;
        }
        chunk = next_chunk;
    }

    mutex_release(&theheap.lock);
}

void miniheap_get_stats(struct miniheap_stats *ptr) {
    ptr->heap_start = theheap.base;
    ptr->heap_len = theheap.len;
    ptr->heap_free = 0;
    ptr->heap_max_chunk = 0;

    mutex_acquire(&theheap.lock);

    for (struct free_heap_chunk *chunk = theheap.free_list; chunk; chunk = chunk->next) {
        ptr->heap_free += chunk->len;

        if (chunk->len > ptr->heap_max_chunk) {
            ptr->heap_max_chunk = chunk->len;
        }
    }

    ptr->heap_low_watermark = theheap.low_watermark;

    mutex_release(&theheap.lock);
}

static ssize_t heap_grow(size_t size) {
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

void miniheap_init(void *ptr, size_t len) {
    LTRACEF("ptr %p, len %zu\n", ptr, len);

    // create a mutex
    mutex_init(&theheap.lock);

    // initialize the free list
    theheap.free_list = NULL;

    // Align the base pointer to HEAP_ALIGN so the first free chunk
    // header — and every malloc() return derived from it — meets the
    // minimum alignment guarantee.  The backing store from novm_alloc_unaligned
    // may be only 4-byte aligned, so we must not assume stronger alignment here.
    if (((uintptr_t)ptr % HEAP_ALIGN) > 0) {
        uintptr_t aligned_ptr = ROUNDUP((uintptr_t)ptr, HEAP_ALIGN);

        DEBUG_ASSERT((aligned_ptr - (uintptr_t)ptr) < len);

        len -= aligned_ptr - (uintptr_t)ptr;
        ptr = (void *)aligned_ptr;

        LTRACEF("(aligned) ptr %p, len %zu\n", ptr, len);
    }

    // set the heap range
    theheap.base = ptr;
    theheap.len = len;
    theheap.remaining = 0; // will get set by heap_insert_free_chunk()
    theheap.low_watermark = 0;

    // if passed a default range, use it
    if (len > 0)
        heap_insert_free_chunk(heap_create_free_chunk(ptr, len, true));
}

