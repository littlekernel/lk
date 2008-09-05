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
#include <list.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <platform/timer.h>
#include <platform.h>

static struct list_node timer_queue;

void timer_initialize(timer_t *timer)
{
	timer->magic = TIMER_MAGIC;
	list_clear_node(&timer->node);
	timer->scheduled_time = 0;
	timer->periodic_time = 0;
	timer->callback = 0;
	timer->arg = 0;
}

static void insert_timer_in_queue(timer_t *timer)
{
	timer_t *entry;

	list_for_every_entry(&timer_queue, entry, timer_t, node) {
		if (entry->scheduled_time > timer->scheduled_time) {
			list_add_before(&entry->node, &timer->node);
			return;
		}
	}

	/* walked off the end of the list */
	list_add_tail(&timer_queue, &timer->node);
}

void timer_set_oneshot(timer_t *timer, time_t delay, timer_callback callback, void *arg)
{
	time_t now;

//	TRACEF("delay %d, callback %p, arg %p\n", delay, callback, arg);

	DEBUG_ASSERT(timer->magic == TIMER_MAGIC);	

	if (list_in_list(&timer->node)) {
		panic("timer %p already in list\n", timer);
	}

	now = current_time();
	timer->scheduled_time = now + delay;
	timer->periodic_time = 0;
	timer->callback = callback;
	timer->arg = arg;

//	TRACEF("scheduled time %u\n", timer->scheduled_time);

	enter_critical_section();

	insert_timer_in_queue(timer);

	exit_critical_section();
}

void timer_cancel(timer_t *timer)
{
	DEBUG_ASSERT(timer->magic == TIMER_MAGIC);

	enter_critical_section();

	if (list_in_list(&timer->node))
		list_delete(&timer->node);

	exit_critical_section();
}

/* called at interrupt time to process any pending timers */
static enum handler_return timer_tick(void *arg, time_t now)
{
	timer_t *timer;
	enum handler_return ret = INT_NO_RESCHEDULE;

#if THREAD_STATS
	thread_stats.timer_ints++;
#endif

	for (;;) {
		/* see if there's an event to process */
		timer = list_peek_head_type(&timer_queue, timer_t, node);
		if (likely(!timer || now < timer->scheduled_time))
			break;

		/* process it */
		DEBUG_ASSERT(timer->magic == TIMER_MAGIC);
		list_delete(&timer->node);
//		timer = list_remove_head_type(&timer_queue, timer_t, node);
//		ASSERT(timer);

#if THREAD_STATS
		thread_stats.timers++;
#endif

//		TRACEF("firing callback %p, arg %p\n", timer->callback, timer->arg);
		if (timer->callback(timer, now, timer->arg) == INT_RESCHEDULE)
			ret = INT_RESCHEDULE;
	}

	/* let the scheduler have a shot to do quantum expiration, etc */
	if (thread_timer_tick() == INT_RESCHEDULE)
		ret = INT_RESCHEDULE;

	return INT_RESCHEDULE;
}

void timer_init(void)
{
	list_initialize(&timer_queue);

	/* register for a periodic timer tick */
	platform_set_periodic_timer(timer_tick, NULL, 10); /* 10ms */
}


