#ifndef __LIB_LWIP_ARCH_SYS_ARCH_H
#define __LIB_LWIP_ARCH_SYS_ARCH_H

#include <kernel/thread.h>
#include <kernel/semaphore.h>
#include <kernel/mutex.h>

typedef semaphore_t sys_sem_t; 
typedef mutex_t sys_mutex_t;

typedef struct {
	semaphore_t empty;
	semaphore_t full;
	mutex_t lock;

	int head;
	int tail;

	int size;

	void **queue;
} sys_mbox_t;

typedef thread_t * sys_thread_t;

#endif

