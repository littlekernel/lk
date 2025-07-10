// semaphore.h
//
// Copyright 2012 Christopher Anderson <chris@nullcode.org>
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <lk/compiler.h>
#include <stdint.h>

// This file defines the semaphore API for the LK kernel.

// Semaphores are simple single count synchronization primitives.
// They are used to control access to a shared resource by multiple threads.

__BEGIN_CDECLS

#define SEMAPHORE_MAGIC (0x73656D61) // 'sema'

typedef struct semaphore {
    uint32_t magic;
    int count;
    wait_queue_t wait;
} semaphore_t;

// Initializes a semaphore to the default state. Can statically initialize a semaphore
// with the SEMAPHORE_INITIAL_VALUE macro or dynamically initialize it with sem_init().
#define SEMAPHORE_INITIAL_VALUE(s, initial_count) \
{ \
    .magic = SEMAPHORE_MAGIC, \
    .count = (initial_count), \
    .wait = WAIT_QUEUE_INITIAL_VALUE((s).wait), \
}

void sem_init(semaphore_t *, int initial_count);

// Destroys a semaphore, releasing any resources it holds.
// Any threads waiting on the semaphore will be woken up with an error status.
void sem_destroy(semaphore_t *);

// Posts to a semaphore, incrementing its count.
// If the count was negative, it wakes one waiting thread.
// Returns the number of threads woken up.
// If resched is true, it will also trigger a reschedule if a thread was woken.
int sem_post(semaphore_t *, bool resched);


// Waits on a semaphore, decrementing its count. The current thread will block
// until the semaphore count is greater than zero.
// Returns NO_ERROR if the semaphore was acquired, or an error status if the semaphore
// was destroyed while waiting.
status_t sem_wait(semaphore_t *);

// Tries to wait on a semaphore without blocking.
// Returns ERR_NOT_READY if the semaphore count is not greater than zero,
// or NO_ERROR if the semaphore was acquired.
status_t sem_trywait(semaphore_t *);

// Waits on a semaphore with a timeout. If the semaphore count is not greater than zero,
// the current thread will block until the semaphore is posted or the timeout expires.
// Returns NO_ERROR if the semaphore was acquired, ERR_TIMED_OUT if the timeout expired,
// or ERR_OBJECT_DESTROYED if the semaphore was destroyed while waiting.
status_t sem_timedwait(semaphore_t *, lk_time_t);

__END_CDECLS
