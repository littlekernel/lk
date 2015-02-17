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

static struct list_node pb_freelist = LIST_INITIAL_VALUE(pb_freelist);
static semaphore_t pb_sem = SEMAPHORE_INITIAL_VALUE(pb_sem, 0);
static spin_lock_t lock;

void pktbuf_create(void *ptr, u32 phys, size_t size) {
	pktbuf_t *p = ptr;
	if (size != PKTBUF_SIZE) {
		panic("pktbuf_create: invalid size %d\n", size);
	}

	p->phys_base = phys + __offsetof(pktbuf_t, buffer);
	p->rsv0 = 0;
	p->rsv1 = 0;
	p->rsv2 = 0;
	list_add_tail(&pb_freelist, &(p->list));
	sem_post(&pb_sem, false);
}

pktbuf_t *pktbuf_alloc(void) {
	spin_lock_saved_state_t state;

	pktbuf_t *p;

	sem_wait(&pb_sem);
	spin_lock_irqsave(&lock, state);
	spin_lock_irqsave(&lock, state);
	p = list_remove_head_type(&pb_freelist, pktbuf_t, list);
	spin_unlock_irqrestore(&lock, state);
	if (!p)
		return NULL;

	p->data = p->buffer + PKTBUF_MAX_HDR;
	p->dlen = 0;
	return p;
}

void pktbuf_free(pktbuf_t *p) {
	spin_lock_saved_state_t state;
	spin_lock_irqsave(&lock, state);
	list_add_tail(&pb_freelist, &(p->list));
	spin_unlock_irqrestore(&lock, state);

	sem_post(&pb_sem, true);
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

// vim: set noexpandtab:
