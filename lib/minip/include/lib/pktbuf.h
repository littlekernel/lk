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

#ifndef _NET_PKTBUF_H_
#define _NET_PKTBUF_H_

#include <sys/types.h>
#include <list.h>

/* PAGE_SIZE minus 16 bytes of metadata in pktbuf_buf */
#ifndef PKTBUF_POOL_SIZE
#define PKTBUF_POOL_SIZE 256
#endif

#ifndef PKTBUF_SIZE
#define	PKTBUF_SIZE		1536
#endif

/* How much space pktbuf_alloc should save for IP headers in the front of the buffer */
#define PKTBUF_MAX_HDR  64
/* The remaining space in the buffer */
#define PKTBUF_MAX_DATA (PKTBUF_SIZE - PKTBUF_MAX_HDR)

typedef struct pktbuf {
	u8 *data;
	u32 dlen;
	u32 phys_base;
	struct list_node list;
	u32 flags;
	bool managed;
	bool eof;
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
	return PKTBUF_SIZE - (p->data - p->buffer) - p->dlen;
}

// allocate packet buffer from buffer pool
pktbuf_t *pktbuf_alloc(void);
pktbuf_t *pktbuf_alloc_empty(void);

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

// create a new packet buffer from raw memory and add
// it to the free pool
void pktbuf_create(void *ptr, size_t size);

// Create buffers for pktbufs of size PKTBUF_BUF_SIZE out of size
void pktbuf_create_bufs(void *ptr, size_t size);

void pktbuf_dump(pktbuf_t *p);
#endif

// vim: set noexpandtab:
