/*
 * Copyright (c) 2015 Google, Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "cmpctmalloc_private.h"
#include <lib/cmpctmalloc.h>

#include <assert.h>
#include <kernel/mutex.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lib/heap.h>
#include <lib/page_alloc.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Malloc implementation tuned for space.
//
// Allocation strategy takes place with a global mutex.  Freelist entries are
// kept in linked lists with 8 different sizes per binary order of magnitude
// and the header size is two words with eager coalescing on free.

#ifdef DEBUG
#define CMPCT_DEBUG
#endif

#define LOCAL_TRACE 0

#define ALLOC_FILL   0x99
#define FREE_FILL    0x77
#define PADDING_FILL 0x55

#if WITH_KERNEL_VM && !defined(HEAP_GROW_SIZE)
#define HEAP_GROW_SIZE (1 * 1024 * 1024) /* Grow aggressively */
#elif !defined(HEAP_GROW_SIZE)
#define HEAP_GROW_SIZE (4 * 1024) /* Grow less aggressively */
#endif

STATIC_ASSERT(IS_PAGE_ALIGNED(HEAP_GROW_SIZE));

// When we grow the heap we have to have somewhere in the freelist to put the
// resulting freelist entry, so the freelist has to have a certain number of
// buckets.
STATIC_ASSERT(HEAP_GROW_SIZE <= (1u << HEAP_ALLOC_VIRTUAL_BITS));

// Heap static vars.
struct heap theheap;

static ssize_t heap_grow(size_t len, free_t **bucket);

static void lock(void) {
    mutex_acquire(&theheap.lock);
}

static void unlock(void) {
    mutex_release(&theheap.lock);
}

static void dump_free(header_t *header) {
    dprintf(INFO, "\t\tbase %p, end 0x%lx, len 0x%zx\n", header, (vaddr_t)header + header->size,
            header->size);
}

void cmpct_dump(void) {
    lock();
    dprintf(INFO, "Heap dump (using cmpctmalloc):\n");
    dprintf(INFO, "\tsize %lu, remaining %lu\n", (unsigned long)theheap.size,
            (unsigned long)theheap.remaining);

    dprintf(INFO, "\tfree list:\n");
    for (int i = 0; i < NUMBER_OF_BUCKETS; i++) {
        bool header_printed = false;
        free_t *free_area = theheap.free_lists[i];
        for (; free_area != NULL; free_area = free_area->next) {
            ASSERT(free_area != free_area->next);
            if (!header_printed) {
                dprintf(INFO, "\tbucket %d\n", i);
                header_printed = true;
            }
            dump_free(&free_area->header);
        }
    }
    unlock();
}

// Operates in sizes that don't include the allocation header.
static int size_to_index_helper(size_t size, size_t *rounded_up_out, int adjust, int increment) {
    const size_t log_threshold = 16 * HEAP_ALIGN;
    // First buckets are simply HEAP_ALIGN-spaced up to log_threshold.
    if (size <= log_threshold) {
        if (sizeof(size_t) == 8u && size <= sizeof(free_t) - sizeof(header_t)) {
            *rounded_up_out = sizeof(free_t) - sizeof(header_t);
        } else {
            *rounded_up_out = size;
        }
        // No allocation is smaller than HEAP_ALIGN bytes, so the first bucket is for HEAP_ALIGN
        // byte spaces (not including the header).  For 64 bit, the free list
        // struct is 16 bytes larger than the header, so no allocation can be
        // smaller than that (otherwise how to free it), but we have empty 8
        // and 16 byte buckets for simplicity.
        return (size >> HEAP_ALIGN_SHIFT) - 1;
    }

    // We are going to go up to the next size to round up, but if we hit a
    // bucket size exactly we don't want to go up. By subtracting HEAP_ALIGN here, we
    // will do the right thing (the carry propagates up for the round numbers
    // we are interested in).
    size += adjust;
    // After log_threshold the buckets are logarithmically spaced, every 16 up to 256,
    // every 32 up to 512 etc.  This can be thought of as rows of 8 buckets.
    // GCC intrinsic count-leading-zeros.
    // Eg. 128-255 has 24 leading zeros and we want row to be 4.
    unsigned row = sizeof(size_t) * 8 - 4 - __builtin_clzl(size);
    // For row 4 we want to shift down 4 bits.
    unsigned column = (size >> row) & 7;
    int row_column = (row << 3) | column;
    row_column += increment;
    size = (8 + (row_column & 7)) << (row_column >> 3);
    *rounded_up_out = size;
    // We start with 15 buckets, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96,
    // 104, 112, 120.  Then we have row 4, sizes 128 and up, with the
    // row-column 8 and up.
    int answer = row_column + 15 - ((HEAP_ALIGN_SHIFT + 1) << 3);
    DEBUG_ASSERT(answer < NUMBER_OF_BUCKETS);
    return answer;
}

