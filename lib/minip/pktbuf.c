/*
 * Copyright (c) 2014 Brian Swetland
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
#include <printf.h>
#include <string.h>

#include <kernel/thread.h>
#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <lib/pktbuf.h>
#include <lk/init.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#define LOCAL_TRACE 0

/* default number of packet buffers */
#ifndef PKTBUF_COUNT
#define PKTBUF_COUNT 128
#endif

static struct list_node pb_freelist = LIST_INITIAL_VALUE(pb_freelist);
static struct list_node pb_buflist = LIST_INITIAL_VALUE(pb_buflist);
static semaphore_t pb_sem = SEMAPHORE_INITIAL_VALUE(pb_sem, -1);
static spin_lock_t lock;


static unsigned int cur_id = 0;

void pktbuf_create(void *ptr, size_t size) {
	pktbuf_t *p = ptr;

	p->magic = PKTBUF_HDR_MAGIC;
	p->phys_base = 0;
	p->id = cur_id++;
	list_add_tail(&pb_freelist, &(p->list));
	sem_post(&pb_sem, false);
}

/* Carve buffers for pktbufs of size PKTBUF_BUF_SIZE from the memory pointed at by ptr */
void pktbuf_create_bufs(void *ptr, size_t size) {
	uintptr_t phys_addr;

#if WITH_KERNEL_VM
	if (arch_mmu_query((uintptr_t) ptr, &phys_addr, NULL) < 0) {
		printf("Failed to get physical address for pktbuf slab, using virtual\n");
	}
#else
	phys_addr = ptr;
#endif

	while (size > sizeof(pktbuf_buf_t)) {
		pktbuf_buf_t *pkt = ptr;

		pkt->magic = PKTBUF_BUF_MAGIC;
		pkt->phys_addr = phys_addr;
		list_add_tail(&pb_buflist, &pkt->list);


		ptr += sizeof(pktbuf_buf_t);
		phys_addr += sizeof(pktbuf_buf_t);
		size -= sizeof(pktbuf_buf_t);
	}
}

static inline pktbuf_buf_t *pktbuf_get_buf(void) {
	return list_remove_head_type(&pb_buflist, pktbuf_buf_t, list);
}

pktbuf_t *pktbuf_alloc(void) {
	spin_lock_saved_state_t state;

	pktbuf_t *p = NULL;
	pktbuf_buf_t *b = NULL;

	/* Check for buffers first to reduce the complexity of cases where we have a pktbuf
	 * pointer but no buffer and would otherwise have to do sem / list bookkeeping on
	 * cleanup */
	sem_wait(&pb_sem);
	spin_lock_irqsave(&lock, state);
	b = pktbuf_get_buf();
	if (b) {
		p = list_remove_head_type(&pb_freelist, pktbuf_t, list);
	}
	spin_unlock_irqrestore(&lock, state);

	if (b->magic != PKTBUF_BUF_MAGIC) {
		panic("pktbuf id %u has corrupted buffer magic value\n"
				"buf_addr %p magic: 0x%08X (expected 0x%08X), phys_addr: %p\n",
				p->id, b, b->magic, PKTBUF_BUF_MAGIC, (void *) b->phys_addr);
	}

	if (!p) {
		return NULL;
	}

	p->buffer = (uint8_t *) b;
	p->data = p->buffer + PKTBUF_MAX_HDR;
	p->dlen = 0;
	p->managed = true;
	p->flags = 0;
	/* TODO: This will be moved to the stack soon */
	p->eof = true;
	p->phys_base = b->phys_addr;

	return p;
}

pktbuf_t *pktbuf_alloc_empty(void *buf, size_t dlen) {
	spin_lock_saved_state_t state;
	pktbuf_t *p;

	sem_wait(&pb_sem);
	spin_lock_irqsave(&lock, state);
	p = list_remove_head_type(&pb_freelist, pktbuf_t, list);
	spin_unlock_irqrestore(&lock, state);

	if (!p) {
		return NULL;
	}

	p->buffer = buf;
	p->data = p->buffer;
	p->dlen = dlen;
	p->managed = false;
	p->flags = 0;
	return p;
}

int pktbuf_free(pktbuf_t *p, bool reschedule) {
	spin_lock_saved_state_t state;
	spin_lock_irqsave(&lock, state);
	list_add_tail(&pb_freelist, &(p->list));
	if (p->managed && p->buffer) {
		pktbuf_buf_t *pkt = (pktbuf_buf_t *)p->buffer;
		list_add_tail(&pb_buflist, &pkt->list);
	}
	p->buffer = NULL;
	p->data = NULL;
	p->eof = false;
	p->managed = false;
	p->flags = 0;
	spin_unlock_irqrestore(&lock, state);

	return sem_post(&pb_sem, reschedule);
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
	printf("pktbuf id %u, data %p, buffer %p, dlen %u, data offset %lu, phys_base %p, managed %u\n",
			p->id, p->data, p->buffer, p->dlen, (uintptr_t) p->data - (uintptr_t) p->buffer,
			(void *)p->phys_base, p->managed);
}

static void pktbuf_init(uint level)
{
	void *buf;

#if LK_DEBUGLEVEL > 0
	printf("pktbuf: creating %u packet buffers (%zu/%zu bytes header/buffers)\n",
		PKTBUF_COUNT, PKTBUF_COUNT * sizeof(pktbuf_t), PKTBUF_COUNT * sizeof(pktbuf_buf_t));
#endif

#if WITH_KERNEL_VM
	if (vmm_alloc_contiguous(vmm_get_kernel_aspace(), "pktbuf_headers",
			PKTBUF_COUNT * sizeof(pktbuf_t), &buf, 0, 0, ARCH_MMU_FLAG_CACHED) < 0) {
		printf("Failed to initialize pktbuf hdr slab\n");
		return;
	}
#else
	buf = malloc(PKTBUF_COUNT * sizeof(pktbuf_t));
#endif

	for (size_t i = 0; i < PKTBUF_COUNT; i++) {
		pktbuf_create((char *)buf + i * sizeof(pktbuf_t), sizeof(pktbuf_t));
	}

#if WITH_KERNEL_VM
	if (vmm_alloc_contiguous(vmm_get_kernel_aspace(), "pktbuf_buffers",
			PKTBUF_COUNT * sizeof(pktbuf_buf_t), &buf, 0, 0, ARCH_MMU_FLAG_CACHED) < 0) {
		printf("Failed to initialize pktbuf vm slab\n");
		return;
	}
#else
	buf = memalign(CACHE_LINE, PKTBUF_COUNT * sizeof(pktbuf_buf_t));
#endif

	pktbuf_create_bufs(buf, PKTBUF_COUNT * sizeof(pktbuf_buf_t));
}

LK_INIT_HOOK(pktbuf, pktbuf_init, LK_INIT_LEVEL_THREADING);

// vim: set noexpandtab:
