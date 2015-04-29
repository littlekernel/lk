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
#include <lib/pktbuf.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#define LOCAL_TRACE 0

static struct list_node pb_freelist = LIST_INITIAL_VALUE(pb_freelist);
static struct list_node pb_buflist = LIST_INITIAL_VALUE(pb_buflist);
static semaphore_t pb_sem = SEMAPHORE_INITIAL_VALUE(pb_sem, 0);


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
	pktbuf_t *p = NULL;
	pktbuf_buf_t *b = NULL;

	/* Check for buffers first to reduce the complexity of cases where we have a pktbuf
	 * pointer but no buffer and would otherwise have to do sem / list bookkeeping on
	 * cleanup */
	sem_wait(&pb_sem);
	enter_critical_section();
	b = pktbuf_get_buf();
	if (b) {
		p = list_remove_head_type(&pb_freelist, pktbuf_t, list);
	}
	exit_critical_section();

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
	/* TODO: This will be moved to the stack soon */
	p->eof = true;
	p->phys_base = b->phys_addr;

	return p;
}

pktbuf_t *pktbuf_alloc_empty(void *buf, size_t dlen) {
	pktbuf_t *p;

	sem_wait(&pb_sem);
	enter_critical_section();
	p = list_remove_head_type(&pb_freelist, pktbuf_t, list);
	exit_critical_section();

	if (!p) {
		return NULL;
	}

	p->buffer = buf;
	p->data = p->buffer;
	p->dlen = dlen;
	p->managed = false;
	return p;
}

int pktbuf_free(pktbuf_t *p, bool reschedule) {
	enter_critical_section();
	list_add_tail(&pb_freelist, &(p->list));
	if (p->managed && p->buffer) {
		pktbuf_buf_t *pkt = (pktbuf_buf_t *)p->buffer;
		list_add_tail(&pb_buflist, &pkt->list);
	}
	p->buffer = NULL;
	p->data = NULL;
	p->eof = false;
	p->managed = false;
	exit_critical_section();

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
// vim: set noexpandtab:
