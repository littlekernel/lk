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
#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/spinlock.h>
#include <lib/cmpctmalloc.h>
#include <lib/heap.h>
#include <lib/page_alloc.h>

// Malloc implementation tuned for space.
//
// Allocation strategy takes place with a global mutex.  Freelist entries are
// kept in linked lists with 8 different sizes per binary order of magnitude
// and the header size is two words with eager coalescing on free.

#define LOCAL_TRACE 0

#define ALLOC_FILL 0x99
#define FREE_FILL 0x77
#define PADDING_FILL 0x55

#if WITH_KERNEL_VM && !defined(HEAP_GROW_SIZE)
#define HEAP_GROW_SIZE (4 * 1024 * 1024) /* Grow aggressively */
#elif !defined(HEAP_GROW_SIZE)
#define HEAP_GROW_SIZE (4 * 1024) /* Grow less aggressively */
#endif

STATIC_ASSERT(IS_PAGE_ALIGNED(HEAP_GROW_SIZE));

// Enough in practice even for 64 bit CPUs.
#define HEAP_ALLOC_VIRTUAL_BITS 48

// Buckets for allocations.  The smallest buckets are 16, 24, etc. up to 120
// bytes.  After that we round up to the nearest size that can be written
// /^0*1...0*$/, giving 8 buckets per order of binary magnitude.  The freelist
// entries in a given bucket have at least the given size.  For 64 bit we go up
// to 48 bits, 128 extra buckets.  On 64 bit, the 16 byte bucket is useless,
// since the freelist header is 32 bytes, but we have it for simplicity.
#define NUMBER_OF_BUCKETS (1 + (HEAP_ALLOC_VIRTUAL_BITS - 5) * 8)

// All individual memory areas on the heap start with this.
typedef struct header_struct {
	struct header_struct *left;  // Pointer to the previous area in memory order.
	size_t size;
} header_t;

struct free_heap_chunk {
	header_t header;
	struct free_heap_chunk *next;
	struct free_heap_chunk *prev;
};

struct heap {
	void *base;
	void *top;
	size_t remaining;
	size_t low_watermark;
	mutex_t lock;
	struct free_heap_chunk *free_lists[NUMBER_OF_BUCKETS];
	// We have some 32 bit words that tell us whether there is an entry in the
	// freelist.
#define BUCKET_WORDS (((NUMBER_OF_BUCKETS) + 31) >> 5)
	uint32_t free_list_bits[BUCKET_WORDS];
	struct list_node delayed_free_list;
	spin_lock_t delayed_free_lock;
};

// heap static vars
static struct heap theheap;

static ssize_t heap_grow(size_t len);

static void lock(void)
{
	mutex_acquire(&theheap.lock);
}

static void unlock(void)
{
	mutex_release(&theheap.lock);
}

static void dump_chunk(header_t *header)
{
	dprintf(INFO, "\t\tbase %p, end 0x%lx, len 0x%zx\n", header, (vaddr_t)header + header->size, header->size);
}

void cmpct_dump(void)
{
	lock();
	dprintf(INFO, "Heap dump (using cmpctmalloc):\n");
	dprintf(INFO, "\tbase %p, len 0x%zx\n", theheap.base, theheap.top - theheap.base);
	dprintf(INFO, "\tfree list:\n");
	for (int i = 0; i < NUMBER_OF_BUCKETS; i++) {
		bool header_printed = false;
		struct free_heap_chunk *chunk = theheap.free_lists[i];
		for (; chunk != NULL; chunk = chunk->next) {
			if (!header_printed) {
				dprintf(INFO, "\tbucket %d\n", i);
				header_printed = true;
			}
			dump_chunk(&chunk->header);
		}
	}
	unlock();

	dprintf(INFO, "\tdelayed free list:\n");
	spin_lock_saved_state_t state;
	spin_lock_irqsave(&theheap.delayed_free_lock, state);
	struct list_node *node;
	list_for_every(&theheap.delayed_free_list, node) {
		dump_chunk((header_t *)node - 1);
	}
	spin_unlock_irqrestore(&theheap.delayed_free_lock, state);
}