// Round up size to next bucket when allocating.
int size_to_index_allocating(size_t size, size_t *rounded_up_out) {
    size_t rounded = ROUNDUP(size, HEAP_ALIGN);
    return size_to_index_helper(rounded, rounded_up_out, -(int)HEAP_ALIGN, 1);
}

// Round down size to next bucket when freeing.
int size_to_index_freeing(size_t size) {
    size_t dummy;
    return size_to_index_helper(size, &dummy, 0, 0);
}

static inline header_t *tag_as_free(void *left) {
    return (header_t *)((uintptr_t)left | 1);
}

static inline bool is_tagged_as_free(header_t *header) {
    return ((uintptr_t)(header->left) & 1) != 0;
}

static inline header_t *untag(void *left) {
    return (header_t *)((uintptr_t)left & ~1);
}

static inline header_t *right_header(header_t *header) {
    return (header_t *)((char *)header + header->size);
}

static inline void set_free_list_bit(int index) {
    theheap.free_list_bits[index >> 5] |= (1u << (31 - (index & 0x1f)));
}

static inline void clear_free_list_bit(int index) {
    theheap.free_list_bits[index >> 5] &= ~(1u << (31 - (index & 0x1f)));
}

static int find_nonempty_bucket(int index) {
    uint32_t mask = (1u << (31 - (index & 0x1f))) - 1;
    mask = mask * 2 + 1;
    mask &= theheap.free_list_bits[index >> 5];
    if (mask != 0) {
        return (index & ~0x1f) + __builtin_clz(mask);
    }
    for (index = ROUNDUP(index + 1, 32); index < NUMBER_OF_BUCKETS; index += 32) {
        mask = theheap.free_list_bits[index >> 5];
        if (mask != 0u) {
            return index + __builtin_clz(mask);
        }
    }
    return -1;
}

static bool is_start_of_os_allocation(header_t *header) {
    return header->left == untag(NULL);
}

static void create_free_area(void *address, void *left, size_t size, free_t **bucket) {
    free_t *free_area = (free_t *)address;
    free_area->header.size = size;
    free_area->header.left = tag_as_free(left);
    if (bucket == NULL) {
        int index = size_to_index_freeing(size - sizeof(header_t));
        set_free_list_bit(index);
        bucket = &theheap.free_lists[index];
    }
    free_t *old_head = *bucket;
    if (old_head != NULL) {
        old_head->prev = free_area;
    }
    free_area->next = old_head;
    free_area->prev = NULL;
    *bucket = free_area;
    theheap.remaining += size;
#ifdef CMPCT_DEBUG
    memset(free_area + 1, FREE_FILL, size - sizeof(free_t));
#endif
}

static bool is_end_of_os_allocation(char *address) {
    return ((header_t *)address)->size == 0;
}

static void free_to_os(header_t *header, size_t size) {
    DEBUG_ASSERT(IS_PAGE_ALIGNED(size));
    page_free(header, size >> PAGE_SIZE_SHIFT);
    theheap.size -= size;
}

static void free_memory(void *address, void *left, size_t size) {
    left = untag(left);
    if (IS_PAGE_ALIGNED(left) && is_start_of_os_allocation(left) &&
        is_end_of_os_allocation((char *)address + size)) {
        free_to_os(left, size + ((header_t *)left)->size + sizeof(header_t));
    } else {
        create_free_area(address, left, size, NULL);
    }
}

