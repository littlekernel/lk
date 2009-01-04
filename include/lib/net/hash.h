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
#ifndef _LIB_NET_HASH_H
#define _LIB_NET_HASH_H

struct hash_iterator {
	void *ptr;
	int bucket;
};

void *hash_init(unsigned int table_size, int next_ptr_offset,
	int compare_func(void *a, const void *key),
	unsigned int hash_func(void *a, const void *key, unsigned int range));
int hash_uninit(void *_hash_table);
int hash_insert(void *_hash_table, void *_elem);
int hash_remove(void *_hash_table, void *_elem);
void *hash_find(void *_hash_table, void *e);
void *hash_lookup(void *_hash_table, const void *key);
struct hash_iterator *hash_open(void *_hash_table, struct hash_iterator *i);
void hash_close(void *_hash_table, struct hash_iterator *i, bool free_iterator);
void *hash_next(void *_hash_table, struct hash_iterator *i);
void hash_rewind(void *_hash_table, struct hash_iterator *i);
void hash_dump(void *_hash_table);

/* function ptrs must look like this:
	// hash function should calculate hash on either e or key,
	// depending on which one is not NULL
unsigned int hash_func(void *e, const void *key, unsigned int range);
	// compare function should compare the element with
	// the key, returning 0 if equal, other if not
	// NOTE: compare func can be null, in which case the hash
	// code will compare the key pointer with the target
int compare_func(void *e, const void *key);
*/

unsigned int hash_hash_str( const char *str );

#endif

