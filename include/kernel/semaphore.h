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

void sem_init(semaphore_t *, unsigned int);
void sem_destroy(semaphore_t *);
status_t sem_post(semaphore_t *);
status_t sem_wait(semaphore_t *);
status_t sem_trywait(semaphore_t *);
status_t sem_timedwait(semaphore_t *, lk_time_t);
#endif
