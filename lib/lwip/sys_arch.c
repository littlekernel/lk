/*
 * Copyright (c) 2006 Travis Geiselbrecht
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
#include <malloc.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <platform.h>
#include <lwip/sys.h>

static struct sys_timeouts timeouts;

void sys_init(void)
{
}

/*---------------------------------------
 * Thread stuff
 *---------------------------------------*/
sys_thread_t sys_thread_new(void (* thread)(void *arg), void *arg, int prio)
{
	thread_t *t;

	t = thread_create("lwip thread", (thread_start_routine)thread, arg, prio, DEFAULT_STACK_SIZE);
	if (t)
		thread_resume(t);

	return t;
}

struct sys_timeouts *sys_arch_timeouts(void)
{
	// XXX I think this needs to be per thread
	return &timeouts;
}

/*---------------------------------------
 * Semaphore stuff
 *---------------------------------------*/

sys_sem_t sys_sem_new(u8_t count)
{
	struct sys_sem_struct *sem;

//	dprintf("sys_sem_new: count %d\n", count);

	sem = malloc(sizeof(struct sys_sem_struct));
	if (!sem)
		return SYS_SEM_NULL;

	sem->count = count;
	wait_queue_init(&sem->wait);

//	dprintf("sys_sem_new: count %d returning sem %p\n", count, sem);

	return sem;
}

void sys_sem_signal(sys_sem_t sem)
{
	enter_critical_section();

	sem->count++;
	if (sem->count <= 0)
		wait_queue_wake_one(&sem->wait, true, NO_ERROR);

	exit_critical_section();
}

u32_t sys_arch_sem_wait(sys_sem_t sem, u32_t timeout)
{
	status_t err;
	u32_t ret = 0;

//	dprintf("sys_arch_sem_wait: sem %p, timeout %d\n", sem, timeout);

	enter_critical_section();

	sem->count--;
	if (sem->count < 0) {
		ret = current_time();
		
		err = wait_queue_block(&sem->wait, timeout);
		if (err == ERR_TIMED_OUT) {
			ret = SYS_ARCH_TIMEOUT;
		} else if (err >= 0) {
			ret = current_time() - ret;
		} else {
			panic("sys_arch_sem_wait: funny retcode from wait_queue_block %d\n", err);
		}
	}

	exit_critical_section();

	return ret;
}

void sys_sem_free(sys_sem_t sem)
{
	enter_critical_section();
	wait_queue_destroy(&sem->wait, true);
	exit_critical_section();
	free(sem);
}

/*---------------------------------------
 * Mailbox stuff
 *---------------------------------------*/

sys_mbox_t sys_mbox_new(void)
{
	struct sys_mbox_struct *mbox;

	mbox = malloc(sizeof(struct sys_mbox_struct));
	if (!mbox)
		return SYS_MBOX_NULL;

	mbox->msg = NULL;
	wait_queue_init(&mbox->wait);

//	dprintf("sys_mbox_new: returning mbox %p\n", mbox);

	return mbox;
}

void sys_mbox_post(sys_mbox_t mbox, void *msg)
{
	enter_critical_section();

//	dprintf("sys_mbox_post: mbox %p, msg %p\n", mbox, msg);

	mbox->msg = msg;
	wait_queue_wake_one(&mbox->wait, true, NO_ERROR);

	exit_critical_section();
}

u32_t sys_arch_mbox_fetch(sys_mbox_t mbox, void **msg, u32_t timeout)
{
	status_t err;
	u32_t ret = 0;

//	dprintf("sys_arch_mbox_fetch: mbox %p, msg %p, timeout %d\n", mbox, msg, timeout);

	enter_critical_section();

	if (mbox->msg == NULL) {
		ret = current_time();
		
		err = wait_queue_block(&mbox->wait, timeout);
		if (err == ERR_TIMED_OUT) {
			ret = SYS_ARCH_TIMEOUT;
			goto out;
		} else if (err >= 0) {
			ret = current_time() - ret;
		} else {
			panic("sys_arch_mbox_fetch: funny retcode from wait_queue_block %d\n", err);
		}
	}

	/* retrieve the message */
	if (msg)
		*msg = mbox->msg;
	mbox->msg = NULL;

out:
//	dprintf("sys_arch_mbox_fetch: returning with err code %d\n", ret);

	exit_critical_section();

	return ret;
}

void sys_mbox_free(sys_mbox_t mbox)
{
	enter_critical_section();
	wait_queue_destroy(&mbox->wait, true);
	exit_critical_section();
	free(mbox);
}

