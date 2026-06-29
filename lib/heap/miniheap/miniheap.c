/*
 * Copyright (c) 2008-2009,2012-2015 Travis Geiselbrecht
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/*
 * Theory of Operation:
 *
 * Miniheap is a minimalist, space-efficient, and self-contained heap allocator
 * designed for resource-constrained or early boot environments. The "mini" name
 * reflects its key design goal: minimizing metadata and tracking overhead.
 *
 * Free blocks are tracked in a singly-linked list kept in ascending memory
 * address order. When a block is freed, the allocator traverses the list to
 * insert the block at its correct address-sorted location. This ordering makes
 * coalescing contiguous free blocks trivial, as adjacent blocks can be merged
 * in-place on the fly during deallocations.
 *
 * Shortcomings and Trade-offs:
 *
 * While highly space-efficient (reusing free block memory for link pointers and
 * keeping a minimal header for active allocations), miniheap is slow:
 * - Linear Search Complexity: Both allocation (`malloc`) and deallocation
 *   (`free`) operations require traversing the singly-linked free list, resulting
 *   in O(N) complexity where N is the number of free blocks.
 * - Naive Strategies: First-fit allocation and O(N) reallocations (which always
 *   allocate-copy-free rather than extending in-place) make it unsuitable for
 *   high-performance or heavily dynamic allocation patterns.
 */
#include <lib/miniheap.h>

#include <assert.h>
#include <kernel/mutex.h>
#include <lib/heap.h>
#include <lib/page_alloc.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

#define DEBUG_HEAP   0
#define ALLOC_FILL   0x99
#define FREE_FILL    0x77
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
    struct free_heap_chunk *free_list; // singly-linked, kept in address order, NULL-terminated
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

// Both structs must be multiples of HEAP_ALIGN bytes so that:
//   - every chunk boundary is naturally aligned for malloc() return values
//   - the minimum-allocation-size invariant stays clean
STATIC_ASSERT((sizeof(struct alloc_struct_begin) % HEAP_ALIGN) == 0);
STATIC_ASSERT(sizeof(struct free_heap_chunk) == HEAP_ALIGN);

static ssize_t heap_grow(size_t len);

static bool heap_range_contains(uintptr_t addr, size_t len) {
    const uintptr_t heap_start = (uintptr_t)theheap.base;
    const uintptr_t heap_end = heap_start + theheap.len;

    // Is the heap range valid?
    if (unlikely(heap_end < heap_start)) {
        return false;
    }

    // Is the start of the range within the heap?
    if (unlikely(addr < heap_start || addr > heap_end)) {
        return false;
    }

    // Is the end of the range within the heap?
    if (unlikely(addr + len > heap_end)) {
        return false;
    }

    return true;
}

static bool heap_validate_allocation(const struct alloc_struct_begin *as) {
    return heap_range_contains((uintptr_t)as->ptr, as->size);
}

static void dump_free_chunk(struct free_heap_chunk *chunk) {
    dprintf(INFO, "\t\tbase %p, end 0x%lx, len 0x%zx\n", chunk, (vaddr_t)chunk + chunk->len,
            chunk->len);
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
        chunk = prev_chunk; // chunk now refers to the merged block
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
    if (allow_debug) {
        memset(ptr, FREE_FILL, len);
    }
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
    if (alignment & (alignment - 1)) {
        return NULL;
    }

    // we always put a base pointer + size in front of the allocation
    size += sizeof(struct alloc_struct_begin);
#if DEBUG_HEAP
    size += PADDING_SIZE;
#endif

    // make sure we allocate at least the size of a struct free_heap_chunk so that
    // when we free it, we can create a struct free_heap_chunk struct and stick it
    // in the spot
    if (size < sizeof(struct free_heap_chunk)) {
        size = sizeof(struct free_heap_chunk);
    }

    // round up size to HEAP_ALIGN so every chunk boundary stays
    // naturally aligned and malloc() return values meet the C standard guarantee
    size = ROUNDUP(size, HEAP_ALIGN);

    // check that the size additions above didn't wrap around
    if (size < original_size) {
        return NULL;
    }

    size_t needed_size = size;

    int retry_count = 0;
retry:
    mutex_acquire(&theheap.lock);

    // walk through the list
    ptr = NULL;
    struct free_heap_chunk **prevp = &theheap.free_list;
    struct free_heap_chunk *chunk;
    for (chunk = theheap.free_list; chunk; prevp = &chunk->next, chunk = chunk->next) {
        DEBUG_ASSERT((chunk->len % HEAP_ALIGN) ==
                     0); // len must be a multiple of the heap alignment unit

        // Calculate the exact alignment offset (front slack) for this chunk
        size_t front_slack = 0;
        if (alignment > HEAP_ALIGN) {
            uintptr_t chunk_start = (uintptr_t)chunk;
            uintptr_t aligned_payload =
                ROUNDUP(chunk_start + sizeof(struct alloc_struct_begin), (uintptr_t)alignment);
            uintptr_t allocated_start = aligned_payload - sizeof(struct alloc_struct_begin);
            front_slack = allocated_start - chunk_start;
        }

        // is it big enough to service our allocation?
        if (chunk->len >= front_slack + needed_size) {
            ptr = (void *)chunk;
            struct free_heap_chunk *next_chunk = chunk->next;

            size_t total_allocated_size = chunk->len - front_slack;
            size_t rear_slack = total_allocated_size - needed_size;

            bool split_rear = (rear_slack >= sizeof(struct free_heap_chunk));
            if (split_rear) {
                total_allocated_size = needed_size;
            }

            // Verify heap invariants under the new split configuration
            DEBUG_ASSERT((front_slack % HEAP_ALIGN) == 0 &&
                         (front_slack == 0 || front_slack >= sizeof(struct free_heap_chunk)));
            DEBUG_ASSERT(total_allocated_size >= needed_size &&
                         (total_allocated_size % HEAP_ALIGN) == 0);
            DEBUG_ASSERT(front_slack + total_allocated_size + (split_rear ? rear_slack : 0) ==
                         chunk->len);
            DEBUG_ASSERT(split_rear ? ((rear_slack % HEAP_ALIGN) == 0 &&
                                       rear_slack >= sizeof(struct free_heap_chunk))
                                    : (rear_slack < sizeof(struct free_heap_chunk)));
            if (alignment > HEAP_ALIGN) {
                uintptr_t allocated_start = (uintptr_t)ptr + front_slack;
                DEBUG_ASSERT(((allocated_start + sizeof(struct alloc_struct_begin)) % alignment) ==
                             0);
            }

            // Update free list and handle splits
            if (front_slack > 0) {
                // Keep the front part of the chunk in the free list, shrinking its length in-place
                chunk->len = front_slack;

                if (split_rear) {
                    // Create rear free chunk and link it after the front chunk
                    struct free_heap_chunk *rear_chunk = heap_create_free_chunk(
                        (uint8_t *)ptr + front_slack + total_allocated_size, rear_slack, true);
                    rear_chunk->next = next_chunk;
                    chunk->next = rear_chunk;
                }
            } else {
                // No front slack. The allocation starts at the chunk's base.
                // We remove chunk from the free list.
                if (split_rear) {
                    struct free_heap_chunk *rear_chunk = heap_create_free_chunk(
                        (uint8_t *)ptr + total_allocated_size, rear_slack, true);
                    rear_chunk->next = next_chunk;
                    *prevp = rear_chunk;
                } else {
                    *prevp = next_chunk;
                }
            }

            uintptr_t allocated_start = (uintptr_t)ptr + front_slack;

#if DEBUG_HEAP
            memset((void *)allocated_start, ALLOC_FILL, total_allocated_size);
#endif

            ptr = (void *)(allocated_start + sizeof(struct alloc_struct_begin));

            struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
            as--;
            as->ptr = (void *)allocated_start;
            as->size = total_allocated_size;
            theheap.remaining -= total_allocated_size;

            if (theheap.remaining < theheap.low_watermark) {
                theheap.low_watermark = theheap.remaining;
            }
#if DEBUG_HEAP
            as->padding_start = ((uint8_t *)ptr + original_size);
            as->padding_size =
                (((addr_t)allocated_start + total_allocated_size) - ((addr_t)ptr + original_size));
            memset(as->padding_start, PADDING_FILL, as->padding_size);
#endif

            break;
        }
    }

    mutex_release(&theheap.lock);

    /* try to grow the heap if we can */
    if (ptr == NULL && retry_count == 0) {
        size_t grow_size = needed_size;
        if (alignment > HEAP_ALIGN) {
            grow_size += alignment - HEAP_ALIGN;
        }
        ssize_t err = heap_grow(grow_size);
        if (err >= 0) {
            retry_count++;
            goto retry;
        }
    }

    LTRACEF("returning ptr %p\n", ptr);

    return ptr;
}

// Scan the free list to locate a free chunk starting at target_addr.
// Returns the chunk, and sets out_prevp to its pointing pointer for list updates.
static struct free_heap_chunk *find_free_chunk_at(vaddr_t target_addr,
                                                  struct free_heap_chunk ***out_prevp) {
    struct free_heap_chunk **prevp = &theheap.free_list;
    struct free_heap_chunk *chunk = theheap.free_list;
    while (chunk) {
        if ((vaddr_t)chunk == target_addr) {
            *out_prevp = prevp;
            return chunk;
        }
        prevp = &chunk->next;
        chunk = chunk->next;
    }
    return NULL;
}

void *miniheap_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return miniheap_alloc(size, 0);
    }
    if (size == 0) {
        miniheap_free(ptr);
        return NULL;
    }

    struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
    as--;

    DEBUG_ASSERT(heap_range_contains((uintptr_t)as, sizeof(struct alloc_struct_begin)));
    DEBUG_ASSERT(heap_validate_allocation(as));

    DEBUG_ASSERT((addr_t)ptr >= (addr_t)as->ptr);
    size_t ptr_offset = (addr_t)ptr - (addr_t)as->ptr;
    DEBUG_ASSERT(as->size >= ptr_offset);

    // Compute the new target block size (including headers, alignment padding)
    size_t new_block_size = ptr_offset + size;
