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
#define HEAP_GROW_SIZE (1 * 1024 * 1024) /* Grow aggressively */
#elif !defined(HEAP_GROW_SIZE)
#define HEAP_GROW_SIZE (4 * 1024) /* Grow less aggressively */
#endif

STATIC_ASSERT(IS_PAGE_ALIGNED(HEAP_GROW_SIZE));

// Individual allocations above 4Mbytes are just fetched directly from the
// block allocator.
#define HEAP_ALLOC_VIRTUAL_BITS 22

// When we grow the heap we have to have somewhere in the freelist to put the
// resulting freelist entry, so the freelist has to have a certain number of
// buckets.
STATIC_ASSERT(HEAP_GROW_SIZE <= (1u << HEAP_ALLOC_VIRTUAL_BITS));

// Buckets for allocations.  The smallest 14 buckets are 16, 24, etc. up to 120
// bytes.  After that we round up to the nearest size that can be written
// /^0*1...0*$/, giving 8 buckets per order of binary magnitude.  The freelist
// entries in a given bucket have at least the given size.  For 64 bit we go up
// to 48 bits, 128 extra buckets.  On 64 bit, the 16 byte bucket is useless,
// since the freelist header is 32 bytes, but we have it for simplicity.
#define NUMBER_OF_BUCKETS (1 + 14 + (HEAP_ALLOC_VIRTUAL_BITS - 7) * 8)

// All individual memory areas on the heap start with this.
typedef struct header_struct {
	struct header_struct *left;  // Pointer to the previous area in memory order.
	size_t size;
} header_t;

typedef struct free_struct {
	header_t header;
	struct free_struct *next;
	struct free_struct *prev;
} free_t;

struct heap {
	size_t size;
	size_t remaining;
	mutex_t lock;
	free_t *free_lists[NUMBER_OF_BUCKETS];
	// We have some 32 bit words that tell us whether there is an entry in the
	// freelist.
#define BUCKET_WORDS (((NUMBER_OF_BUCKETS) + 31) >> 5)
	uint32_t free_list_bits[BUCKET_WORDS];
};

// Heap static vars.
static struct heap theheap;

static ssize_t heap_grow(size_t len, free_t **bucket);

static void lock(void)
{
	mutex_acquire(&theheap.lock);
}

static void unlock(void)
{
	mutex_release(&theheap.lock);
}

static void dump_free(header_t *header)
{
	dprintf(INFO, "\t\tbase %p, end 0x%lx, len 0x%zx\n", header, (vaddr_t)header + header->size, header->size);
}

void cmpct_dump(void)
{
	lock();
	dprintf(INFO, "Heap dump (using cmpctmalloc):\n");
	dprintf(INFO, "\tsize %lu, remaining %lu\n",
			(unsigned long)theheap.size,
			(unsigned long)theheap.remaining);

	dprintf(INFO, "\tfree list:\n");
	for (int i = 0; i < NUMBER_OF_BUCKETS; i++) {
		bool header_printed = false;
		free_t *free_area = theheap.free_lists[i];
		for (; free_area != NULL; free_area = free_area->next) {
			if (!header_printed) {
				dprintf(INFO, "\tbucket %d\n", i);
				header_printed = true;
			}
			dump_free(&free_area->header);
		}
	}
	unlock();
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
		// struct is 32 bytes, so no allocation can be smaller than that
		// (otherwise how to free it), but we have empty 16 and 24 byte buckets
		// for simplicity.
		return (size >> 3) - 2;
	}

	// We are going to go up to the next size to round up, but if we hit a
	// bucket size exactly we don't want to go up. By subtracting 8 here, we
	// will do the right thing (the carry propagates up for the round numbers
	// we are interested in).
	size += adjust;
	// After 128 the buckets are logarithmically spaced, every 16 up to 256,
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
	// We start with 14 buckets, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96,
	// 104, 112, 120.  Then we have row 4, sizes 128 and up, with the
	// row-column 8 and up.
	int answer = row_column + 14 - 32;
	DEBUG_ASSERT(answer < NUMBER_OF_BUCKETS);
	return answer;
}

// Round up size to next bucket when allocating.
static int size_to_index_allocating(size_t size, size_t *rounded_up_out)
{
	size_t rounded = ROUNDUP(size, 8);
	return size_to_index_helper(rounded, rounded_up_out, -8, 1);
}

// Round down size to next bucket when freeing.
static int size_to_index_freeing(size_t size)
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

bool is_start_of_os_allocation(header_t *header)
{
	uintptr_t address = (uintptr_t)header;
	if ((address & (PAGE_SIZE - 1)) != 0) return false;
	if (header->size != sizeof(header_t)) return false;
	return header->left == untag(NULL);
}

static void create_free_area(void *address, void *left, size_t size, free_t **bucket)
{
	free_t *free_area = (free_t *)address;
	free_area->header.size = size;
	free_area->header.left = tag_as_free(left);
	if (bucket == NULL) {
		int index = size_to_index_freeing(size);
		set_free_list_bit(index);
		bucket = &theheap.free_lists[index];
	}
	free_t *old_head = *bucket;
	if (old_head != NULL) old_head->prev = free_area;
	free_area->next = old_head;
	free_area->prev = NULL;
	*bucket = free_area;
	theheap.remaining += size;
#ifdef DEBUG
	memset(free_area + 1, FREE_FILL, size - sizeof(free_t));
#endif
}

bool is_end_of_os_allocation(char *address)
{
	return ((header_t *)address)->size == 0;
}

void free_to_os(header_t *header, size_t size)
{
	DEBUG_ASSERT(size == ROUNDUP(size, PAGE_SIZE));
	page_free(header, size >> PAGE_SIZE_SHIFT);
	theheap.size -= size;
}

static void free_memory(void *address, void *left, size_t size)
{
	if (is_start_of_os_allocation(untag(left)) &&
			(char *)address - (char *)left == sizeof(header_t) &&
			is_end_of_os_allocation((char *)address + size)) {
		free_to_os(untag(left), size + 2 * sizeof(header_t));
	} else {
		create_free_area(address, left, size, NULL);
	}
}

static void unlink_free(free_t *free_area, int bucket)
{
	theheap.remaining -= free_area->header.size;
	ASSERT(theheap.remaining < 4000000000u);
	free_t *next = free_area->next;
	free_t *prev = free_area->prev;
	if (theheap.free_lists[bucket] == free_area) {
		theheap.free_lists[bucket] = next;
		if (next == NULL) clear_free_list_bit(bucket);
	}
	if (prev != NULL) prev->next = next;
	if (next != NULL) next->prev = prev;
}

static void unlink_free_unknown_bucket(free_t *free_area)
{
	return unlink_free(free_area, size_to_index_freeing(free_area->header.size));
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

void cmpct_test_buckets(void) {
	size_t rounded;
	unsigned bucket;
	// Check for the 8-spaced buckets up to 128.
	for (unsigned i = 9; i <= 128; i++) {
		// Round up when allocating.
		bucket = size_to_index_allocating(i, &rounded);
		unsigned expected = (ROUNDUP(i, 8) >> 3) - 2;
		ASSERT(bucket == expected);
		ASSERT(rounded == ROUNDUP(rounded, 8));
		ASSERT(rounded >= i);
		if (i > sizeof(free_t) - 8) {
			// Once we get above the size of the free area struct (4 words), we
			// won't round up much for these small size.
			ASSERT(rounded - i < 8);
		}
		// Only rounded sizes are freed.
		if ((i & 7) == 0) {
			// Up to size 128 we have exact buckets for each multiple of 8.
			ASSERT(bucket == (unsigned)size_to_index_freeing(i));
		}
	}
	int bucket_base = 6;
	for (unsigned j = 16; j < 1024; j *= 2, bucket_base += 8) {
		// Note the "<=", which ensures that we test the powers of 2 twice to ensure
		// that both ways of calculating the bucket number match.
		for (unsigned i = j * 8; i <= j * 16; i++) {
			// Round up to j multiple in this range when allocating.
			bucket = size_to_index_allocating(i, &rounded);
			unsigned expected = bucket_base + ROUNDUP(i, j) / j;
			ASSERT(bucket == expected);
			ASSERT(rounded == ROUNDUP(rounded, j));
			ASSERT(rounded >= i);
			ASSERT(rounded - i < j);
			// Only 8-rounded sizes are freed or chopped off the end of a free area
			// when allocating.
			if ((i & 7) == 0) {
				// When freeing, if we don't hit the size of the bucket precisely,
				// we have to put the free space into a smaller bucket, because
				// the buckets have entries that will always be big enough for
				// the corresponding allocation size (so we don't have to
				// traverse the free chains to find a big enough one).
				if ((i % j) == 0) {
					ASSERT((int)bucket == size_to_index_freeing(i));
				} else {
					ASSERT((int)bucket - 1 == size_to_index_freeing(i));
				}
			}
		}
	}
}

void cmpct_test_get_back_newly_freed_helper(size_t size)
{
	void *allocated = cmpct_alloc(size);
	if (allocated == NULL) return;
	char *allocated2 = cmpct_alloc(8);
	char *expected_position = (char *)allocated + size;
	if (allocated2 < expected_position || allocated2 > expected_position + 128) {
		// If the allocated2 allocation is not in the same OS allocation as the
		// first allocation then the test may not work as expected (the memory
		// may be returned to the OS when we free the first allocation, and we
		// might not get it back).
		cmpct_free(allocated);
		cmpct_free(allocated2);
		return;
	}

	cmpct_free(allocated);
	void *allocated3 = cmpct_alloc(size);
	// To avoid churn and fragmentation we would want to get the newly freed
	// memory back again when we allocate the same size shortly after.
	ASSERT(allocated3 == allocated);
	cmpct_free(allocated2);
	cmpct_free(allocated3);
}

void cmpct_test_get_back_newly_freed(void)
{
	size_t increment = 16;
	for (size_t i = 128; i <= 0x8000000; i *= 2, increment *= 2) {
		for (size_t j = i; j < i * 2; j += increment) {
			cmpct_test_get_back_newly_freed_helper(i - 8);
			cmpct_test_get_back_newly_freed_helper(i);
			cmpct_test_get_back_newly_freed_helper(i + 1);
		}
	}
	for (size_t i = 1024; i <= 2048; i++) {
		cmpct_test_get_back_newly_freed_helper(i);
	}
}

void cmpct_test(void)
{
	cmpct_test_buckets();
	cmpct_test_get_back_newly_freed();
	cmpct_dump();
	void *ptr[16];

	ptr[0] = cmpct_alloc(8);
	ptr[1] = cmpct_alloc(32);
	ptr[2] = cmpct_alloc(7);
	cmpct_trim();
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
	cmpct_trim();
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

void *large_alloc(size_t size)
{
	size = ROUNDUP(size, 8);
	free_t *free_area = NULL;
	lock();
	heap_grow(size, &free_area);
	void *result =
		create_allocation_header(free_area, 0, free_area->header.size, free_area->header.left);
	theheap.remaining -= free_area->header.size;
	unlock();
	return result;
}

void cmpct_trim(void)
{
	// Look at free list entries that are strictly larger than one page. They
	// might be at the start or the end of a block, so we can trim them and
	// free the page(s).
	lock();
	for (int bucket = 1 + size_to_index_freeing(PAGE_SIZE);
		 bucket < NUMBER_OF_BUCKETS;
		 bucket++) {
		free_t * next;
		for (free_t *free_area = theheap.free_lists[bucket];
			 free_area != NULL;
			 free_area = next) {
			next = free_area->next;
			header_t *right = right_header(&free_area->header);
			if (is_end_of_os_allocation((char *)right)) {
				unlink_free(free_area, bucket);
				char *old_end_of_os_allocation = (char *)ROUNDUP((uintptr_t)right, PAGE_SIZE);
				// The page ends with a free list entry and a header-sized sentinel.
				char *new_end_of_os_allocation =
					(char *)ROUNDUP((uintptr_t)free_area + sizeof(header_t) + sizeof(free_t),
									PAGE_SIZE);
				DEBUG_ASSERT(old_end_of_os_allocation != new_end_of_os_allocation);
				size_t freed_up = old_end_of_os_allocation - new_end_of_os_allocation;
				size_t new_size = free_area->header.size - freed_up;
				// Right sentinel, not free, stops attempts to coalesce right.
				create_allocation_header(free_area, new_size, 0, free_area);
				// Also puts it in the right bucket.
				create_free_area(free_area, untag(free_area->header.left), new_size, NULL);
				page_free(new_end_of_os_allocation, freed_up >> PAGE_SIZE_SHIFT);
				theheap.size -= freed_up;
			} else if (is_start_of_os_allocation(untag(free_area->header.left))) {
				unlink_free(free_area, bucket);
				char *old_start_of_os_allocation =
					(char *)ROUNDDOWN((uintptr_t)free_area, PAGE_SIZE);
				char *new_start_of_os_allocation =
					(char *)ROUNDDOWN((uintptr_t)right - sizeof(free_t), PAGE_SIZE);
				DEBUG_ASSERT(old_start_of_os_allocation != new_start_of_os_allocation);
				size_t freed_up = new_start_of_os_allocation - old_start_of_os_allocation;
				size_t new_size = free_area->header.size - freed_up;
				// Left sentinel, not free, stops attempts to coalesce left.
				create_allocation_header(new_start_of_os_allocation, 0, sizeof(header_t), NULL);
				// Also puts it in the right bucket.
				create_free_area(new_start_of_os_allocation + sizeof(header_t), new_start_of_os_allocation, new_size, NULL);
				page_free(old_start_of_os_allocation, freed_up >> PAGE_SIZE_SHIFT);
				theheap.size -= freed_up;
			}
		}
	}
	unlock();
}

void *cmpct_alloc(size_t size)
{
	if (size == 0u) return NULL;

	size += sizeof(header_t);

	if (size > (1u << HEAP_ALLOC_VIRTUAL_BITS)) return large_alloc(size);

	size_t rounded_up;
	int start_bucket = size_to_index_allocating(size, &rounded_up);

	lock();
	int bucket = find_nonempty_bucket(start_bucket);
	if (bucket == -1) {
		// Grow heap by at least 12% if we can.
		size_t growby = MIN(1u << HEAP_ALLOC_VIRTUAL_BITS,
							MAX(theheap.size >> 3,
								MAX(HEAP_GROW_SIZE, rounded_up)));
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
	if (left_over >= sizeof(free_t) &&
		left_over > (size >> 6)) {
		header_t *right = right_header(&head->header);
		unlink_free(head, bucket);
		void *free = (char *)head + rounded_up;
		create_free_area(free, head, left_over, NULL);
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
		size + alignment + sizeof(free_t) + sizeof(header_t);
	char *unaligned = (char *)cmpct_alloc(padded_size);
	lock();
	size_t mask = alignment - 1;
	uintptr_t payload_int = (uintptr_t)unaligned + sizeof(free_t) +
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

static void add_to_heap(void *new_area, size_t size, free_t **bucket)
{
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
static ssize_t heap_grow(size_t size, free_t **bucket)
{
	DEBUG_ASSERT(size == ROUNDUP(size, 8));
	// The new free list entry will have a header on each side (the
	// sentinels) so we need to grow the gross heap size by this much more.
	size += 2 * sizeof(header_t);
	size = ROUNDUP(size, PAGE_SIZE);
	void *ptr = page_alloc(size >> PAGE_SIZE_SHIFT);
	theheap.size += size;
	if (ptr == NULL) return -1;
	LTRACEF("growing heap by 0x%zx bytes, new ptr %p\n", size, ptr);
	add_to_heap(ptr, size, bucket);
	return size;
}

void cmpct_init(void)
{
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

/* vim: set ts=4 sw=4 noexpandtab: */
