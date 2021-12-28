/*
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <lk/list.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

/* PAGE_SIZE minus 16 bytes of metadata in pktbuf_buf */
#ifndef PKTBUF_POOL_SIZE
#define PKTBUF_POOL_SIZE 256
#endif

#ifndef PKTBUF_SIZE
#define PKTBUF_SIZE     1536
#endif

/* How much space pktbuf_alloc should save for IP headers in the front of the buffer */
#define PKTBUF_MAX_HDR  64
/* The remaining space in the buffer */
#define PKTBUF_MAX_DATA (PKTBUF_SIZE - PKTBUF_MAX_HDR)

typedef void (*pktbuf_free_callback)(void *buf, void *arg);
typedef struct pktbuf {
    u8 *data;
    u32 blen;
    u32 dlen;
    paddr_t phys_base;
    struct list_node list;
    u32 flags;
    pktbuf_free_callback cb;
    void *cb_args;
    u8 *buffer;
} pktbuf_t;

typedef struct pktbuf_pool_object {
    union {
        pktbuf_t p;
        uint8_t b[PKTBUF_SIZE];
    };
} pktbuf_pool_object_t;

#define PKTBUF_FLAG_CKSUM_IP_GOOD  (1<<0)
#define PKTBUF_FLAG_CKSUM_TCP_GOOD (1<<1)
#define PKTBUF_FLAG_CKSUM_UDP_GOOD (1<<2)
#define PKTBUF_FLAG_EOF            (1<<3)
#define PKTBUF_FLAG_CACHED         (1<<4)

/* Return the physical address offset of data in the packet */
static inline u32 pktbuf_data_phys(pktbuf_t *p) {
    return p->phys_base + (p->data - p->buffer);
}

// number of bytes available for _prepend
static inline u32 pktbuf_avail_head(pktbuf_t *p) {
    return p->data - p->buffer;
}

// number of bytes available for _append or _append_data
static inline u32 pktbuf_avail_tail(pktbuf_t *p) {
    return p->blen - (p->data - p->buffer) - p->dlen;
}

// allocate packet buffer from buffer pool
pktbuf_t *pktbuf_alloc(void);
pktbuf_t *pktbuf_alloc_empty(void);

/* Add a buffer to an existing packet buffer */
void pktbuf_add_buffer(pktbuf_t *p, u8 *buf, u32 len, uint32_t header_sz,
                       uint32_t flags, pktbuf_free_callback cb, void *cb_args);

// return packet buffer to buffer pool
// returns number of threads woken up
int pktbuf_free(pktbuf_t *p, bool reschedule);

// extend buffer by sz bytes, copied from data
void pktbuf_append_data(pktbuf_t *p, const void *data, size_t sz);

// extend buffer by sz bytes, returning a pointer to the
// start of the newly appended region
void *pktbuf_append(pktbuf_t *p, size_t sz);

// grow the front of the buffer and return a pointer
// to the new start of packet
void *pktbuf_prepend(pktbuf_t *p, size_t sz);

// shrink the buffer by discarding the first sz bytes
// returning a pointer to the discarded bytes (which
// will remain untouched until the next _prepend),
// or NULL if there were not enough bytes to consume
void *pktbuf_consume(pktbuf_t *p, size_t sz);

// remove sz bytes from the end of the pktbuf
void pktbuf_consume_tail(pktbuf_t *p, size_t sz);

// reset the header size to header_sz into the underlying buffer.
// p->dlen is also set to 0.
// pktbuf must have a buffer with a valid blen and header_sz must
// be within the buffer.
void pktbuf_reset(pktbuf_t *p, uint32_t header_sz);

// create a new packet buffer from raw memory and add
// it to the free pool
void pktbuf_create(void *ptr, size_t size);

// Create buffers for pktbufs of size PKTBUF_BUF_SIZE out of size
void pktbuf_create_bufs(void *ptr, size_t size);

void pktbuf_dump(pktbuf_t *p);

__END_CDECLS
