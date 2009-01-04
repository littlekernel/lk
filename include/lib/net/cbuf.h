/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#ifndef _LIB_NET_CBUF_H
#define _LIB_NET_CBUF_H

#include <sys/types.h>

#define CBUF_LEN 2048

#define CBUF_FLAG_CHAIN_HEAD 1
#define CBUF_FLAG_CHAIN_TAIL 2

typedef struct cbuf {
	struct cbuf *next;
	size_t len;
	size_t total_len;
	void *data;
	int flags;

	/* used by the network stack to chain a list of these together */
	struct cbuf *packet_next;

	char dat[CBUF_LEN - 2*sizeof(struct cbuf *) - 2*sizeof(size_t) - sizeof(void *) - sizeof(int)];
} cbuf;

int cbuf_init(void);
cbuf *cbuf_get_chain(size_t len);
cbuf *cbuf_get_chain_noblock(size_t len);
void cbuf_free_chain_noblock(cbuf *buf);
void cbuf_free_chain(cbuf *buf);

size_t cbuf_get_len(cbuf *buf);
void *cbuf_get_ptr(cbuf *buf, size_t offset);
int cbuf_is_contig_region(cbuf *buf, size_t start, size_t end);

int cbuf_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len);
int cbuf_memcpy_from_chain(void *dest, cbuf *chain, size_t offset, size_t len);

int cbuf_user_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len);
int cbuf_user_memcpy_from_chain(void *dest, cbuf *chain, size_t offset, size_t len);

uint16 cbuf_ones_cksum16(cbuf *chain, size_t offset, size_t len);
uint16 cbuf_ones_cksum16_2(cbuf *chain, size_t offset, size_t len, void *buf, size_t buf_len);

cbuf *cbuf_merge_chains(cbuf *chain1, cbuf *chain2);
cbuf *cbuf_duplicate_chain(cbuf *chain, size_t offset, size_t len, size_t leading_space);

cbuf *cbuf_truncate_head(cbuf *chain, size_t trunc_bytes, bool free_unused);
int cbuf_truncate_tail(cbuf *chain, size_t trunc_bytes, bool free_unused);

int cbuf_extend_head(cbuf **chain, size_t extend_bytes);
int cbuf_extend_tail(cbuf *chain, size_t extend_bytes);

void cbuf_test(void);

#endif

