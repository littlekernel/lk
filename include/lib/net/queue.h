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
#ifndef _LIB_NET_QUEUE_H
#define _LIB_NET_QUEUE_H

typedef struct queue_element {
	void *next;
} queue_element;

typedef struct queue {
	void *head;
	void *tail;
	int count;
} queue;

int queue_init(queue *q);
int queue_remove_item(queue *q, void *e);
int queue_enqueue(queue *q, void *e);
void *queue_dequeue(queue *q);
void *queue_peek(queue *q);

typedef struct fixed_queue {
	void **table;
	int head;
	int tail;
	int count;
	int size;
} fixed_queue;

int fixed_queue_init(fixed_queue *q, int size);
void fixed_queue_destroy(fixed_queue *q);
int fixed_queue_enqueue(fixed_queue *q, void *e);
void *fixed_queue_dequeue(fixed_queue *q);
void *fixed_queue_peek(fixed_queue *q);

#endif