#if DEBUG_HEAP
    new_block_size += PADDING_SIZE;
#endif
    new_block_size = ROUNDUP(new_block_size, HEAP_ALIGN);

    mutex_acquire(&theheap.lock);

    // Case 1: In-place Shrinking
    if (new_block_size <= as->size) {
        size_t leftover_slack = as->size - new_block_size;
        if (leftover_slack >= sizeof(struct free_heap_chunk)) {
            void *free_tail_ptr = (uint8_t *)as->ptr + new_block_size;
            as->size = new_block_size;
            theheap.remaining -= leftover_slack; // heap_insert_free_chunk will add this back

            // Release the lock before calling heap_insert_free_chunk to avoid deadlock
            mutex_release(&theheap.lock);
            heap_insert_free_chunk(heap_create_free_chunk(free_tail_ptr, leftover_slack, true));
        } else {
            mutex_release(&theheap.lock);
        }

#if DEBUG_HEAP
        as->padding_start = ((uint8_t *)ptr + size);
        as->padding_size = (((addr_t)as->ptr + as->size) - ((addr_t)ptr + size));
        memset(as->padding_start, PADDING_FILL, as->padding_size);
#endif
        return ptr;
    }

    // Case 2: In-place Growing
    vaddr_t block_end = (vaddr_t)as->ptr + as->size;
    struct free_heap_chunk **prevp = NULL;
    struct free_heap_chunk *next_free = find_free_chunk_at(block_end, &prevp);

    if (next_free && (as->size + next_free->len >= new_block_size)) {
        size_t extra_needed = new_block_size - as->size;
        size_t remaining_len = next_free->len - extra_needed;

        if (remaining_len >= sizeof(struct free_heap_chunk)) {
            // Shrink next_free in-place
            struct free_heap_chunk *new_next =
                heap_create_free_chunk((uint8_t *)next_free + extra_needed, remaining_len, false);
            new_next->next = next_free->next;
            *prevp = new_next;

            as->size = new_block_size;
            theheap.remaining -= extra_needed;
        } else {
            // Absorb the entire next_free chunk
            *prevp = next_free->next;
            as->size = as->size + next_free->len;
            theheap.remaining -= next_free->len;
        }

        mutex_release(&theheap.lock);

#if DEBUG_HEAP
        as->padding_start = ((uint8_t *)ptr + size);
        as->padding_size = (((addr_t)as->ptr + as->size) - ((addr_t)ptr + size));
        memset(as->padding_start, PADDING_FILL, as->padding_size);
#endif
        return ptr;
    }

    mutex_release(&theheap.lock);

    // Case 3: Fallback (Allocate - Copy - Free)
    size_t old_usable = as->size - ptr_offset;
    size_t copy_size = (size < old_usable) ? size : old_usable;

    void *p = miniheap_alloc(size, 0);
    if (!p) {
        return NULL;
    }

    memcpy(p, ptr, copy_size);
    miniheap_free(ptr);

    return p;
}

void miniheap_free(void *ptr) {
    if (!ptr) {
        return;
    }

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
        DEBUG_ASSERT(end >
                     start); // make sure it doesn't wrap the address space and has a positive len

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

                // if there's not enough space afterwards for a free chunk, we can't free the last
                // page
                if (new_chunk_size < sizeof(struct free_heap_chunk)) {
                    LTRACEF("not enough space for free chunk afterwards\n");
                    end_page -= PAGE_SIZE;
                    goto retry;
                }

                // trim the new space off the end of the current chunk
                chunk->len -= new_chunk_size;
                end = end_page;

                // create a new chunk after the one we're trimming and splice it in
                struct free_heap_chunk *new_chunk =
                    heap_create_free_chunk((void *)end_page, new_chunk_size, false);
                new_chunk->next = next_chunk;
                chunk->next = new_chunk;
                next_chunk = new_chunk; // update so the outer loop advances correctly
            }

            // check again to see if we are now completely covering a block
            if (start_page == start && end_page == end) {
                LTRACEF("special case, after splitting off new chunk, free chunk completely covers "
                        "page(s)\n");
                *prevp = next_chunk;
                goto free_chunk;
            }

            // trim the size of the block
            chunk->len -= end_page - start_page;

free_chunk:
            // return it to the allocator
            LTRACEF("returning %p size 0x%lx to the page allocator\n", (void *)start_page,
                    end_page - start_page);
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
    if ((uintptr_t)ptr < (uintptr_t)theheap.base || theheap.base == 0) {
        theheap.base = ptr;
    }

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
    if (len > 0) {
        heap_insert_free_chunk(heap_create_free_chunk(ptr, len, true));
    }
}