static void unlink_free(free_t *free_area, int bucket) {
    theheap.remaining -= free_area->header.size;
    ASSERT(theheap.remaining < 4000000000u);
    free_t *next = free_area->next;
    free_t *prev = free_area->prev;
    if (theheap.free_lists[bucket] == free_area) {
        theheap.free_lists[bucket] = next;
        if (next == NULL) {
            clear_free_list_bit(bucket);
        }
    }
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
}

static void unlink_free_unknown_bucket(free_t *free_area) {
    return unlink_free(free_area, size_to_index_freeing(free_area->header.size - sizeof(header_t)));
}

static void *create_allocation_header(void *address, size_t offset, size_t size, void *left) {
    header_t *standalone = (header_t *)((char *)address + offset);
    standalone->left = untag(left);
    standalone->size = size;
    return standalone + 1;
}

static void FixLeftPointer(header_t *right, header_t *new_left) {
    int tag = (uintptr_t)right->left & 1;
    right->left = (header_t *)(((uintptr_t)new_left & ~1) | tag);
}

static void *large_alloc(size_t size) {
#ifdef CMPCT_DEBUG
    size_t requested_size = size;
#endif
    size = ROUNDUP(size, HEAP_ALIGN);
    free_t *free_area = NULL;
    lock();
    if (heap_grow(size, &free_area) < 0) {
        unlock();
        return 0;
    }
    void *result =
        create_allocation_header(free_area, 0, free_area->header.size, free_area->header.left);
    // Normally the 'remaining free space' counter would be decremented when we
    // unlink the free area from its bucket.  However in this case the free
    // area was too big to go in any bucket and we had it in our own
    // "free_area" variable so there is no unlinking and we have to adjust the
    // counter here.
    theheap.remaining -= free_area->header.size;
    unlock();
#ifdef CMPCT_DEBUG
    memset(result, ALLOC_FILL, requested_size);
    memset((char *)result + requested_size, PADDING_FILL,
           free_area->header.size - (requested_size + sizeof(header_t)));
#endif
    return result;
}

void cmpct_trim(void) {
    // Look at free list entries that are at least as large as one page plus a
    // header. They might be at the start or the end of a block, so we can trim
    // them and free the page(s).
    lock();
    for (int bucket = size_to_index_freeing(PAGE_SIZE); bucket < NUMBER_OF_BUCKETS; bucket++) {
        free_t *next;
        for (free_t *free_area = theheap.free_lists[bucket]; free_area != NULL; free_area = next) {
            DEBUG_ASSERT(free_area->header.size >= PAGE_SIZE + sizeof(header_t));
            next = free_area->next;
            header_t *right = right_header(&free_area->header);
            if (is_end_of_os_allocation((char *)right)) {
                char *old_os_allocation_end = (char *)ROUNDUP((uintptr_t)right, PAGE_SIZE);
                // The page will end with a smaller free list entry and a header-sized sentinel.
                char *new_os_allocation_end = (char *)ROUNDUP(
                    (uintptr_t)free_area + sizeof(header_t) + sizeof(free_t), PAGE_SIZE);
                size_t freed_up = old_os_allocation_end - new_os_allocation_end;
                DEBUG_ASSERT(IS_PAGE_ALIGNED(freed_up));
                // Rare, because we only look at large freelist entries, but unlucky rounding
                // could mean we can't actually free anything here.
                if (freed_up == 0) {
                    continue;
                }
                unlink_free(free_area, bucket);
                size_t new_free_size = free_area->header.size - freed_up;
                DEBUG_ASSERT(new_free_size >= sizeof(free_t));
                // Right sentinel, not free, stops attempts to coalesce right.
                create_allocation_header(free_area, new_free_size, 0, free_area);
                // Also puts it in the correct bucket.
                create_free_area(free_area, untag(free_area->header.left), new_free_size, NULL);
                page_free(new_os_allocation_end, freed_up >> PAGE_SIZE_SHIFT);
                theheap.size -= freed_up;
            } else if (is_start_of_os_allocation(untag(free_area->header.left))) {
                char *old_os_allocation_start = (char *)ROUNDDOWN((uintptr_t)free_area, PAGE_SIZE);
                // For the sentinel, we need at least one header-size of space between the page
                // edge and the first allocation to the right of the free area.
                char *new_os_allocation_start =
                    (char *)ROUNDDOWN((uintptr_t)(right - 1), PAGE_SIZE);
                size_t freed_up = new_os_allocation_start - old_os_allocation_start;
                DEBUG_ASSERT(IS_PAGE_ALIGNED(freed_up));
                // This should not happen because we only look at the large free list buckets.
                if (freed_up == 0) {
                    continue;
                }
                unlink_free(free_area, bucket);
                size_t sentinel_size = sizeof(header_t);
                size_t new_free_size = free_area->header.size - freed_up;
                if (new_free_size < sizeof(free_t)) {
                    sentinel_size += new_free_size;
                    new_free_size = 0;
                }
                // Left sentinel, not free, stops attempts to coalesce left.
                create_allocation_header(new_os_allocation_start, 0, sentinel_size, NULL);
                if (new_free_size == 0) {
                    FixLeftPointer(right, (header_t *)new_os_allocation_start);
                } else {
                    DEBUG_ASSERT(new_free_size >= sizeof(free_t));
                    char *new_free = new_os_allocation_start + sentinel_size;
                    // Also puts it in the correct bucket.
                    create_free_area(new_free, new_os_allocation_start, new_free_size, NULL);
                    FixLeftPointer(right, (header_t *)new_free);
                }
                page_free(old_os_allocation_start, freed_up >> PAGE_SIZE_SHIFT);
                theheap.size -= freed_up;
            }
        }
    }
    unlock();
}

