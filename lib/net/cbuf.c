/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
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
#include <debug.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <lib/net/cbuf.h>
#include <lib/net/misc.h> // for cksum16

#define LOCAL_TRACE 0 

#if DEBUG > 1
#define VALIDATE_CBUFS 1
#else
#define VALIDATE_CBUFS 0
#endif

#define ALLOCATE_CHUNK (PAGE_SIZE * 16)
#define CBUF_REGION_SIZE (1*1024*1024)
#define CBUF_BITMAP_SIZE (CBUF_REGION_SIZE / CBUF_LEN)

static cbuf *cbuf_free_list;
static mutex_t cbuf_free_list_lock;
static cbuf *cbuf_free_noblock_list;

static cbuf *cbuf_region;
static uint8_t *cbuf_bitmap;

/* initialize most of the cbuf structure */
/* does not initialize the next pointer, because it may already be in a chain */
static void initialize_cbuf(cbuf *buf)
{
	buf->len = sizeof(buf->dat);
	buf->total_len = 0;
	buf->data = buf->dat;
	buf->flags = 0;
	buf->packet_next = 0;
}

static int validate_cbuf(cbuf *head)
{
#if VALIDATE_CBUFS
	size_t counted_size;
	cbuf *tail;
	cbuf *buf;

	if(head == NULL)
		return ERR_GENERAL;

	/* make sure the first cbuf is the head */
	if((head->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		panic("validate_cbuf: cbuf %p is not head\n", head);

	/* walk through the chain and verify a few things */
	buf = head;
	counted_size = 0;
	while(buf) {
		if(buf != head && (buf->flags & CBUF_FLAG_CHAIN_HEAD))
			panic("validate_cbuf: cbuf %p has buffer %p with HEAD flag set\n", head, buf);

		if(buf->next != NULL && (buf->flags & CBUF_FLAG_CHAIN_TAIL))
			panic("validate_cbuf: cbuf %p has buffer %p with TAIL flag set\n", head, buf);

		/* make sure len makes sense */
		if(buf->len > sizeof(buf->dat))
			panic("validate_cbuf: cbuf %p has buffer %p with bad length\n", head, buf);

		/* add up the size of this part of the chain */
		counted_size += buf->len;

		/* make sure the data pointer is inside the buffer */
		if(((addr_t)buf->data < (addr_t)buf->dat)
		  || ((addr_t)buf->data - (addr_t)buf->dat > sizeof(buf->dat))
		  || ((addr_t)buf->data + buf->len > (addr_t)buf->data + sizeof(buf->dat)))
			panic("validate_cbuf: cbuf %p has buffer %p with bad pointer\n", head, buf);

		tail = buf;
		buf = buf->next;
	}

	/* look at the tail */
	if((tail->flags & CBUF_FLAG_CHAIN_TAIL) == 0)
		panic("validate_cbuf: cbuf %p 's tail at %p does not have TAIL flag set\n", head, tail);

	/* make sure the added up size == the total size */
	if(counted_size != head->total_len)
		panic("validate_cbuf: cbuf %p has bad total_len %ld, counted %ld\n", head, head->total_len, counted_size);
#endif
	return 0;
}

static void *_cbuf_alloc(size_t *size)
{
	void *buf;
	int i;
	int start;
	size_t len_found = 0;

//	LTRACEF("asked to allocate size %d\n", *size);

	enter_critical_section();

	// scan through the allocation bitmap, looking for the first free block
	// XXX not optimal
	start = -1;
	for(i = 0; i < CBUF_BITMAP_SIZE; i++) {
		// skip bytes of the bitmap at a time
		if((i % 8) == 0 && cbuf_bitmap[i/8] == 0xff) {
			i += 8;
			continue;
		}

		if(!CHECK_BIT(cbuf_bitmap[i/8], i%8)) {
			cbuf_bitmap[i/8] = SET_BIT(cbuf_bitmap[i/8], i%8);
			if(start < 0)
				start = i;
			len_found += CBUF_LEN;
			if(len_found >= *size) {
				// we're done
				break;
			}
		} else if(start >= 0) {
			// we've found a start of a run before, so we're done now
			break;
		}
	}

	if(start < 0) {
		// couldn't find any memory
		buf = NULL;
		*size = 0;
	} else {
		buf = &cbuf_region[start];
		*size = len_found;
	}

	exit_critical_section();

	return buf;
}

static cbuf *allocate_cbuf_mem(size_t size)
{
	cbuf *buf;
	cbuf *last_buf = NULL;
	cbuf *head_buf = NULL;
	size_t found_size;

	size = PAGE_ALIGN(size);

	while(size > 0) {
		found_size = size;
		buf = (cbuf *)_cbuf_alloc(&found_size);
		if(!buf) {
			// couldn't allocate, lets bail with what we have
			break;
		}
		if(!head_buf) {
			head_buf = last_buf = buf;
			head_buf->total_len = 0;
		}
		
		size -= found_size;
//		LTRACEF("returned %d of memory, %d left\n", found_size, size);

		ASSERT(found_size % CBUF_LEN == 0);
		for (; found_size > 0; found_size -= CBUF_LEN) {
			initialize_cbuf(buf);
			head_buf->total_len += buf->len;
			last_buf->next = buf;
			
			last_buf = buf;
			buf++;
		}
	}
	if(head_buf) {
		head_buf->flags |= CBUF_FLAG_CHAIN_HEAD;
		last_buf->flags |= CBUF_FLAG_CHAIN_TAIL;
		last_buf->next = NULL;
	}

	return head_buf;
}

static void _clear_chain(cbuf *head, cbuf **tail)
{
	cbuf *buf;

	buf = head;
	*tail = NULL;
	while(buf) {
		initialize_cbuf(buf); // doesn't touch the next ptr
		*tail = buf;
		buf = buf->next;
	}

	return;
}

void cbuf_free_chain_noblock(cbuf *buf)
{
	cbuf *head, *last;

	if(buf == NULL)
		return;

	head = buf;
	_clear_chain(head, &last);

	enter_critical_section();

	last->next = cbuf_free_noblock_list;
	cbuf_free_noblock_list = head;

	exit_critical_section();
}

void cbuf_free_chain(cbuf *buf)
{
	cbuf *head, *last;

	if(buf == NULL)
		return;

	head = buf;
	_clear_chain(head, &last);

	mutex_acquire(&cbuf_free_list_lock);

	last->next = cbuf_free_list;
	cbuf_free_list = head;

	mutex_release(&cbuf_free_list_lock);
}

cbuf *cbuf_get_chain(size_t len)
{
	cbuf *chain = NULL;
	cbuf *tail = NULL;
	cbuf *temp;
	size_t chain_len = 0;

	if(len == 0)
		panic("cbuf_get_chain: passed size 0\n");

	mutex_acquire(&cbuf_free_list_lock);

	while(chain_len < len) {
		if(cbuf_free_list == NULL) {
			// we need to allocate some more cbufs
			mutex_release(&cbuf_free_list_lock);
			temp = allocate_cbuf_mem(ALLOCATE_CHUNK);
			if(!temp) {
				// no more ram
				if(chain)
					cbuf_free_chain(chain);
				TRACEF("asked to allocate %ld bytes but out of memory\n", len);
				return NULL;
			}
			cbuf_free_chain(temp);
			mutex_acquire(&cbuf_free_list_lock);
			continue;
		}

		temp = cbuf_free_list;
		cbuf_free_list = cbuf_free_list->next;
		temp->flags = 0;
		temp->next = chain;
		if(chain == NULL)
			tail = temp;
		chain = temp;

		chain_len += chain->len;
	}
	mutex_release(&cbuf_free_list_lock);

	// now we have a chain, fixup the first and last entry
	chain->total_len = len;
	chain->flags |= CBUF_FLAG_CHAIN_HEAD;
	tail->len -= chain_len - len;
	tail->flags |= CBUF_FLAG_CHAIN_TAIL;

	validate_cbuf(chain);

	return chain;
}

cbuf *cbuf_get_chain_noblock(size_t len)
{
	cbuf *chain = NULL;
	cbuf *tail = NULL;
	cbuf *temp;
	size_t chain_len = 0;

	enter_critical_section();

	while(chain_len < len) {
		if(cbuf_free_noblock_list == NULL) {
			TRACEF("not enough cbufs\n");
			exit_critical_section();

			if(chain != NULL)
				cbuf_free_chain_noblock(chain);

			return NULL;
		}

		temp = cbuf_free_noblock_list;
		cbuf_free_noblock_list = cbuf_free_noblock_list->next;
		temp->next = chain;
		if(chain == NULL)
			tail = temp;
		chain = temp;

		chain_len += chain->len;
	}
	exit_critical_section();

	// now we have a chain, fixup the first and last entry
	chain->total_len = len;
	chain->flags |= CBUF_FLAG_CHAIN_HEAD;
	tail->len -= chain_len - len;
	tail->flags |= CBUF_FLAG_CHAIN_TAIL;

	return chain;
}

int cbuf_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len)
{
	cbuf *buf;
	char *src = (char *)_src;
	int buf_offset;

	validate_cbuf(chain);

	if(offset + len > chain->total_len) {
		panic("cbuf_memcpy_to_chain: offset + len > size of cbuf chain\n");
		return ERR_INVALID_ARGS;
	}

	// find the starting cbuf in the chain to copy to
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			panic("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	while(len > 0) {
		int to_copy;

		if(buf == NULL) {
			panic("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		to_copy = MIN(len, buf->len - buf_offset);
		memcpy((char *)buf->data + buf_offset, src, to_copy);

		buf_offset = 0;
		len -= to_copy;
		src += to_copy;
		buf = buf->next;
	}

	return NO_ERROR;
}

#if 0
int cbuf_user_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len)
{
	cbuf *buf;
	char *src = (char *)_src;
	int buf_offset;
	int err;

	validate_cbuf(chain);

	if(len + offset > chain->total_len) {
		TRACEF("len + offset > size of cbuf chain\n");
		return ERR_INVALID_ARGS;
	}

	// find the starting cbuf in the chain to copy to
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			TRACEF("end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	err = NO_ERROR;
	while(len > 0) {
		int to_copy;

		if(buf == NULL) {
			TRACEF("end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		to_copy = MIN(len, buf->len - buf_offset);
		if ((err = user_memcpy((char *)buf->data + buf_offset, src, to_copy) < 0))
			break; // memory exception

		buf_offset = 0;
		len -= to_copy;
		src += to_copy;
		buf = buf->next;
	}

	return err;
}
#endif

int cbuf_memcpy_from_chain(void *_dest, cbuf *chain, size_t offset, size_t len)
{
	cbuf *buf;
	char *dest = (char *)_dest;
	int buf_offset;

	validate_cbuf(chain);

	if(len + offset > chain->total_len) {
		TRACEF("len + offset > size of cbuf chain\n");
		return ERR_INVALID_ARGS;
	}

	// find the starting cbuf in the chain to copy from
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			TRACEF("end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	while(len > 0) {
		int to_copy;

		if(buf == NULL) {
			TRACEF("end of chain reached too early!\n");
			return ERR_GENERAL;
		}

		to_copy = MIN(len, buf->len - buf_offset);
		memcpy(dest, (char *)buf->data + buf_offset, to_copy);

		buf_offset = 0;
		len -= to_copy;
		dest += to_copy;
		buf = buf->next;
	}

	return NO_ERROR;
}

#if 0
int cbuf_user_memcpy_from_chain(void *_dest, cbuf *chain, size_t offset, size_t len)
{
	cbuf *buf;
	char *dest = (char *)_dest;
	int buf_offset;
	int err;

	validate_cbuf(chain);

	if(len + offset > chain->total_len) {
		TRACEF("len + offset > size of cbuf chain\n");
		return ERR_INVALID_ARGS;
	}

	// find the starting cbuf in the chain to copy from
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			TRACEF("end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	err = NO_ERROR;
	while(len > 0) {
		int to_copy;

		if(buf == NULL) {
			TRACEF("end of chain reached too early!\n");
			return ERR_GENERAL;
		}

		to_copy = MIN(len, buf->len - buf_offset);
		if ((err = user_memcpy(dest, (char *)buf->data + buf_offset, to_copy) < 0))
			break;

		buf_offset = 0;
		len -= to_copy;
		dest += to_copy;
		buf = buf->next;
	}

	return err;
}
#endif

cbuf *cbuf_duplicate_chain(cbuf *chain, size_t offset, size_t len, size_t leading_space)
{
	cbuf *buf;
	cbuf *newbuf;
	cbuf *destbuf;
	int dest_buf_offset;
	int buf_offset;

	if(!chain)
		return NULL;

	validate_cbuf(chain);

	if(offset >= chain->total_len)
		return NULL;
	len = MIN(len, chain->total_len - offset);

	newbuf = cbuf_get_chain(len + leading_space);
	if(!newbuf)
		return NULL;

	if(leading_space > 0) {
		cbuf_truncate_head(newbuf, leading_space, false);
	}

	// find the starting cbuf in the chain to copy from
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			cbuf_free_chain(newbuf);
			TRACEF("end of chain reached too early!\n");
			return NULL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	destbuf = newbuf;
	dest_buf_offset = 0;
	while(len > 0) {
		size_t to_copy;

		if(buf == NULL) {
			cbuf_free_chain(newbuf);
			TRACEF("end of source chain reached too early!\n");
			return NULL;
		}
		if(destbuf == NULL) {
			cbuf_free_chain(newbuf);
			TRACEF("end of destination chain reached too early!\n");
			return NULL;
		}

		to_copy = MIN(destbuf->len - dest_buf_offset, buf->len - buf_offset);
		to_copy = MIN(to_copy, len);
		memcpy((char *)destbuf->data + dest_buf_offset, (char *)buf->data + buf_offset, to_copy);

		len -= to_copy;
		if(to_copy + buf_offset == buf->len) {
			buf = buf->next;
			buf_offset = 0;
		} else {
			buf_offset += to_copy;
		}
		if(to_copy + dest_buf_offset == destbuf->len) {
			destbuf = destbuf->next;
			dest_buf_offset = 0;
		} else {
			dest_buf_offset += to_copy;
		}
	}

	validate_cbuf(newbuf);

	return newbuf;
}


cbuf *cbuf_merge_chains(cbuf *chain1, cbuf *chain2)
{
	cbuf *buf;

	if(!chain1 && !chain2)
		return NULL;
	if(!chain1)
		return chain2;
	if(!chain2)
		return chain1;
	if(chain1 == chain2)
		return chain1;

	validate_cbuf(chain1);
	validate_cbuf(chain2);

	// walk to the end of the first chain and tag the second one on
	buf = chain1;
	while(buf->next)
		buf = buf->next;

	buf->next = chain2;

	// modify the flags on the chain headers
	buf->flags &= ~CBUF_FLAG_CHAIN_TAIL;
	chain1->total_len += chain2->total_len;
	chain2->flags &= ~CBUF_FLAG_CHAIN_HEAD;

	return chain1;
}

size_t cbuf_get_len(cbuf *buf)
{
	if(!buf)
		return 0;

	validate_cbuf(buf);

	if(buf->flags & CBUF_FLAG_CHAIN_HEAD) {
		return buf->total_len;
	} else {
		int len = 0;
		while(buf) {
			len += buf->len;
			buf = buf->next;
		}
		return len;
	}
}

void *cbuf_get_ptr(cbuf *buf, size_t offset)
{
	validate_cbuf(buf);

	while(buf) {
		if(buf->len > offset)
			return (void *)((int)buf->data + offset);
		offset -= buf->len;
		buf = buf->next;
	}
	return NULL;
}

int cbuf_is_contig_region(cbuf *buf, size_t start, size_t end)
{
	validate_cbuf(buf);

	while(buf) {
		if(buf->len > start) {
			if(buf->len - start >= end)
				return 1;
			else
				return 0;
		}
		start -= buf->len;
		end -= buf->len;
		buf = buf->next;
	}
	return 0;
}

static uint16_t _cbuf_ones_cksum16(cbuf *buf, size_t offset, size_t len, uint16_t sum)
{
	int swapped = 0;

	if(!buf)
		return sum;

	validate_cbuf(buf);

	// find the start ptr
	while(buf) {
		if(buf->len > offset)
			break;
		if(buf->len > offset)
			return 0;
		offset -= buf->len;
		buf = buf->next;
	}

	// start checksumming
	while(buf && len > 0) {
		void *ptr = (void *)((addr_t)buf->data + offset);
		size_t plen = MIN(len, buf->len - offset);

		sum = ones_sum16(sum, ptr, plen);

		len -= plen;
		buf = buf->next;

		// if the pointer was odd, or the length was odd, but not both,
		// the checksum was swapped
		if((buf && len > 0) && (((offset % 2) && ((plen % 2) == 0)) || (((offset % 2) == 0) && (plen % 2)))) {
			swapped ^= 1;
			sum = ((sum & 0xff) << 8) | ((sum >> 8) & 0xff);
		}
		offset = 0;
	}

	if (swapped)
		sum = ((sum & 0xff) << 8) | ((sum >> 8) & 0xff);

	return sum;
}

uint16_t cbuf_ones_cksum16(cbuf *chain, size_t offset, size_t len)
{
	return ~_cbuf_ones_cksum16(chain, offset, len, 0);
}

uint16_t cbuf_ones_cksum16_2(cbuf *chain, size_t offset, size_t len, void *buf, size_t buf_len)
{
	uint16_t sum = ones_sum16(0, buf, buf_len);
	return ~_cbuf_ones_cksum16(chain, offset, len, sum);
}

cbuf *cbuf_truncate_head(cbuf *buf, size_t trunc_bytes, bool free_unused)
{
	cbuf *head = buf;
	cbuf *tail = NULL;

	if(!buf)
		return head;

	//LTRACEF("buf: total_len: %d, len: %d\n", buf->total_len, buf->len);
	validate_cbuf(buf);

	while(buf && trunc_bytes > 0) {
		int to_trunc;

		to_trunc = MIN(trunc_bytes, buf->len);

		buf->len -= to_trunc;
		buf->data = (void *)((int)buf->data + to_trunc);

		trunc_bytes -= to_trunc;
		head->total_len -= to_trunc;

		if(free_unused && buf->len == 0)
			tail = buf;
			
  		buf = buf->next;
	}

	if(!tail) {
		validate_cbuf(head);
		return head;
	}

	buf = tail->next;
	
	tail->next = NULL;
	tail->flags |= CBUF_FLAG_CHAIN_TAIL;
	
	if (buf) {
		buf->total_len = head->total_len;
		buf->flags |= CBUF_FLAG_CHAIN_HEAD;
		buf->packet_next = head->packet_next;
		//LTRACEF("new buf: total_len: %d, len: %d\n", buf->total_len, buf->len);
	}
	
	cbuf_free_chain(head);

	validate_cbuf(buf);
	return buf;
}

int cbuf_truncate_tail(cbuf *buf, size_t trunc_bytes, bool free_unused)
{
	size_t offset;
	cbuf *head = buf;
	cbuf *free_chain = NULL;

	validate_cbuf(buf);

	if(trunc_bytes > buf->total_len)
		trunc_bytes = buf->total_len;

	offset = buf->total_len - trunc_bytes;
	while (buf) {
		if (offset <= buf->len)
			break;
		offset -= buf->len;
		buf = buf->next;
	}
	if(!buf)
		return ERR_GENERAL;

	head->total_len -= buf->len - offset;
	buf->len = offset;

	if (free_unused)
		buf->flags |= CBUF_FLAG_CHAIN_TAIL;

	// clear out the rest of the buffers in this chain
	buf = buf->next;
	while(buf) {
		cbuf *temp = buf;

		// account for the loss of this buffer
		head->total_len -= buf->len;
		buf = buf->next;

		if(free_unused) {
			// stick it on the free list that we'll dispose of in a bit
			temp->next = free_chain;
			free_chain = temp;
		}
	}

	if(free_chain)
		cbuf_free_chain(free_chain);

	validate_cbuf(head);

	return NO_ERROR;
}

int cbuf_extend_head(cbuf **_buf, size_t extend_bytes)
{
	cbuf *buf;

	if(!_buf || !(*_buf))
		return ERR_INVALID_ARGS;

	buf = *_buf;

	validate_cbuf(buf);

	// first, see how much space we can allocate off the front of the chain
	if(buf->len < sizeof(buf->dat) && (addr_t)buf->data != (addr_t)buf->dat) {
		// there is some space at the front of this buffer, lets see how much
		size_t available;
		size_t to_extend;

		// check to make sure the data pointer is inside the dat buffer in this cbuf
		ASSERT((addr_t)buf->data > (addr_t)buf->dat);
		ASSERT((addr_t)buf->data - (addr_t)buf->dat < sizeof(buf->dat));

		available = (addr_t)buf->data - (addr_t)buf->dat;
		to_extend = MIN(available, extend_bytes);

		buf->len += to_extend;
		buf->data = (void *)((addr_t)buf->data - to_extend);
		extend_bytes -= to_extend;
	}

	if(extend_bytes > 0) {
		cbuf *new_buf;

		new_buf = cbuf_get_chain(extend_bytes);
		if(!new_buf)
			return ERR_NO_MEMORY;

		if(new_buf->next == NULL) {
			// simple case, the head extension is a single buffer.
			// move the data in the head to the end of the cbuf (so subsequent extends)
			// have a better shot at being able to reuse the cbuf.
			size_t move_size;

			ASSERT(new_buf->len <= sizeof(new_buf->dat));

			move_size = sizeof(new_buf->dat) - new_buf->len;

			buf->data = (void *)((addr_t)buf->data + move_size);
		}

		buf = cbuf_merge_chains(new_buf, buf);
	}

	validate_cbuf(buf);

	return NO_ERROR;
}

int cbuf_extend_tail(cbuf *head, size_t extend_bytes)
{
	cbuf *temp;
	size_t available;

	if(!head)
		return ERR_INVALID_ARGS;

	validate_cbuf(head);

	// walk to the end of this buffer
	for(temp = head; temp->next != NULL; temp = temp->next)
		;
	if(!temp)
		return ERR_INVALID_ARGS;

	// calculate the available space in this cbuf
	ASSERT((addr_t)temp->data >= (addr_t)temp->dat);
	ASSERT((addr_t)temp->data - (addr_t)temp->dat <= sizeof(temp->dat));
	ASSERT((addr_t)temp->data + temp->len <= (addr_t)temp->dat + sizeof(temp->dat));

	available = sizeof(temp->dat) - (temp->len + ((addr_t)temp->data - (addr_t)temp->dat));
	if(available > 0) {
		// we can extend by adding
		size_t extend_by = MIN(available, extend_bytes);

		temp->len += extend_by;
		head->total_len += extend_by;
		extend_bytes -= extend_by;
	}

	if(extend_bytes > 0) {
		// we still need to extend
		cbuf *new_buf;

		new_buf = cbuf_get_chain(extend_bytes);
		if(!new_buf) {
			// XXX undo any previons extension we may have done
			return ERR_NO_MEMORY;
		}

		cbuf_merge_chains(head, new_buf);
	}

	return NO_ERROR;
}


static void dbg_dump_cbuf_freelists(int argc, char **argv)
{
	cbuf *buf;

	printf("cbuf_free_list:\n");
	for(buf = cbuf_free_list; buf; buf = buf->next)
		printf("%p ", buf);
	printf("\n");

	printf("cbuf_free_noblock_list:\n");
	for(buf = cbuf_free_noblock_list; buf; buf = buf->next)
		printf("%p ", buf);
	printf("\n");
}

void cbuf_test()
{
	cbuf *buf, *buf2;
	char temp[1024];
	unsigned int i;

	printf("starting cbuffer test\n");

	buf = cbuf_get_chain(32);
	if(!buf)
		panic("cbuf_test: failed allocation of 32\n");

	buf2 = cbuf_get_chain(3*1024*1024);
	if(!buf2)
		panic("cbuf_test: failed allocation of 3mb\n");

	buf = cbuf_merge_chains(buf2, buf);

	cbuf_free_chain(buf);

	printf("allocating too much...\n");

	buf = cbuf_get_chain(128*1024*1024);
	if(buf)
		panic("cbuf_test: should have failed to allocate 128mb\n");

	printf("touching memory allocated by cbuf\n");

	buf = cbuf_get_chain(7*1024*1024);
	if(!buf)
		panic("cbuf_test: failed allocation of 7mb\n");

	for(i=0; i < sizeof(temp); i++)
		temp[i] = i;
	for(i=0; i<7*1024*1024 / sizeof(temp); i++) {
		if(i % 128 == 0) printf("%Lud\n", (long long)(i*sizeof(temp)));
		cbuf_memcpy_to_chain(buf, i*sizeof(temp), temp, sizeof(temp));
	}
	cbuf_free_chain(buf);

	printf("finished cbuffer test\n");
}

int cbuf_init()
{
	cbuf *buf;
	int i;

	cbuf_free_list = NULL;
	cbuf_free_noblock_list = NULL;

	// add the debug command
//	dbg_add_command(&dbg_dump_cbuf_freelists, "cbuf_freelist", "Dumps the cbuf free lists");

	mutex_init(&cbuf_free_list_lock);

	cbuf_region = malloc(CBUF_REGION_SIZE);
	cbuf_bitmap = malloc(CBUF_BITMAP_SIZE / 8);

	// initialize the bitmap
	for(i=0; i<CBUF_BITMAP_SIZE/8; i++)
		cbuf_bitmap[i] = 0;

#if 0
	buf = allocate_cbuf_mem(ALLOCATE_CHUNK);
	if(buf == NULL)
		return ERR_NO_MEMORY;
	cbuf_free_chain_noblock(buf);
#endif
	buf = allocate_cbuf_mem(ALLOCATE_CHUNK);
	if(buf == NULL)
		return ERR_NO_MEMORY;

	cbuf_free_chain(buf);

	return NO_ERROR;
}
