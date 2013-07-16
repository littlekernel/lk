#include <arch/sys_arch.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include <platform.h>
#include <assert.h>
#include <debug.h>
#include <err.h>
#include <stdbool.h>

#define LOCAL_TRACE 1

void sys_init(void)
{
}

static int sys_thread_func(void *arg)
{
	LTRACE_ENTRY;

	struct sys_thread *st = arg;
	DEBUG_ASSERT(st);

	tls_set(0, (uint32_t) st);

	st->func(st->arg);

	LTRACE_EXIT;
	return 0;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	struct sys_thread *st = malloc(sizeof(struct sys_thread));
	DEBUG_ASSERT(st);

	thread_t *t = thread_create(name, sys_thread_func, st, prio, stacksize);
	DEBUG_ASSERT(t);

	st->t = t;
	st->func = thread;
	st->arg = arg;

	thread_detach(t);
	thread_resume(t);

	return st;
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	sem_init(sem, count);
	return ERR_OK;
}

void sys_sem_free(sys_sem_t * sem)
{
	sem_destroy(sem);
}

int sys_sem_valid(sys_sem_t *sem)
{
	return 1;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
}

void sys_sem_signal(sys_sem_t * sem)
{
	sem_post(sem);
}

u32_t sys_arch_sem_wait(sys_sem_t * sem, u32_t timeout)
{
	lk_time_t start = current_time();

	status_t err = sem_timedwait(sem, timeout ? timeout : INFINITE_TIME);
	if (err == ERR_TIMED_OUT)
		return SYS_ARCH_TIMEOUT;
	
	return current_time() - start;
}

err_t sys_mbox_new(sys_mbox_t * mbox, int size)
{
	sem_init(&mbox->empty, size);
	sem_init(&mbox->full, 0);
	mutex_init(&mbox->lock);

	mbox->head = 0;
	mbox->tail = 0;
	mbox->size = size;

	mbox->queue = calloc(size, sizeof(void *));
	if (!mbox->queue)
		return ERR_MEM;

	return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	free(mbox->queue);
	mbox->queue = NULL;
}

void sys_mbox_post(sys_mbox_t * mbox, void *msg)
{
	sem_wait(&mbox->empty);
	mutex_acquire(&mbox->lock);

	mbox->queue[mbox->head] = msg;
	mbox->head = (mbox->head + 1) % mbox->size;

	mutex_release(&mbox->lock);
	sem_post(&mbox->full);
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t * mbox, void **msg)
{
	//LTRACE_ENTRY;

	status_t res;

	res = sem_trywait(&mbox->full);
	if (res == ERR_NOT_READY) {
		//LTRACE_EXIT;
		return SYS_MBOX_EMPTY;
	}
	
	mutex_acquire(&mbox->lock);

	*msg = mbox->queue[mbox->tail];
	mbox->tail = (mbox->tail + 1) % mbox->size;

	mutex_release(&mbox->lock);
	sem_post(&mbox->empty);

	//LTRACE_EXIT;
	return 0;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	//LTRACE_ENTRY;

	status_t res;
	lk_time_t start = current_time();

	res = sem_timedwait(&mbox->full, timeout ? timeout : INFINITE_TIME);
	if (res == ERR_TIMED_OUT) {
		//LTRACE_EXIT;
		return SYS_ARCH_TIMEOUT; //timeout ? SYS_ARCH_TIMEOUT : 0;
	}
	
	mutex_acquire(&mbox->lock);

	*msg = mbox->queue[mbox->tail];
	mbox->tail = (mbox->tail + 1) % mbox->size;

	mutex_release(&mbox->lock);
	sem_post(&mbox->empty);

	//LTRACE_EXIT;
	return current_time() - start;
}

err_t sys_mbox_trypost(sys_mbox_t * mbox, void *msg)
{
	status_t res;

	res = sem_trywait(&mbox->empty);
	if (res == ERR_NOT_READY)
		return ERR_TIMEOUT;
	
	mutex_acquire(&mbox->lock);

	mbox->queue[mbox->head] = msg;
	mbox->head = (mbox->head + 1) % mbox->size;

	mutex_release(&mbox->lock);
	sem_post(&mbox->full);

	return ERR_OK;
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
	return mbox->queue != NULL;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
}

err_t sys_mutex_new(sys_mutex_t *mutex)
{
	mutex_init(mutex);
	return ERR_OK;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
	mutex_acquire(mutex);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
	mutex_release(mutex);
}

void sys_mutex_free(sys_mutex_t *mutex)
{
	mutex_destroy(mutex);
}

