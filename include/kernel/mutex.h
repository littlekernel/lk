/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
 * Copyright (c) 2012 Shantanu Gupta
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
#ifndef __KERNEL_MUTEX_H
#define __KERNEL_MUTEX_H

#include <debug.h>
#include <stdint.h>
#include <kernel/thread.h>

#define MUTEX_MAGIC 'mutx'

typedef struct mutex {
	uint32_t magic;
	thread_t *holder;
	int count;
	wait_queue_t wait;
} mutex_t;

#define MUTEX_INITIAL_VALUE(m) \
{ \
	.magic = MUTEX_MAGIC, \
	.holder = NULL, \
	.count = 0, \
	.wait = WAIT_QUEUE_INITIAL_VALUE((m).wait), \
}

/* Rules for Mutexes:
 * - Mutexes are only safe to use from thread context.
 * - Mutexes are non-recursive.
*/

void mutex_init(mutex_t *);
void mutex_destroy(mutex_t *);
status_t mutex_acquire_timeout(mutex_t *, lk_time_t); /* try to acquire the mutex with a timeout value */
status_t mutex_release(mutex_t *);

static inline status_t mutex_acquire(mutex_t *m) {
	return mutex_acquire_timeout(m, INFINITE_TIME);
}

#endif

