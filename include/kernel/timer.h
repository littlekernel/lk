/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
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
#ifndef __KERNEL_TIMER_H
#define __KERNEL_TIMER_H

#include <list.h>
#include <sys/types.h>

void timer_init(void);

struct timer;
typedef enum handler_return (*timer_callback)(struct timer *, lk_time_t now, void *arg);

#define TIMER_MAGIC 'timr'

typedef struct timer {
	int magic;
	struct list_node node;

	lk_time_t scheduled_time;
	lk_time_t periodic_time;

	timer_callback callback;
	void *arg;
} timer_t;

#define TIMER_INITIAL_VALUE(t) \
{ \
	.magic = TIMER_MAGIC, \
	.node = LIST_INITIAL_CLEARED_VALUE, \
	.scheduled_time = 0, \
	.periodic_time = 0, \
	.callback = NULL, \
	.arg = NULL, \
}

/* Rules for Timers:
 * - Timer callbacks occur from interrupt context
 * - Timers may be programmed or canceled from interrupt or thread context
 * - Timers may be canceled or reprogrammed from within their callback
 * - Timers currently are dispatched from a 10ms periodic tick
*/
void timer_initialize(timer_t *);
void timer_set_oneshot(timer_t *, lk_time_t delay, timer_callback, void *arg);
void timer_set_periodic(timer_t *, lk_time_t period, timer_callback, void *arg);
void timer_cancel(timer_t *);

#endif

