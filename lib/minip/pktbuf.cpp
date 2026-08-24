/*
 * Copyright (c) 2014 Brian Swetland
 * Copyright (c) 2014-2015 Christopher Anderson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <assert.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <malloc.h>
#include <printf.h>
#include <string.h>

#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lib/pktbuf.h>
#include <lib/pool.h>
#include <lk/init.h>
#include <lk/pow2.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#define LOCAL_TRACE 0

/* Total number of pktbuf headers: one per pool data buffer plus a reserve for
 * headers wrapping externally owned (driver) buffers.
 */
#define PKTBUF_TOTAL_HEADERS (PKTBUF_POOL_SIZE + PKTBUF_EXTRA_HEADERS)

static pool_t pktbuf_buf_pool;  // PKTBUF_SIZE sized data buffers
static pool_t pktbuf_hdr_pool;  // pktbuf_t headers
static semaphore_t pktbuf_sem;  // counts available data buffers
static spin_lock_t lock;        // protects both pools

size_t pktbuf_recommended_eth_rx_depth(size_t requested_depth) {
    if (requested_depth == 0) {
        return 0;
    }

    size_t available = PKTBUF_POOL_SIZE;
    if (available > PKTBUF_ETH_RX_POOL_RESERVE) {
        available -= PKTBUF_ETH_RX_POOL_RESERVE;
    } else {
        available = 0;
    }

    size_t max_depth = available;
    if (max_depth == 0) {
        max_depth = 1;
    }

    size_t depth = (requested_depth < max_depth) ? requested_depth : max_depth;
    size_t pow2_depth = valpow2(log2_uint((uint)depth));

    return (pow2_depth != 0) ? pow2_depth : 1;
}

/* Take a header from the header pool. */
static pktbuf_t *alloc_header(void) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
    void *h = pool_alloc(&pktbuf_hdr_pool);
    spin_unlock_irqrestore(&lock, state);

    return (pktbuf_t *)h;
}

/* Return a header to the header pool. */
static void free_header(pktbuf_t *p) {
    DEBUG_ASSERT(p);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
    pool_free(&pktbuf_hdr_pool, p);
    spin_unlock_irqrestore(&lock, state);
}

/* Take a data buffer from the buffer pool. The pool semaphore must have been
 * acquired beforehand.
 */
static void *alloc_buffer(void) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
    void *b = pool_alloc(&pktbuf_buf_pool);
    spin_unlock_irqrestore(&lock, state);

    return b;
}

/* Return a data buffer to the buffer pool and post the pool semaphore. */
static void free_buffer(void *buf, bool reschedule) {
    DEBUG_ASSERT(buf);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
    pool_free(&pktbuf_buf_pool, buf);
    spin_unlock_irqrestore(&lock, state);
    sem_post(&pktbuf_sem, reschedule);
}

/* Callback used internally to place a pool data buffer back in the pool after
 * the pktbuf wrapping it is freed.
 */
static void free_pktbuf_buf_cb(void *buf, void *arg, bool reschedule) {
    free_buffer(buf, reschedule);
}

/* Add a buffer to a pktbuf. Header space for prepending data is adjusted based on
 * header_sz. cb is called when the pktbuf is freed / released by the driver level
 * and should handle proper management / freeing of the buffer pointed to by the iovec.
 *
 * It's important to note that there is a flag to note that the buffer is cached and should
 * be properly handled via the appropriate driver when it's time to deal with buffer
 * descriptors.
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
    p->phys_base = vaddr_to_paddr(buf) | (uintptr_t)buf % PAGE_SIZE;
#else
    p->phys_base = (uintptr_t)buf;
#endif
}

pktbuf_t *pktbuf_alloc(void) {
    sem_wait(&pktbuf_sem);
    void *buf = alloc_buffer();
    DEBUG_ASSERT(buf); // the semaphore guarantees a buffer is available

    pktbuf_t *p = alloc_header();
    if (!p) {
        free_buffer(buf, false);
        return NULL;
    }

    memset(p, 0, sizeof(pktbuf_t));
    pktbuf_add_buffer(p, (u8 *)buf, PKTBUF_SIZE, PKTBUF_MAX_HDR, 0, free_pktbuf_buf_cb, NULL);
    return p;
}

pktbuf_t *pktbuf_alloc_empty(void) {
    pktbuf_t *p = alloc_header();
    if (!p) {
        return NULL;
    }

    memset(p, 0, sizeof(pktbuf_t));
    p->flags = PKTBUF_FLAG_EOF;
    return p;
}

void pktbuf_reset(pktbuf_t *p, uint32_t header_sz) {
    DEBUG_ASSERT(p);
    DEBUG_ASSERT(p->buffer);
    DEBUG_ASSERT(header_sz < p->blen);

    p->data = p->buffer + header_sz;
    p->dlen = 0;
}

int pktbuf_free(pktbuf_t *p, bool reschedule) {
    DEBUG_ASSERT(p);

    if (p->cb) {
        p->cb(p->buffer, p->cb_args, reschedule);
    }
    free_header(p);

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
    printf("pktbuf data %p, buffer %p, dlen %u, data offset %lu, phys_base %p\n", p->data,
           p->buffer, p->dlen, (uintptr_t)p->data - (uintptr_t)p->buffer, (void *)p->phys_base);
}

static void pktbuf_init(uint level) {
    const size_t buf_storage_size = pool_storage_size(PKTBUF_SIZE, CACHE_LINE, PKTBUF_POOL_SIZE);
    const size_t hdr_storage_size =
        pool_storage_size(sizeof(pktbuf_t), CACHE_LINE, PKTBUF_TOTAL_HEADERS);

#if LK_DEBUGLEVEL > 0
    printf("pktbuf: creating %u pktbufs of size %u (%zu bytes of buffers, %zu bytes of headers)\n",
           PKTBUF_POOL_SIZE, PKTBUF_SIZE, buf_storage_size, hdr_storage_size);
#endif

    void *buf_slab;
#if WITH_KERNEL_VM
    if (vmm_alloc_contiguous(vmm_get_kernel_aspace(), "pktbuf", buf_storage_size, &buf_slab, 0, 0,
                             ARCH_MMU_FLAG_CACHED) < 0) {
        printf("Failed to allocate pktbuf buffer slab\n");
        return;
    }
#else
    buf_slab = memalign(CACHE_LINE, buf_storage_size);
    if (!buf_slab) {
        printf("Failed to allocate pktbuf buffer slab\n");
        return;
    }
#endif

    void *hdr_slab = memalign(CACHE_LINE, hdr_storage_size);
    if (!hdr_slab) {
        printf("Failed to allocate pktbuf header slab\n");
        return;
    }

    pool_init(&pktbuf_buf_pool, PKTBUF_SIZE, CACHE_LINE, PKTBUF_POOL_SIZE, buf_slab);
    pool_init(&pktbuf_hdr_pool, sizeof(pktbuf_t), CACHE_LINE, PKTBUF_TOTAL_HEADERS, hdr_slab);
    sem_init(&pktbuf_sem, PKTBUF_POOL_SIZE);
}

LK_INIT_HOOK(pktbuf, pktbuf_init, LK_INIT_LEVEL_THREADING);