static int size_to_index_helper(
	size_t size, size_t *rounded_up_out, int adjust, int increment)
{
	// First buckets are simply 8-spaced up to 128.
	if (size <= 128) {
		if (sizeof(size_t) == 8u && size <= 32) {
			*rounded_up_out = 32;
		} else {
			*rounded_up_out = size;
		}
		// With the 8 byte header, no allocation is smaller than 16 bytes, so
		// the first bucket is for 16 byte spaces.  For 64 bit, the free list
		// chunk is 32 bytes, so no allocation can be smaller than that
		// (otherwise how to free it), but we have empty 16 and 24 byte buckets
		// for simplicity.
		return (size >> 3) - 2;
	}

	// We are going to go up to the next size to round up, but if we hit a
	// bucket size exactly we don't want to go up. By subtracting 8 here, we
	// will do the
	// right thing (the carry propagates up for the round numbers we are
	// interested in).
	size -= adjust;
	// After 128 the buckets are logarithmically spaced, every 16 up to 256,
	// every 32 up to 512 etc.  This can be thought of as rows of 8 buckets.
	// GCC intrinsic count-leading-zeros.
	// Eg. 128-255 has 24 leading zeros and we want row to be 4.
	unsigned row = sizeof(size_t) * 8 - 4 - __builtin_clzl(size);
	// For row 4 we want to shift down 3 bits.
	unsigned column = (size >> row) & 7;
	int row_column = (row << 3) | column;
	row_column += increment;
	size = (8 + (row_column & 7)) << (row_column >> 3);
	*rounded_up_out = size;
	// We start with 14 buckets, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96,
	// 104, 112, 120.  Then we have row 4, sizes 128 and up, with the
	// row-column 8 and up.
	return row_column + 14 - 8;
}

static int size_to_index_rounding_up(size_t size, size_t *rounded_up_out)
{
	size_t rounded = ROUNDUP(size, 8);
	return size_to_index_helper(rounded, rounded_up_out, -8, 1);
}

static int size_to_index_rounding_down(size_t size)
{
	size_t dummy;
	return size_to_index_helper(size, &dummy, 0, 0);
}

inline header_t *tag_as_free(void *left)
{
	return (header_t *)((uintptr_t)left | 1);
}

inline bool is_tagged_as_free(header_t *header)
{
	return ((uintptr_t)(header->left) & 1) != 0;
}

inline header_t *untag(void *left)
{
	return (header_t *)((uintptr_t)left & ~1);
}

inline header_t *right_header(header_t *header)
{
	return (header_t *)((char *)header + header->size);
}

inline static void set_free_list_bit(int index)
{
	theheap.free_list_bits[index >> 5] |= (1u << (31 - (index & 0x1f)));
}

inline static void clear_free_list_bit(int index)
{
	theheap.free_list_bits[index >> 5] &= ~(1u << (31 - (index & 0x1f)));
}

static int find_nonempty_bucket(int index)
{
	uint32_t mask = (1u << (31 - (index & 0x1f))) - 1;
	mask = mask * 2 + 1;
	mask &= theheap.free_list_bits[index >> 5];
	if (mask != 0) return (index & ~0x1f) + __builtin_clz(mask);
	for (index = ROUNDUP(index + 1, 32); index <= NUMBER_OF_BUCKETS; index += 32) {
		mask = theheap.free_list_bits[index >> 5];
		if (mask != 0u) return index + __builtin_clz(mask);
	}
	return -1;
}

bool is_start_of_os_allocation(header_t *address)
{
	if (address->size != sizeof(header_t)) return false;
	return address->left == untag(NULL);
}

static void create_free_heap_chunk(void *address, void *left, size_t size)
{
	struct free_heap_chunk *chunk = (struct free_heap_chunk *)address;
	chunk->header.size = size;
	chunk->header.left = tag_as_free(left);
	int index = size_to_index_rounding_down(size);
	struct free_heap_chunk *old_head = theheap.free_lists[index];
	if (old_head != NULL) old_head->prev = chunk;
	chunk->next = old_head;
	chunk->prev = NULL;
	theheap.free_lists[index] = chunk;
	set_free_list_bit(index);
	theheap.remaining += size;
#ifdef DEBUG
	memset(chunk + 1, FREE_FILL, size - sizeof(struct free_heap_chunk));
#endif
}

bool is_end_of_os_allocation(char *address)
{
	return ((header_t *)address)->size == 0;
}

void free_to_os(header_t *header, size_t size)
{
	// TODO.
}

static void free_memory(void *address, void *left, size_t size)
{
/* TODO: Give back to OS?
	if (is_start_of_os_allocation(untag(left)) &&
			(char *)address - (char *)left == sizeof(header_t) &&
			is_end_of_os_allocation((char *)address + size)) {
		free_to_os(untag(left), size + 2 * sizeof(header_t));
	} else
*/
	{
		create_free_heap_chunk(address, left, size);
	}
}

static void reduce_remaining(size_t adjustment)
{
	theheap.remaining -= adjustment;
	if (theheap.low_watermark > theheap.remaining) {
		theheap.low_watermark = theheap.remaining;
	}
}

static void unlink_free(struct free_heap_chunk *chunk, int bucket)
{
	reduce_remaining(chunk->header.size);
	struct free_heap_chunk *next = chunk->next;
	struct free_heap_chunk *prev = chunk->prev;
	if (theheap.free_lists[bucket] == chunk) {
		theheap.free_lists[bucket] = next;
		if (next == NULL) clear_free_list_bit(bucket);
	}
	if (prev != NULL) prev->next = next;
	if (next != NULL) next->prev = prev;
}

static void unlink_free_unknown_bucket(struct free_heap_chunk *chunk)
{
	return unlink_free(chunk, size_to_index_rounding_down(chunk->header.size));
}

static void *create_allocation_header(
		void *address, size_t offset, size_t size, void *left) {
	header_t *standalone = (header_t *)((char *)address + offset);
	standalone->left = untag(left);
	standalone->size = size;
	return standalone + 1;
}

void FixLeftPointer(header_t *right, header_t *new_left)
{
	int tag = (uintptr_t)right->left & 1;
	right->left = (header_t *)(((uintptr_t)new_left & ~1) | tag);
}

static void cmpct_test(void)
{
	void *ptr[16];

	ptr[0] = cmpct_alloc(8);
	ptr[1] = cmpct_alloc(32);
	ptr[2] = cmpct_alloc(7);
	ptr[3] = cmpct_alloc(0);
	ptr[4] = cmpct_alloc(98713);
	ptr[5] = cmpct_alloc(16);

	cmpct_free(ptr[5]);
	cmpct_free(ptr[1]);
	cmpct_free(ptr[3]);
	cmpct_free(ptr[0]);
	cmpct_free(ptr[4]);
	cmpct_free(ptr[2]);

	cmpct_dump();

	int i;
	for (i=0; i < 16; i++)
		ptr[i] = 0;

	for (i=0; i < 32768; i++) {
		unsigned int index = (unsigned int)rand() % 16;

		if ((i % (16*1024)) == 0)
			printf("pass %d\n", i);

//		printf("index 0x%x\n", index);
		if (ptr[index]) {
//			printf("freeing ptr[0x%x] = %p\n", index, ptr[index]);
			cmpct_free(ptr[index]);
			ptr[index] = 0;
		}
		unsigned int align = 1 << ((unsigned int)rand() % 8);
		ptr[index] = cmpct_memalign((unsigned int)rand() % 32768, align);
//		printf("ptr[0x%x] = %p, align 0x%x\n", index, ptr[index], align);

		DEBUG_ASSERT(((addr_t)ptr[index] % align) == 0);
//		cmpct_dump();
	}

	for (i=0; i < 16; i++) {
		if (ptr[i])
			cmpct_free(ptr[i]);
	}

	cmpct_dump();
}

static void cmpct_free_delayed_list(void)
{
	struct list_node list;

	list_initialize(&list);

	spin_lock_saved_state_t state;
	spin_lock_irqsave(&theheap.delayed_free_lock, state);

	struct list_node *node;
	while ((node = list_remove_head(&theheap.delayed_free_list))) {
		list_add_head(&list, node);
	}
	spin_unlock_irqrestore(&theheap.delayed_free_lock, state);

	while ((node = list_remove_head(&list))) {
		LTRACEF("freeing chunk %p\n", node);
		cmpct_free(node);
	}
}

void *cmpct_alloc(size_t size)
{
	if (size == 0u) return NULL;

	// Guard against some overflows.
	if (size > SIZE_MAX / 2) return NULL;

	// deal with the pending free list
	if (unlikely(!list_is_empty(&theheap.delayed_free_list))) {
		cmpct_free_delayed_list();
	}

	size += sizeof(header_t);
	size_t rounded_up;
	int start_bucket = size_to_index_rounding_up(size, &rounded_up);

	lock();
	int bucket = find_nonempty_bucket(start_bucket);
	if (bucket == -1) {
		size_t growby = MAX(HEAP_GROW_SIZE, ROUNDUP(rounded_up, PAGE_SIZE));

		ssize_t err = heap_grow(growby);
		if (err < 0) {
			unlock();
			return NULL;
		}
		bucket = find_nonempty_bucket(start_bucket);
	}
	struct free_heap_chunk *head = theheap.free_lists[bucket];
	size_t left_over = head->header.size - rounded_up;
	if (left_over >= sizeof(struct free_heap_chunk)) {
		header_t *right = right_header(&head->header);
		unlink_free(head, bucket);
		void *free = (char *)head + rounded_up;
		create_free_heap_chunk(free, head, left_over);
		FixLeftPointer(right, (header_t *)free);
		head->header.size -= left_over;
	} else {
		unlink_free(head, bucket);
	}
	void *result =
		create_allocation_header(head, 0, head->header.size, head->header.left);
#ifdef DEBUG
	memset(result, ALLOC_FILL, size - sizeof(header_t));
	memset(((char *)result) + size - sizeof(header_t), PADDING_FILL, rounded_up - size);
#endif
	unlock();
	return result;
}

