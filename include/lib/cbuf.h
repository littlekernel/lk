/*
 * Copyright (c) 2009-2013 Travis Geiselbrecht
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
#ifndef __LIB_CBUF_H
#define __LIB_CBUF_H

#include <sys/types.h>
#include <kernel/event.h>
#include <kernel/spinlock.h>
#include <iovec.h>

typedef struct cbuf {
	uint head;
	uint tail;
	uint len_pow2;
	char *buf;
	event_t event;
	spin_lock_t lock;
} cbuf_t;

/**
 * cbuf_initialize
 *
 * Initialize a cbuf structure, mallocing the underlying data buffer in the
 * process.  Make sure that the buffer has enough space for at least len bytes.
 *
 * @param[in] cbuf A pointer to the cbuf structure to allocate.
 * @param[in] len The minimum number of bytes for the underlying data buffer.
 */
void cbuf_initialize(cbuf_t *cbuf, size_t len);

/**
 * cbuf_initalize_etc
 *
 * Initialize a cbuf structure using the supplied buffer for internal storage.
 *
 * @param[in] cbuf A pointer to the cbuf structure to allocate.
 * @param[in] len The size of the supplied buffer, in bytes.
 * @param[in] buf A pointer to the memory to be used for internal storage.
 */
void cbuf_initialize_etc(cbuf_t *cbuf, size_t len, void *buf);

/**
 * cbuf_read
 *
 * Read up to buflen bytes in to the supplied buffer.
 *
 * @param[in] cbuf The cbuf instance to read from.
 * @param[in] buf A pointer to a buffer to read data into.  If NULL, cbuf_read
 * will skip up to the next buflen bytes from the cbuf.
 * @param[in] buflen The maximum number of bytes to read from the cbuf.
 * @param[in] block When true, will cause the caller to block until there is at
 * least one byte available to read from the cbuf.
 *
 * @return The actual number of bytes which were read (or skipped).
 */
size_t cbuf_read(cbuf_t *cbuf, void *buf, size_t buflen, bool block);

/**
 * cbuf_peek
 *
 * Peek at the data available for read in the cbuf right now.  Does not actually
 * consume the data, it just fills out a pair of iovec structures describing the
 * (up to) two contiguous regions currently available for read.
 *
 * @param[in] cbuf The cbuf instance to write to.
 * @param[out] A pointer to two iovec structures to hold the contiguous regions
 * for read.  NOTE: regions must point to a chunk of memory which is at least
 * sizeof(iovec_t) * 2 bytes long.
 *
 * @return The number of bytes which were written (or skipped).
 */
size_t cbuf_peek(cbuf_t *cbuf, iovec_t* regions);

/**
 * cbuf_write
 *
 * Write up to len bytes from the the supplied buffer into the cbuf.
 *
 * @param[in] cbuf The cbuf instance to write to.
 * @param[in] buf A pointer to a buffer to read data from.  If NULL, cbuf_write
 * will skip up to the next len bytes in the cbuf, filling with zeros instead of
 * supplied data.
 * @param[in] len The maximum number of bytes to write to the cbuf.
 * @param[in] canreschedule Rescheduling policy passed through to the internal
 * event when signalling the event to indicate that there is now data in the
 * buffer to be read.
 *
 * @return The number of bytes which were written (or skipped).
 */
size_t cbuf_write(cbuf_t *cbuf, const void *buf, size_t len, bool canreschedule);

/**
 * cbuf_space_avail
 *
 * @param[in] cbuf The cbuf instance to query
 *
 * @return The number of free space available in the cbuf (IOW - the maximum
 * number of bytes which can currently be written)
 */
size_t cbuf_space_avail(cbuf_t *cbuf);

/**
 * cbuf_space_used
 *
 * @param[in] cbuf The cbuf instance to query
 *
 * @return The number of used bytes in the cbuf (IOW - the maximum number of
 * bytes which can currently be read).
 */
size_t cbuf_space_used(cbuf_t *cbuf);

/**
 * cbuf_size
 *
 * @param[in] cbuf The cbuf instance to query
 *
 * @return The size of the cbuf's underlying data buffer.
 */
static inline size_t cbuf_size(cbuf_t *cbuf) {
	return (1UL << cbuf->len_pow2);
}

/**
 * cbuf_reset
 *
 * Reset the cbuf instance, discarding any data which may be in the buffer at
 * the moment.
 *
 * @param[in] cbuf The cbuf instance to reset.
 */
static inline void cbuf_reset(cbuf_t *cbuf) {
	cbuf_read(cbuf, NULL, cbuf_size(cbuf), false);
}

/* special cases for dealing with a single char of data */
size_t cbuf_read_char(cbuf_t *cbuf, char *c, bool block);
size_t cbuf_write_char(cbuf_t *cbuf, char c, bool canreschedule);

#endif