void *cmpct_alloc(size_t size) {
    if (size == 0u) {
        return NULL;
    }

    if (size + sizeof(header_t) > (1u << HEAP_ALLOC_VIRTUAL_BITS)) {
        return large_alloc(size);
    }

    size_t rounded_up;
    int start_bucket = size_to_index_allocating(size, &rounded_up);

    rounded_up += sizeof(header_t);

    lock();
    int bucket = find_nonempty_bucket(start_bucket);
    if (bucket == -1) {
        // Grow heap by at least 12% if we can.
        size_t growby = MIN(1u << HEAP_ALLOC_VIRTUAL_BITS,
                            MAX(theheap.size >> 3, MAX(HEAP_GROW_SIZE, rounded_up)));
        while (heap_grow(growby, NULL) < 0) {
            if (growby <= rounded_up) {
                unlock();
                return NULL;
            }
            growby = MAX(growby >> 1, rounded_up);
        }
        bucket = find_nonempty_bucket(start_bucket);
    }
    free_t *head = theheap.free_lists[bucket];
    size_t left_over = head->header.size - rounded_up;
    // We can't carve off the rest for a new free space if it's smaller than the
    // free-list linked structure.  We also don't carve it off if it's less than
    // 1.6% the size of the allocation.  This is to avoid small long-lived
    // allocations being placed right next to large allocations, hindering
    // coalescing and returning pages to the OS.
    if (left_over >= sizeof(free_t) && left_over > (size >> 6)) {
        header_t *right = right_header(&head->header);
        unlink_free(head, bucket);
        void *free = (char *)head + rounded_up;
        create_free_area(free, head, left_over, NULL);
        FixLeftPointer(right, (header_t *)free);
        head->header.size -= left_over;
    } else {
        unlink_free(head, bucket);
    }
    void *result = create_allocation_header(head, 0, head->header.size, head->header.left);
#ifdef CMPCT_DEBUG
    memset(result, ALLOC_FILL, size);
    memset(((char *)result) + size, PADDING_FILL, rounded_up - size - sizeof(header_t));
#endif
    unlock();
    return result;
}

void *cmpct_memalign(size_t size, size_t alignment) {
    if (alignment < HEAP_ALIGN) {
        return cmpct_alloc(size);
    }
    size_t padded_size = size + alignment + sizeof(free_t) + sizeof(header_t);
    char *unaligned = (char *)cmpct_alloc(padded_size);
    if (unaligned == NULL) {
        return NULL;
    }
    lock();
    size_t mask = alignment - 1;
    uintptr_t payload_int = (uintptr_t)unaligned + sizeof(free_t) + sizeof(header_t) + mask;
    char *payload = (char *)(payload_int & ~mask);
    if (unaligned != payload) {
        header_t *unaligned_header = (header_t *)unaligned - 1;
        header_t *header = (header_t *)payload - 1;
        size_t left_over = payload - unaligned;
        create_allocation_header(header, 0, unaligned_header->size - left_over, unaligned_header);
        header_t *right = right_header(unaligned_header);
        unaligned_header->size = left_over;
        FixLeftPointer(right, header);
        unlock();
        cmpct_free(unaligned);
    } else {
        unlock();
    }
    // TODO: Free the part after the aligned allocation.
    return payload;
}

