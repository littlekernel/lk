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
#include <stdlib.h>
#include <err.h>
#include <lib/net/queue.h>

typedef struct queue_typed {
	queue_element *head;
	queue_element *tail;
	int count;
} queue_typed;

int queue_init(queue *q)
{
	q->head = q->tail = NULL;
	q->count = 0;
	return 0;
}

int queue_remove_item(queue *_q, void *e)
{
	queue_typed *q = (queue_typed *)_q;
	queue_element *elem = (queue_element *)e;
	queue_element *temp, *last = NULL;

	temp = (queue_element *)q->head;
	while(temp) {
		if(temp == elem) {
			if(last) {
				last->next = temp->next;
			} else {
				q->head = temp->next;
			}
			if(q->tail == temp)
				q->tail = last;
			q->count--;
			return 0;
		}
		last = temp;
		temp = temp->next;
	}

	return -1;
}

int queue_enqueue(queue *_q, void *e)
{
	queue_typed *q = (queue_typed *)_q;
	queue_element *elem = (queue_element *)e;

	if(q->tail == NULL) {
		q->tail = elem;
		q->head = elem;
	} else {
		q->tail->next = elem;
		q->tail = elem;
	}
	elem->next = NULL;
	q->count++;
	return 0;
}

void *queue_dequeue(queue *_q)
{
	queue_typed *q = (queue_typed *)_q;
	queue_element *elem;

	elem = q->head;
	if(q->head != NULL)
		q->head = q->head->next;
	if(q->tail == elem)
		q->tail = NULL;

	if(elem != NULL)
		q->count--;

	return elem;
}

void *queue_peek(queue *q)
{
	return q->head;
}

/* fixed queue stuff */

int fixed_queue_init(fixed_queue *q, int size)
{
	if(size <= 0)
		return ERR_INVALID_ARGS;

	q->table = malloc(size * sizeof(void *));
	if(!q->table)
		return ERR_NO_MEMORY;
	q->head = 0;
	q->tail = 0;
	q->count = 0;
	q->size = size;

	return NO_ERROR;
}

void fixed_queue_destroy(fixed_queue *q)
{
	if(q->table)
		free(q->table);
}

int fixed_queue_enqueue(fixed_queue *q, void *e)
{
	if(q->count == q->size)
		return ERR_NO_MEMORY;

	q->table[q->head++] = e;
	if(q->head >= q->size) q->head = 0;
	q->count++;

	return NO_ERROR;
}

void *fixed_queue_dequeue(fixed_queue *q)
{
	void *e;

	if(q->count <= 0)
		return NULL;

	e = q->table[q->tail++];
 	if(q->tail >= q->size) q->tail = 0;
	q->count--;

	return e;
}

void *fixed_queue_peek(fixed_queue *q)
{
	if(q->count <= 0)
		return NULL;

	return q->table[q->tail];
}


