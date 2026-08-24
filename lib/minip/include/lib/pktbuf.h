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

struct netif;

/* Number of packet data buffers in the pool. Each is PKTBUF_SIZE bytes.
 * pktbuf headers are allocated from a separate, smaller pool.
 */
#ifndef PKTBUF_POOL_SIZE
#define PKTBUF_POOL_SIZE 256
#endif

/* Reserve this many pool data buffers for non-RX-ring traffic by default,
 * keeping space for TX/control traffic such as ARP/DHCP.
 */
#ifndef PKTBUF_ETH_RX_POOL_RESERVE
#define PKTBUF_ETH_RX_POOL_RESERVE 64
#endif

/* Extra pktbuf headers on top of one per data buffer, for headers that wrap
 * externally owned (driver) buffers via pktbuf_add_buffer().
 */
#ifndef PKTBUF_EXTRA_HEADERS
#define PKTBUF_EXTRA_HEADERS 64
#endif

#ifndef PKTBUF_SIZE
#define PKTBUF_SIZE     1536
#endif

/* Ceiling for on-demand pool growth, in packets. The pool starts at
 * PKTBUF_POOL_SIZE and grows in chunks as blocking allocations find it
 * exhausted, up to this cap. Fixed at PKTBUF_POOL_SIZE on LK_EMBEDDED
 * targets, where the growth path compiles out entirely.
 */
#ifndef PKTBUF_POOL_MAX
#if LK_EMBEDDED
#define PKTBUF_POOL_MAX PKTBUF_POOL_SIZE
#else
#define PKTBUF_POOL_MAX (4 * PKTBUF_POOL_SIZE)
#endif
#endif

/* How much space pktbuf_alloc should save for IP headers in the front of the buffer */
#define PKTBUF_MAX_HDR  64
/* The remaining space in the buffer */
#define PKTBUF_MAX_DATA (PKTBUF_SIZE - PKTBUF_MAX_HDR)

typedef void (*pktbuf_free_callback)(void *buf, void *arg, bool reschedule);
typedef struct pktbuf {
    u8 *data;
    u32 blen;
    u32 dlen;
    paddr_t phys_base;
    struct list_node list;
    u32 flags;
    int ref;                // reference count, adjusted atomically
    u32 seq;                // per-layer scratch (e.g. TCP sequence number)
    struct netif *netif;    // receiving interface, set by the stack input queue
    pktbuf_free_callback cb;
    void *cb_args;
    u8 *buffer;
} pktbuf_t;

#define PKTBUF_FLAG_CKSUM_IP_GOOD  (1<<0)
#define PKTBUF_FLAG_CKSUM_TCP_GOOD (1<<1)
#define PKTBUF_FLAG_CKSUM_UDP_GOOD (1<<2)
#define PKTBUF_FLAG_EOF            (1<<3)
#define PKTBUF_FLAG_CACHED         (1<<4)

/* Return the physical address offset of data in the packet */
static inline paddr_t pktbuf_data_phys(pktbuf_t *p) {
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

// allocate a packet buffer from the pool, with PKTBUF_MAX_HDR bytes of
// headroom reserved for prepending headers.
// non-blocking and callable from interrupt context; returns NULL if the
// pool is exhausted.
pktbuf_t *pktbuf_alloc(void);

// as pktbuf_alloc, but block up to timeout for a buffer to become
// available. thread context only.
pktbuf_t *pktbuf_alloc_timeout(lk_time_t timeout);

// as pktbuf_alloc, but set up for driver RX DMA: data starts at the
// beginning of the buffer with no headroom reserved.
pktbuf_t *pktbuf_alloc_rx(void);

// allocate a bare pktbuf header with no data buffer, for wrapping an
// externally owned buffer via pktbuf_add_buffer().
pktbuf_t *pktbuf_alloc_empty(void);

/* Add a buffer to an existing packet buffer */
void pktbuf_add_buffer(pktbuf_t *p, u8 *buf, u32 len, uint32_t header_sz,
                       uint32_t flags, pktbuf_free_callback cb, void *cb_args);

// take an additional reference on the packet buffer
void pktbuf_ref(pktbuf_t *p);

// drop a reference; when the last reference is dropped the buffer free
// callback runs and the header returns to the pool
void pktbuf_free(pktbuf_t *p, bool reschedule);

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

void pktbuf_dump(pktbuf_t *p);

// pool statistics snapshot
typedef struct pktbuf_stats {
    size_t bufs_total;    // data buffers created so far
    size_t bufs_free;     // currently in the pool
    size_t bufs_free_low; // low water mark of bufs_free
    size_t bufs_max;      // growth ceiling (PKTBUF_POOL_MAX)
    size_t hdrs_total;
    size_t hdrs_free;
} pktbuf_stats_t;

void pktbuf_get_stats(pktbuf_stats_t *stats);

// Return a safe ethernet RX preallocation depth based on pool capacity,
// leaving PKTBUF_ETH_RX_POOL_RESERVE data buffers for other traffic.
// The returned value is clamped to requested_depth.
size_t pktbuf_recommended_eth_rx_depth(size_t requested_depth);

__END_CDECLS
