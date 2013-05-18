/* semaphore.h
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

#ifndef __KERNEL_SEMAPHORE_H
#define __KERNEL_SEMAPHORE_H

#include <kernel/thread.h>
#include <kernel/mutex.h>

#define SEMAPHORE_MAGIC 'sema'

typedef struct semaphore {
	int magic;
	int count;
	wait_queue_t wait;
} semaphore_t;

#define SEMAPHORE_INITIAL_VALUE(s, _count) \
{ \
	.magic = SEMAPHORE_MAGIC, \
	.count = _count, \
	.wait = WAIT_QUEUE_INITIAL_VALUE((s).wait), \
}

void sem_init(semaphore_t *, unsigned int);
void sem_destroy(semaphore_t *);
status_t sem_post(semaphore_t *);
status_t sem_wait(semaphore_t *);
status_t sem_trywait(semaphore_t *);
status_t sem_timedwait(semaphore_t *, lk_time_t);
#endif
