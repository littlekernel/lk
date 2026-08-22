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

#include <kernel/semaphore.h>

#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>

/* A semaphore's count is protected by the lock of its wait queue. */

void sem_init(semaphore_t *sem, int initial_count) {
    *sem = (semaphore_t)SEMAPHORE_INITIAL_VALUE(*sem, initial_count);
}

void sem_destroy(semaphore_t *sem) {
    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&sem->wait);
    sem->count = 0;
    wait_queue_destroy(&sem->wait);
    wait_queue_unlock_irqrestore(&sem->wait, state);
}

int sem_post(semaphore_t *sem) {
    int ret = 0;

    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&sem->wait);

    /*
     * If the count is or was negative then a thread is waiting for a resource, otherwise
     * it's safe to just increase the count available with no downsides
     */
    if (unlikely(++sem->count <= 0))
        ret = wait_queue_wake_one(&sem->wait, NO_ERROR);

    wait_queue_unlock_irqrestore(&sem->wait, state);

    return ret;
}

status_t sem_wait(semaphore_t *sem) {
    status_t ret = NO_ERROR;
    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&sem->wait);

    /*
     * If there are no resources available then we need to
     * sit in the wait queue until sem_post adds some.
     */
    if (unlikely(--sem->count < 0)) {
        /* the block releases the lock */
        ret = wait_queue_block(&sem->wait, INFINITE_TIME);
        arch_interrupt_restore(state);
        return ret;
    }

    wait_queue_unlock_irqrestore(&sem->wait, state);
    return ret;
}

status_t sem_trywait(semaphore_t *sem) {
    status_t ret = NO_ERROR;
    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&sem->wait);

    if (unlikely(sem->count <= 0)) {
        ret = ERR_NOT_READY;
    } else {
        sem->count--;
    }

    wait_queue_unlock_irqrestore(&sem->wait, state);
    return ret;
}

int sem_reset(semaphore_t *sem) {
    int discarded = 0;

    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&sem->wait);

    if (sem->count > 0) {
        discarded = sem->count;
        sem->count = 0;
    }

    wait_queue_unlock_irqrestore(&sem->wait, state);

    return discarded;
}

status_t sem_timedwait(semaphore_t *sem, lk_time_t timeout) {
    status_t ret = NO_ERROR;
    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&sem->wait);

    if (unlikely(--sem->count < 0)) {
        /* the block releases the lock */
        ret = wait_queue_block(&sem->wait, timeout);
        if (ret == ERR_TIMED_OUT) {
            /* give back the count we took, now that nobody is going to hand
             * it to us */
            spin_lock(wait_queue_lock(&sem->wait));
            sem->count++;
            spin_unlock(wait_queue_lock(&sem->wait));
        }
        arch_interrupt_restore(state);
        return ret;
    }

    wait_queue_unlock_irqrestore(&sem->wait, state);
    return ret;
}
