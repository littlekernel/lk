/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/heap.h>
#include <kernel/khash.h>
#include <kernel/debug.h>
#include <newos/errors.h>
#include <string.h>

#define malloc kmalloc
#define free kfree

#define VERIFY_TABLE 0

struct hash_table {
	struct hash_elem **table;
	int next_ptr_offset;
	unsigned int table_size;
	int num_elems;
	int flags;
	int (*compare_func)(void *e, const void *key);
	unsigned int (*hash_func)(void *e, const void *key, unsigned int range);
};

// XXX gross hack
#define NEXT_ADDR(t, e) ((void *)(((unsigned long)(e)) + (t)->next_ptr_offset))
#define NEXT(t, e) ((void *)(*(unsigned long *)NEXT_ADDR(t, e)))
#define PUT_IN_NEXT(t, e, val) (*(unsigned long *)NEXT_ADDR(t, e) = (long)(val))

#if VERIFY_TABLE
static bool find_in_table(struct hash_table *t, struct hash_elem *findit,
	unsigned int starting_index, struct hash_elem *starting_element)
{
	unsigned int i;
	struct hash_elem *e;

	for(i = starting_index; i < t->table_size; i++) {
		for(e = starting_element; e != NULL; e = NEXT(t, e))
			if(findit == e)
				return true;
		starting_element = t->table[i+1];
	}

	return false;
}

static int verify_hash_table(struct hash_table *t)
{
	unsigned int i;
	struct hash_elem *e;

#if 0
	static int count = 0;

	count++;
	if((count % 128) == 0)
		dprintf("verify_hash_table: count %d\n", count);
#endif
	// do a quick spin through the table, making sure the same element isn't double entered
	for(i = 0; i < t->table_size; i++) {
		for(e = t->table[i]; e != NULL; e = NEXT(t, e)) {
			if(find_in_table(t, e, i, NEXT(t, e)) == true) {
				hash_dump(t);
				panic("element %p double inserted into hash table %p\n", e, t);
			}
		}
	}

	return 0;
}

#else
#define find_in_table(t, e, si, se) ((bool)false)
#define verify_hash_table(table)
#endif

void *hash_init(unsigned int table_size, int next_ptr_offset,
	int compare_func(void *e, const void *key),
	unsigned int hash_func(void *e, const void *key, unsigned int range))
{
	struct hash_table *t;
	unsigned int i;

	t = (struct hash_table *)malloc(sizeof(struct hash_table));
	if(t == NULL) {
		return NULL;
	}

	t->table = (struct hash_elem **)malloc(sizeof(void *) * table_size);
	for(i = 0; i<table_size; i++)
		t->table[i] = NULL;
	t->table_size = table_size;
	t->next_ptr_offset = next_ptr_offset;
	t->flags = 0;
	t->num_elems = 0;
	t->compare_func = compare_func;
	t->hash_func = hash_func;

//	dprintf("hash_init: created table 0x%x, next_ptr_offset %d, compare_func 0x%x, hash_func 0x%x\n",
//		t, next_ptr_offset, compare_func, hash_func);

	return t;
}

int hash_uninit(void *_hash_table)
{
	struct hash_table *t = _hash_table;

#if 0
	if(t->num_elems > 0) {
		return -1;
	}
#endif

	free(t->table);
	free(t);

	return 0;
}

int hash_insert(void *_hash_table, void *e)
{
	struct hash_table *t = _hash_table;
	unsigned int hash;

//	dprintf("hash_insert: table 0x%x, element 0x%x\n", t, e);

	verify_hash_table(t);

	hash = t->hash_func(e, NULL, t->table_size);
	PUT_IN_NEXT(t, e, t->table[hash]);
	t->table[hash] = e;
	t->num_elems++;

	verify_hash_table(t);

	return 0;
}

int hash_remove(void *_hash_table, void *e)
{
	struct hash_table *t = _hash_table;
	void *i, *last_i;
	unsigned int hash;

	verify_hash_table(t);

	hash = t->hash_func(e, NULL, t->table_size);
	last_i = NULL;
	for(i = t->table[hash]; i != NULL; last_i = i, i = NEXT(t, i)) {
		if(i == e) {
			if(last_i != NULL)
				PUT_IN_NEXT(t, last_i, NEXT(t, i));
			else
				t->table[hash] = NEXT(t, i);
			t->num_elems--;
			verify_hash_table(t);
			return NO_ERROR;
		}
	}

	verify_hash_table(t);

	return ERR_GENERAL;
}

void *hash_find(void *_hash_table, void *e)
{
	struct hash_table *t = _hash_table;
	void *i;
	unsigned int hash;

	verify_hash_table(t);

	hash = t->hash_func(e, NULL, t->table_size);
	for(i = t->table[hash]; i != NULL; i = NEXT(t, i)) {
		if(i == e) {
			return i;
		}
	}

	return NULL;
}

void *hash_lookup(void *_hash_table, const void *key)
{
	struct hash_table *t = _hash_table;
	void *i;
	unsigned int hash;

	if(t->compare_func == NULL)
		return NULL;

	verify_hash_table(t);

	hash = t->hash_func(NULL, key, t->table_size);
	for(i = t->table[hash]; i != NULL; i = NEXT(t, i)) {
		if(t->compare_func(i, key) == 0) {
			return i;
		}
	}

	return NULL;
}

struct hash_iterator *hash_open(void *_hash_table, struct hash_iterator *i)
{
	struct hash_table *t = _hash_table;

	if(i == NULL) {
		i = (struct hash_iterator *)malloc(sizeof(struct hash_iterator));
		if(i == NULL)
			return NULL;
	}

	hash_rewind(t, i);

	return i;
}

void hash_close(void *_hash_table, struct hash_iterator *i, bool free_iterator)
{
	if(free_iterator)
		free(i);
}

void hash_rewind(void *_hash_table, struct hash_iterator *i)
{
	i->ptr = NULL;
	i->bucket = -1;
}

void *hash_next(void *_hash_table, struct hash_iterator *i)
{
	struct hash_table *t = _hash_table;
	unsigned int index;

	verify_hash_table(t);

restart:
	if(!i->ptr) {
		for(index = (unsigned int)(i->bucket + 1); index < t->table_size; index++) {
			if(t->table[index]) {
				i->bucket = index;
				i->ptr = t->table[index];
				break;
			}
		}
	} else {
		i->ptr = NEXT(t, i->ptr);
		if(!i->ptr)
			goto restart;
	}

	return i->ptr;
}

unsigned int hash_hash_str( const char *str )
{
	char ch;
	unsigned int hash = 0;

	// we assume hash to be at least 32 bits
	while( (ch = *str++) != 0 ) {
		hash ^= hash >> 28;
		hash <<= 4;
		hash ^= ch;
	}

	return hash;
}

void hash_dump(void *_hash_table)
{
	struct hash_table *t = _hash_table;
	unsigned int i;

	dprintf("hash table dump of table at %p\n", t);
	dprintf("\tnext_ptr_offset %d\n", t->next_ptr_offset);
	dprintf("\ttable_size %d\n", t->table_size);
	dprintf("\tnum_elems %d\n", t->num_elems);
	dprintf("\tflags 0x%x\n", t->flags);
	dprintf("\tcompare %p hash %p\n", t->compare_func, t->hash_func);
	dprintf("\ttable %p:\n", t->table);
	for(i = 0; i < t->table_size; i++) {
		dprintf("\t\t%p\n", t->table[i]);
	}
}

