/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
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

/**
 * @file
 * @brief  Event wait and signal functions for threads.
 * @defgroup event Events
 *
 * An event is a subclass of a wait queue.
 *
 * Threads wait for events, with optional timeouts.
 *
 * Events are "signaled", releasing waiting threads to continue.
 * Signals may be one-shot signals (EVENT_FLAG_AUTOUNSIGNAL), in which
 * case one signal releases only one thread, at which point it is
 * automatically cleared. Otherwise, signals release all waiting threads
 * to continue immediately until the signal is manually cleared with
 * event_unsignal().
 *
 * @{
 */

#include <debug.h>
#include <assert.h>
#include <err.h>
#include <kernel/event.h>

/**
 * @brief  Initialize an event object
 *
 * @param e        Event object to initialize
 * @param initial  Initial value for "signaled" state
 * @param flags    0 or EVENT_FLAG_AUTOUNSIGNAL
 */
void event_init(event_t *e, bool initial, uint flags)
{
	*e = (event_t)EVENT_INITIAL_VALUE(*e, initial, flags);
}

/**
 * @brief  Destroy an event object.
 *
 * Event's resources are freed and it may no longer be
 * used until event_init() is called again.  Any threads
 * still waiting on the event will be resumed.
 *
 * @param e        Event object to initialize
 */
void event_destroy(event_t *e)
{
	DEBUG_ASSERT(e->magic == EVENT_MAGIC);

	enter_critical_section();

	e->magic = 0;
	e->signalled = false;
	e->flags = 0;
	wait_queue_destroy(&e->wait, true);

	exit_critical_section();
}

/**
 * @brief  Wait for event to be signaled
 *
 * If the event has already been signaled, this function
 * returns immediately.  Otherwise, the current thread
 * goes to sleep until the event object is signaled,
 * the timeout is reached, or the event object is destroyed
 * by another thread.
 *
 * @param e        Event object
 * @param timeout  Timeout value, in ms
 *
 * @return  0 on success, ERR_TIMED_OUT on timeout,
 *         other values on other errors.
 */
status_t event_wait_timeout(event_t *e, lk_time_t timeout)
{
	status_t ret = NO_ERROR;

	DEBUG_ASSERT(e->magic == EVENT_MAGIC);

	enter_critical_section();

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

/**
 * @brief  Signal an event
 *
 * Signals an event.  If EVENT_FLAG_AUTOUNSIGNAL is set in the event
 * object's flags, only one waiting thread is allowed to proceed.  Otherwise,
 * all waiting threads are allowed to proceed until such time as
 * event_unsignal() is called.
 *
 * @param e           Event object
 * @param reschedule  If true, waiting thread(s) are executed immediately,
 *                    and the current thread resumes only after the
 *                    waiting threads have been satisfied. If false,
 *                    waiting threads are placed at the end of the run
 *                    queue.
 *
 * @return  Returns NO_ERROR on success.
 */
status_t event_signal(event_t *e, bool reschedule)
{
	DEBUG_ASSERT(e->magic == EVENT_MAGIC);

	enter_critical_section();

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

/**
 * @brief  Clear the "signaled" property of an event
 *
 * Used mainly for event objects without the EVENT_FLAG_AUTOUNSIGNAL
 * flag.  Once this function is called, threads that call event_wait()
 * functions will once again need to wait until the event object
 * is signaled.
 *
 * @param e  Event object
 *
 * @return  Returns NO_ERROR on success.
 */
status_t event_unsignal(event_t *e)
{
	DEBUG_ASSERT(e->magic == EVENT_MAGIC);

	e->signalled = false;

	return NO_ERROR;
}

