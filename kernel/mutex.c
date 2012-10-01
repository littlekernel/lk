/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 * Copyright (c) 2012-2012 Shantanu Gupta
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
 * @brief  Mutex functions
 *
 * @defgroup mutex Mutex
 * @{
 */

#include <debug.h>
#include <assert.h>
#include <err.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>

/**
 * @brief  Initialize a mutex_t
 */
void mutex_init(mutex_t *m)
{
#if MUTEX_CHECK
	ASSERT(m->magic != MUTEX_MAGIC);
	m->magic = MUTEX_MAGIC;
	m->holder = 0; // In good code, release is only called if acquire was successful	
#endif

	m->count = 0;
	wait_queue_init(&m->wait);
}

/**
 * @brief  Destroy a mutex_t
 *
 * This function frees any resources that were allocated
 * in mutex_init().  The mutex_t object itself is not freed.
 */
void mutex_destroy(mutex_t *m)
{
	enter_critical_section();

#if MUTEX_CHECK
	ASSERT(m->magic == MUTEX_MAGIC);
	m->magic = 0;

	if (m->holder != 0 && current_thread != m->holder)
		panic("mutex_destroy: thread %p (%s) tried to release mutex %p it doesn't own. owned by %p (%s)\n", 
				current_thread, current_thread->name, m, m->holder, m->holder->name);
#endif

	m->count = 0;
	wait_queue_destroy(&m->wait, true);
	exit_critical_section();
}

/**
 * @brief  Acquire a mutex; wait if needed.
 *
 * This function waits for a mutex to become available.  It
 * may wait forever if the mutex never becomes free.
 *
 * @return  NO_ERROR on success, other values on error
 */
status_t mutex_acquire(mutex_t *m)
{
	status_t ret = NO_ERROR;

#if MUTEX_CHECK
	ASSERT(m->magic == MUTEX_MAGIC);

	if (current_thread == m->holder)
		panic("mutex_acquire: thread %p (%s) tried to acquire mutex %p it already owns.\n",
				current_thread, current_thread->name, m);
#endif

	enter_critical_section();

	if (unlikely(++m->count > 1)) {
		/* 
		 * block on the wait queue. If it returns an error, it was likely destroyed
		 * out from underneath us, so make sure we dont scribble thread ownership 
		 * on the mutex.
		 */
		ret = wait_queue_block(&m->wait, INFINITE_TIME);
		if (ret < 0)
			goto err;
	}

#if MUTEX_CHECK
	m->holder = current_thread;
#endif

err:
	exit_critical_section();
	return ret;
}

/**
 * @brief  Mutex wait with timeout
 *
 * This function waits up to \a timeout ms for the mutex to become available.
 * Timeout may be zero, in which case this function returns immediately if
 * the mutex is not free.
 *
 * @return  NO_ERROR on success, ERR_TIMED_OUT on timeout,
 * other values on error
 */
status_t mutex_acquire_timeout(mutex_t *m, lk_time_t timeout)
{
	status_t ret = NO_ERROR;

#if MUTEX_CHECK
	if (timeout == INFINITE_TIME)
		return mutex_acquire(m); // Unecessary overhead for correct calls, this function can handle this anyway

	ASSERT(m->magic == MUTEX_MAGIC);

	if (current_thread == m->holder)
		panic("mutex_acquire_timeout: thread %p (%s) tried to acquire mutex %p it already owns.\n",
				current_thread, current_thread->name, m);
#endif


	enter_critical_section();

	if (unlikely(++m->count > 1)) {
		ret = wait_queue_block(&m->wait, timeout);
		if (ret < NO_ERROR) {
			/* if the acquisition timed out, back out the acquire and exit */
			if (ret == ERR_TIMED_OUT) {
				/* 
				 * XXX race: the mutex may have been destroyed after the timeout,
				 * but before we got scheduled again which makes messing with the
				 * count variable dangerous.
				 */
				m->count--;
				goto err;
			}
			/* if there was a general error, it may have been destroyed out from 
			 * underneath us, so just exit (which is really an invalid state anyway)
			 */
		}	
	}

#if MUTEX_CHECK
	m->holder = current_thread;
#endif

err:
	exit_critical_section();
	return ret;
}

/**
 * @brief  Release mutex
 */
status_t mutex_release(mutex_t *m)
{
#if MUTEX_CHECK
	ASSERT(m->magic == MUTEX_MAGIC);

	if (current_thread != m->holder)
		panic("mutex_release: thread %p (%s) tried to release mutex %p it doesn't own. owned by %p (%s)\n", 
				current_thread, current_thread->name, m, m->holder, m->holder ? m->holder->name : "none");

#endif

	enter_critical_section();

#if MUTEX_CHECK
	m->holder = 0;
#endif

	if (unlikely(--m->count >= 1)) {
		/* release a thread */
		wait_queue_wake_one(&m->wait, true, NO_ERROR);
	}

	exit_critical_section();
	return NO_ERROR;
}

