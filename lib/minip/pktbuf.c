/*
 * Copyright (c) 2014 Brian Swetland
 * Copyright (c) 2014-2015 Christopher Anderson
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

#include <assert.h>
#include <debug.h>
#include <trace.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>

#include <kernel/thread.h>
#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <lib/pktbuf.h>
#include <lib/pool.h>
#include <lk/init.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#define LOCAL_TRACE 0

static pool_t pktbuf_pool;
static semaphore_t pktbuf_sem;
static spin_lock_t lock;


/* Take an object from the pool of pktbuf objects to act as a header or buffer.  */
static void *get_pool_object(void) {
	pool_t *entry;
	spin_lock_saved_state_t state;

	sem_wait(&pktbuf_sem);
	spin_lock_irqsave(&lock, state);
	entry = pool_alloc(&pktbuf_pool);
	spin_unlock_irqrestore(&lock, state);

	return (pktbuf_pool_object_t *) entry;

}

/* Return an object to thje pktbuf object pool. */
static void free_pool_object(pktbuf_pool_object_t *entry, bool reschedule) {
	DEBUG_ASSERT(entry);
	spin_lock_saved_state_t state;

	spin_lock_irqsave(&lock, state);
	pool_free(&pktbuf_pool, entry);
	spin_unlock_irqrestore(&lock, state);
	sem_post(&pktbuf_sem, reschedule);
}

/* Callback used internally to place a pktbuf_pool_object back in the pool after
 * it was used as a buffer for another pktbuf
 */
static void free_pktbuf_buf_cb(void *buf, void *arg) {
	memset(buf, 0, sizeof(pktbuf_t));
	free_pool_object((pktbuf_pool_object_t *)buf, true);
}

/* Add a buffer to a pktbuf. Header space for prepending data is adjusted based on
 * header_sz. cb is called when the pktbuf is freed / released by the driver level
 * and should handle proper management / freeing of the buffer pointed to by the iovec.
 *
 * It's important to note that there is a flag to note that the buffer is cached and should
 * be properly handled via the appropriate driver when it's time to deal with buffer
 * descriptiors.
 */
void pktbuf_add_buffer(pktbuf_t *p, u8 *buf, u32 len, uint32_t header_sz, uint32_t flags,
					   pktbuf_free_callback cb, void *cb_args) {
	DEBUG_ASSERT(p);
	DEBUG_ASSERT(buf);
	DEBUG_ASSERT(header_sz < len);

	p->buffer = buf;
	p->blen = len;
	p->data = p->buffer + header_sz;
	p->dlen = 0;
	p->flags = PKTBUF_FLAG_EOF | flags;
	p->cb = cb;
	p->cb_args = cb_args;

	/* If we're using a VM then this may be a virtual address, look up to see
	 * if there is an associated physical address we can store. If not, then
	 * stick with the address as presented to us.
	 */
#if WITH_KERNEL_VM
	p->phys_base = kvaddr_to_paddr(buf) | (uintptr_t) buf % PAGE_SIZE;
#else
	p->phys_base = buf;
#endif
}

pktbuf_t *pktbuf_alloc(void) {
	pktbuf_t *p = NULL;
	void *buf = NULL;

	p = get_pool_object();
	if (!p) {
		return NULL;
	}

	buf = get_pool_object();
	if (!buf) {
		free_pool_object((pktbuf_pool_object_t *)p, false);
		return NULL;
	}

	pktbuf_add_buffer(p, buf, PKTBUF_SIZE, PKTBUF_MAX_HDR, 0, free_pktbuf_buf_cb, NULL);
	return p;
}

pktbuf_t *pktbuf_alloc_empty(void) {
	pktbuf_t *p = (pktbuf_t *) get_pool_object();

	p->flags = PKTBUF_FLAG_EOF;
	return p;
}

int pktbuf_free(pktbuf_t *p, bool reschedule) {
	DEBUG_ASSERT(p);

	if (p->cb) {
		p->cb(p->buffer, p->cb_args);
	}
	free_pool_object((pktbuf_pool_object_t *)p, false);

	return 1;
}

void pktbuf_append_data(pktbuf_t *p, const void *data, size_t sz) {
	if (pktbuf_avail_tail(p) < sz) {
		panic("pktbuf_append_data: overflow");
	}

	memcpy(p->data + p->dlen, data, sz);
	p->dlen += sz;
}

void *pktbuf_append(pktbuf_t *p, size_t sz) {
	if (pktbuf_avail_tail(p) < sz) {
		panic("pktbuf_append: overflow");
	}

	void *data = p->data + p->dlen;
	p->dlen += sz;

	return data;
}

void *pktbuf_prepend(pktbuf_t *p, size_t sz) {
	if (pktbuf_avail_head(p) < sz) {
		panic("pktbuf_prepend: not enough space");
	}

	p->dlen += sz;
	p->data -= sz;

	return p->data;
}

void *pktbuf_consume(pktbuf_t *p, size_t sz) {
	void *data = p->data;

	if (sz > p->dlen) {
		return NULL;
	}

	p->data += sz;
	p->dlen -= sz;

	return data;
}

void pktbuf_consume_tail(pktbuf_t *p, size_t sz) {
	if (sz > p->dlen) {
		p->dlen = 0;
		return;
	}

	p->dlen -= sz;
}

void pktbuf_dump(pktbuf_t *p) {
	printf("pktbuf data %p, buffer %p, dlen %u, data offset %lu, phys_base %p\n",
			p->data, p->buffer, p->dlen, (uintptr_t) p->data - (uintptr_t) p->buffer,
			(void *)p->phys_base);
}

static void pktbuf_init(uint level)
{
	void *slab;

#if LK_DEBUGLEVEL > 0
	printf("pktbuf: creating %u pktbuf entries of size %zu (total %zu)\n",
		PKTBUF_POOL_SIZE, sizeof(struct pktbuf_pool_object),
		PKTBUF_POOL_SIZE * sizeof(struct pktbuf_pool_object));
#endif

#if WITH_KERNEL_VM
	if (vmm_alloc_contiguous(vmm_get_kernel_aspace(), "pktbuf",
			PKTBUF_POOL_SIZE * sizeof(struct pktbuf_pool_object), 
			&slab, 0, 0, ARCH_MMU_FLAG_CACHED) < 0) {
		printf("Failed to initialize pktbuf hdr slab\n");
		return;
	}
#else
	slab = memalign(CACHE_LINE, PKTBUF_POOL_SIZE * sizeof(pktbuf_pool_object_t));
#endif

	pool_init(&pktbuf_pool, sizeof(struct pktbuf_pool_object), CACHE_LINE, PKTBUF_POOL_SIZE, slab);
	sem_init(&pktbuf_sem, PKTBUF_POOL_SIZE);
}

LK_INIT_HOOK(pktbuf, pktbuf_init, LK_INIT_LEVEL_THREADING);

// vim: set noexpandtab:
