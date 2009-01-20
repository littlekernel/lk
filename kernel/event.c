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
#include <err.h>
#include <kernel/event.h>

#if DEBUGLEVEL > 1
#define EVENT_CHECK 1
#endif

void event_init(event_t *e, bool initial, uint flags)
{
#if EVENT_CHECK
//	ASSERT(e->magic != EVENT_MAGIC);
#endif

	e->magic = EVENT_MAGIC;
	e->signalled = initial;
	e->flags = flags;
	wait_queue_init(&e->wait);
}

void event_destroy(event_t *e)
{
	enter_critical_section();

#if EVENT_CHECK
	ASSERT(e->magic == EVENT_MAGIC);
#endif

	e->magic = 0;
	e->signalled = false;
	e->flags = 0;
	wait_queue_destroy(&e->wait, true);

	exit_critical_section();
}

status_t event_wait_timeout(event_t *e, time_t timeout)
{
	status_t ret = NO_ERROR;

	enter_critical_section();

#if EVENT_CHECK
	ASSERT(e->magic == EVENT_MAGIC);
#endif

	if (e->signalled) {
		/* signalled, we're going to fall through */
		if (e->flags & EVENT_FLAG_AUTOUNSIGNAL) {
			/* autounsignal flag lets one thread fall through before unsignalling */
			e->signalled = false;
		}
	} else {
		/* unsignalled, block here */
		ret = wait_queue_block(&e->wait, timeout);
		if (ret < 0)
			goto err;
	}

err:
	exit_critical_section();

	return ret;
}

status_t event_wait(event_t *e)
{
	return event_wait_timeout(e, INFINITE_TIME);
}

status_t event_signal(event_t *e, bool reschedule)
{
	enter_critical_section();

#if EVENT_CHECK
	ASSERT(e->magic == EVENT_MAGIC);
#endif

	if (!e->signalled) {
		if (e->flags & EVENT_FLAG_AUTOUNSIGNAL) {
			/* try to release one thread and leave unsignalled if successful */
			if (wait_queue_wake_one(&e->wait, reschedule, NO_ERROR) <= 0) {
				/*
				 * if we didn't actually find a thread to wake up, go to
				 * signalled state and let the next call to event_wait
				 * unsignal the event.
				 */
				e->signalled = true;
			}
		} else {
			/* release all threads and remain signalled */
			e->signalled = true;
			wait_queue_wake_all(&e->wait, reschedule, NO_ERROR);
		}
	}

	exit_critical_section();

	return NO_ERROR;
}

status_t event_unsignal(event_t *e)
{
	enter_critical_section();

#if EVENT_CHECK
	ASSERT(e->magic == EVENT_MAGIC);
#endif

	e->signalled = false;

	exit_critical_section();

	return NO_ERROR;
}