void *cmpct_memalign(size_t size, size_t alignment)
{
	if (alignment < 8) return cmpct_alloc(size);
	size_t padded_size =
		size + alignment + sizeof(struct free_heap_chunk) + sizeof(header_t);
	char *unaligned = (char *)cmpct_alloc(padded_size);
	lock();
	size_t mask = alignment - 1;
	uintptr_t payload_int = (uintptr_t)unaligned + sizeof(struct free_heap_chunk) +
		sizeof(header_t) + mask;
	char *payload = (char *)(payload_int & ~mask);
	if (unaligned != payload) {
		header_t *unaligned_header = (header_t *)unaligned - 1;
		header_t *header = (header_t *)payload - 1;
		size_t left_over = payload - unaligned;
		create_allocation_header(
			header, 0, unaligned_header->size - left_over, unaligned_header);
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

void cmpct_free(void *payload)
{
	if (payload == NULL) return;
	header_t *header = (header_t *)payload - 1;
	size_t size = header->size;
	lock();
	header_t *left = header->left;
	if (left != NULL && is_tagged_as_free(left)) {
		// Coalesce with left free object.
		unlink_free_unknown_bucket((struct free_heap_chunk *)left);
		header_t *right = right_header(header);
		if (is_tagged_as_free(right)) {
			// Coalesce both sides.
			unlink_free_unknown_bucket((struct free_heap_chunk *)right);
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
			unlink_free_unknown_bucket((struct free_heap_chunk *)right);
			FixLeftPointer(right_right, header);
			free_memory(header, left, size + right->size);
		} else {
			free_memory(header, left, size);
		}
	}
	unlock();
}

void *cmpct_realloc(void *payload, size_t size)
{
	if (payload == NULL) return cmpct_alloc(size);
	header_t *header = (header_t *)payload - 1;
	size_t old_size = header->size - sizeof(header_t);
	void *new_payload = cmpct_alloc(size);
	memcpy(new_payload, payload, MIN(size, old_size));
	cmpct_free(payload);
	return new_payload;
}

void cmpct_delayed_free(void *ptr)
{
	if (ptr == NULL) return;

	LTRACEF("ptr %p\n", ptr);

	// Check the allocation header.
	header_t *header = (header_t *)ptr - 1;
	DEBUG_ASSERT(header->size >= sizeof(header_t) + sizeof(struct list_node));
	DEBUG_ASSERT(!is_tagged_as_free(header));

	// We leave the header marked as allocated so nobody tries to coalesce with
	// the area duing the delay.  The nodes for linking up are placed in the old
	// payload area, leaving the header untouched.
	struct list_node *node = (struct list_node *)ptr;

	spin_lock_saved_state_t state;
	spin_lock_irqsave(&theheap.delayed_free_lock, state);
	list_add_head(&theheap.delayed_free_list, node);
	spin_unlock_irqrestore(&theheap.delayed_free_lock, state);
}

static void add_to_heap(void *new_area, size_t size)
{
	lock();
	void *top = (char *)new_area + size;
	header_t *left_sentinel = (header_t *)new_area;
	// Not free, stops attempts to coalesce left.
	create_allocation_header(left_sentinel, 0, sizeof(header_t), NULL);
	header_t *new_header = left_sentinel + 1;
	size_t free_size = size - 2 * sizeof(header_t);
	create_free_heap_chunk(new_header, left_sentinel, free_size);
	header_t *right_sentinel = (header_t *)(top - sizeof(header_t));
	// Not free, stops attempts to coalesce right.
	create_allocation_header(right_sentinel, 0, 0, new_header);
	if (theheap.base == (void *)-1l) {
		theheap.base = new_area;
		theheap.top = top;
	} else {
		theheap.base = MIN(theheap.base, new_area);
		theheap.top = MAX(theheap.top, top);
	}
	unlock();
}

static ssize_t heap_grow(size_t size)
{
	size = ROUNDUP(size, PAGE_SIZE);
	void *ptr = page_alloc(size >> PAGE_SIZE_SHIFT);
	if (ptr == NULL) return -1;
	LTRACEF("growing heap by 0x%zx bytes, new ptr %p\n", size, ptr);
	add_to_heap(ptr, size);
	return size;
}

void cmpct_init(void)
{
	LTRACE_ENTRY;

	// create a mutex
	mutex_init(&theheap.lock);

	// initialize the free list
	for (int i = 0; i < NUMBER_OF_BUCKETS; i++) {
		theheap.free_lists[i] = NULL;
	}
	for (int i = 0; i < BUCKET_WORDS; i++) {
		theheap.free_list_bits[i] = 0;
	}

	// initialize the delayed free list
	list_initialize(&theheap.delayed_free_list);
	spin_lock_init(&theheap.delayed_free_lock);

	// set the heap range
	theheap.base = (void *)-1l;
	theheap.remaining = 0;
	theheap.low_watermark = (size_t)-1l;
	heap_grow(HEAP_GROW_SIZE);
}

/* vim: set ts=4 sw=4 noexpandtab: */
