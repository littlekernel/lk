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
#ifndef __LIST_H
#define __LIST_H

#include <sys/types.h>

#define containerof(ptr, type, member) \
	((type *)((addr_t)(ptr) - offsetof(type, member)))

struct list_node {
	struct list_node *prev;
	struct list_node *next;
};

#define LIST_INITIAL_VALUE(list) { &(list), &(list) }

static inline void list_initialize(struct list_node *list)
{
	list->prev = list->next = list;
}

static inline void list_clear_node(struct list_node *item)
{
	item->prev = item->next = 0;
}

static inline bool list_in_list(struct list_node *item)
{
	if (item->prev == 0 && item->next == 0)
		return false;
	else
		return true;
}

static inline void list_add_head(struct list_node *list, struct list_node *item)
{
	item->next = list->next;
	item->prev = list;
	list->next->prev = item;
	list->next = item;
}

#define list_add_after(entry, new_entry) list_add_head(entry, new_entry)

static inline void list_add_tail(struct list_node *list, struct list_node *item)
{
	item->prev = list->prev;
	item->next = list;
	list->prev->next = item;
	list->prev = item;
}

#define list_add_before(entry, new_entry) list_add_tail(entry, new_entry)

static inline void list_delete(struct list_node *item)
{
	item->next->prev = item->prev;
	item->prev->next = item->next;
	item->prev = item->next = 0;
}

static inline struct list_node* list_remove_head(struct list_node *list)
{
	if(list->next != list) {
		struct list_node *item = list->next;
		list_delete(item);
		return item;
	} else {
		return NULL;
	}
}

#define list_remove_head_type(list, type, element) ({\
    struct list_node *__nod = list_remove_head(list);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node* list_remove_tail(struct list_node *list)
{
	if(list->prev != list) {
		struct list_node *item = list->prev;
		list_delete(item);
		return item;
	} else {
		return NULL;
	}
}

#define list_remove_tail_type(list, type, element) ({\
    struct list_node *__nod = list_remove_tail(list);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node* list_peek_head(struct list_node *list)
{
	if(list->next != list) {
		return list->next;
	} else {
		return NULL;
	}	
}

#define list_peek_head_type(list, type, element) ({\
    struct list_node *__nod = list_peek_head(list);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node* list_peek_tail(struct list_node *list)
{
	if(list->prev != list) {
		return list->prev;
	} else {
		return NULL;
	}	
}

#define list_peek_tail_type(list, type, element) ({\
    struct list_node *__nod = list_peek_tail(list);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node* list_prev(struct list_node *list, struct list_node *item)
{
	if(item->prev != list)
		return item->prev;
	else
		return NULL;
}

#define list_prev_type(list, item, type, element) ({\
    struct list_node *__nod = list_prev(list, item);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node* list_prev_wrap(struct list_node *list, struct list_node *item)
{
	if(item->prev != list)
		return item->prev;
	else if(item->prev->prev != list)
		return item->prev->prev;
	else
		return NULL;
}

#define list_prev_wrap_type(list, item, type, element) ({\
    struct list_node *__nod = list_prev_wrap(list, item);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node* list_next(struct list_node *list, struct list_node *item)
{
	if(item->next != list)
		return item->next;
	else
		return NULL;
}

#define list_next_type(list, item, type, element) ({\
    struct list_node *__nod = list_next(list, item);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node* list_next_wrap(struct list_node *list, struct list_node *item)
{
	if(item->next != list)
		return item->next;
	else if(item->next->next != list)
		return item->next->next;
	else
		return NULL;
}

#define list_next_wrap_type(list, item, type, element) ({\
    struct list_node *__nod = list_next_wrap(list, item);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

// iterates over the list, node should be struct list_node*
#define list_for_every(list, node) \
	for(node = (list)->next; node != (list); node = node->next)

// iterates over the list in a safe way for deletion of current node
// node and temp_node should be struct list_node*
#define list_for_every_safe(list, node, temp_node) \
	for(node = (list)->next, temp_node = (node)->next;\
	node != (list);\
	node = temp_node, temp_node = (node)->next)

// iterates over the list, entry should be the container structure type *
#define list_for_every_entry(list, entry, type, member) \
	for((entry) = containerof((list)->next, type, member);\
		&(entry)->member != (list);\
		(entry) = containerof((entry)->member.next, type, member))

// iterates over the list in a safe way for deletion of current node
// entry and temp_entry should be the container structure type *
#define list_for_every_entry_safe(list, entry, temp_entry, type, member) \
	for(entry = containerof((list)->next, type, member),\
		temp_entry = containerof((entry)->member.next, type, member);\
		&(entry)->member != (list);\
		entry = temp_entry, temp_entry = containerof((temp_entry)->member.next, type, member))

static inline bool list_is_empty(struct list_node *list)
{
	return (list->next == list) ? true : false;
}

#endif
