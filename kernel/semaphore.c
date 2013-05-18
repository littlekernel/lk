/* semaphore.c
 *
 * Copyright 2012 Christopher Anderson <chris@nullcode.org>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <debug.h>
#include <err.h>
#include <kernel/semaphore.h>
#include <kernel/thread.h>

void sem_init(semaphore_t *sem, unsigned int value)
{
	*sem = (semaphore_t)SEMAPHORE_INITIAL_VALUE(*sem, value);
}

void sem_destroy(semaphore_t *sem)
{
	enter_critical_section();
	sem->count = 0;
	wait_queue_destroy(&sem->wait, true);
	exit_critical_section();
}

status_t sem_post(semaphore_t *sem)
{
	status_t ret = NO_ERROR;
	enter_critical_section();

	/*
	 * If the count is or was negative then a thread is waiting for a resource, otherwise
	 * it's safe to just increase the count available with no downsides
	 */
	if (unlikely(++sem->count <= 0))
		wait_queue_wake_one(&sem->wait, true, NO_ERROR);

	exit_critical_section();
	return ret;
}

status_t sem_wait(semaphore_t *sem)
{
	status_t ret = NO_ERROR;
	enter_critical_section();

	/*
	 * If there are no resources available then we need to
	 * sit in the wait queue until sem_post adds some.
	 */
	if (unlikely(--sem->count < 0))
		ret = wait_queue_block(&sem->wait, INFINITE_TIME);

	exit_critical_section();
	return ret;
}

status_t sem_trywait(semaphore_t *sem)
{
	status_t ret = NO_ERROR;
	enter_critical_section();

	if (unlikely(sem->count <= 0))
		ret = ERR_NOT_READY;
	else
		sem->count--;

	exit_critical_section();
	return ret;
}

status_t sem_timedwait(semaphore_t *sem, lk_time_t timeout)
{
	status_t ret = NO_ERROR;
	enter_critical_section();

	if (unlikely(--sem->count < 0)) {
		ret = wait_queue_block(&sem->wait, timeout);
		if (ret < NO_ERROR) {
			if (ret == ERR_TIMED_OUT) {
				sem->count++;
			}
		}
	}

	exit_critical_section();
	return ret;
}