void cmpct_free(void *payload) {
    if (payload == NULL) {
        return;
    }
    header_t *header = (header_t *)payload - 1;
    DEBUG_ASSERT(!is_tagged_as_free(header)); // Double free!
    size_t size = header->size;
    lock();
    header_t *left = header->left;
    if (left != NULL && is_tagged_as_free(left)) {
        // Coalesce with left free object.
        unlink_free_unknown_bucket((free_t *)left);
        header_t *right = right_header(header);
        if (is_tagged_as_free(right)) {
            // Coalesce both sides.
            unlink_free_unknown_bucket((free_t *)right);
            header_t *right_right = right_header(right);
            FixLeftPointer(right_right, left);
            free_memory(left, left->left, left->size + size + right->size);
        } else {
            // Coalesce only left.
            FixLeftPointer(right, left);
            free_memory(left, left->left, left->size + size);
        }
    } else {
        header_t *right = right_header(header);
        if (is_tagged_as_free(right)) {
            // Coalesce only right.
            header_t *right_right = right_header(right);
            unlink_free_unknown_bucket((free_t *)right);
            FixLeftPointer(right_right, header);
            free_memory(header, left, size + right->size);
        } else {
            free_memory(header, left, size);
        }
    }
    unlock();
}

void *cmpct_realloc(void *payload, size_t size) {
    if (payload == NULL) {
        return cmpct_alloc(size);
    }
    header_t *header = (header_t *)payload - 1;
    size_t old_size = header->size - sizeof(header_t);
    void *new_payload = cmpct_alloc(size);
    memcpy(new_payload, payload, MIN(size, old_size));
    cmpct_free(payload);
    return new_payload;
}

static void add_to_heap(void *new_area, size_t size, free_t **bucket) {
    void *top = (char *)new_area + size;
    header_t *left_sentinel = (header_t *)new_area;
    // Not free, stops attempts to coalesce left.
    create_allocation_header(left_sentinel, 0, sizeof(header_t), NULL);
    header_t *new_header = left_sentinel + 1;
    size_t free_size = size - 2 * sizeof(header_t);
    create_free_area(new_header, left_sentinel, free_size, bucket);
    header_t *right_sentinel = (header_t *)(top - sizeof(header_t));
    // Not free, stops attempts to coalesce right.
    create_allocation_header(right_sentinel, 0, 0, new_header);
}

// Create a new free-list entry of at least size bytes (including the
// allocation header).  Called with the lock, apart from during init.
static ssize_t heap_grow(size_t size, free_t **bucket) {
    // The new free list entry will have a header on each side (the
    // sentinels) so we need to grow the gross heap size by this much more.
    size += 2 * sizeof(header_t);
    size = ROUNDUP(size, PAGE_SIZE);
    void *ptr = page_alloc(size >> PAGE_SIZE_SHIFT, PAGE_ALLOC_ANY_ARENA);
    if (ptr == NULL) {
        return -1;
    }
    theheap.size += size;
    LTRACEF("growing heap by 0x%zx bytes, new ptr %p\n", size, ptr);
    add_to_heap(ptr, size, bucket);
    return size;
}

void cmpct_init(void) {
    LTRACE_ENTRY;

    // Create a mutex.
    mutex_init(&theheap.lock);

    // Initialize the free list.
    for (int i = 0; i < NUMBER_OF_BUCKETS; i++) {
        theheap.free_lists[i] = NULL;
    }
    for (int i = 0; i < BUCKET_WORDS; i++) {
        theheap.free_list_bits[i] = 0;
    }

    size_t initial_alloc = HEAP_GROW_SIZE - 2 * sizeof(header_t);

    theheap.remaining = 0;

    heap_grow(initial_alloc, NULL);
}
