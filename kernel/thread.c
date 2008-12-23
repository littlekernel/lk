/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <list.h>
#include <malloc.h>
#include <string.h>
#include <err.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/dpc.h>
#include <platform.h>

#if DEBUGLEVEL > 1
#define THREAD_CHECKS 1
#endif

#if THREAD_STATS
struct thread_stats thread_stats;
#endif

/* global thread list */
static struct list_node thread_list;

/* the current thread */
thread_t *current_thread;

/* the global critical section count */
int critical_section_count = 1;

/* the run queue */
static struct list_node run_queue[NUM_PRIORITIES];
static uint32_t run_queue_bitmap;

/* the bootstrap thread (statically allocated) */
static thread_t bootstrap_thread;

/* the idle thread */
thread_t *idle_thread;

/* local routines */
static void thread_resched(void);
static void idle_thread_routine(void) __NO_RETURN;

/* run queue manipulation */
static void insert_in_run_queue_head(thread_t *t)
{
#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
	ASSERT(t->state == THREAD_READY);
	ASSERT(!list_in_list(&t->queue_node));
	ASSERT(in_critical_section());
#endif

	list_add_head(&run_queue[t->priority], &t->queue_node);
	run_queue_bitmap |= (1<<t->priority);
}

static void insert_in_run_queue_tail(thread_t *t)
{
#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
	ASSERT(t->state == THREAD_READY);
	ASSERT(!list_in_list(&t->queue_node));
	ASSERT(in_critical_section());
#endif

	list_add_tail(&run_queue[t->priority], &t->queue_node);
	run_queue_bitmap |= (1<<t->priority);
}

static void init_thread_struct(thread_t *t, const char *name)
{
	memset(t, 0, sizeof(thread_t));
	t->magic = THREAD_MAGIC;
	strlcpy(t->name, name, sizeof(t->name));
}

thread_t *thread_create(const char *name, thread_start_routine entry, void *arg, int priority, size_t stack_size)
{
	thread_t *t;

	t = malloc(sizeof(thread_t));
	if (!t)
		return NULL;

	init_thread_struct(t, name);

	t->entry = entry;
	t->arg = arg;
	t->priority = priority;
	t->saved_critical_section_count = 1; /* we always start inside a critical section */
	t->state = THREAD_SUSPENDED;
	t->blocking_wait_queue = NULL;
	t->wait_queue_block_ret = NO_ERROR;

	/* create the stack */
	t->stack = malloc(stack_size);
	if (!t->stack) {
		free(t);
		return NULL;
	}

	t->stack_size = stack_size;

	/* inheirit thread local storage from the parent */
	int i;
	for (i=0; i < MAX_TLS_ENTRY; i++)
		t->tls[i] = current_thread->tls[i];

	/* set up the initial stack frame */
	arch_thread_initialize(t);

	/* add it to the global thread list */
	enter_critical_section();
	list_add_head(&thread_list, &t->thread_list_node);
	exit_critical_section();

	return t;
}

status_t thread_resume(thread_t *t)
{
#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
	ASSERT(t->state != THREAD_DEATH);
#endif

	if (t->state == THREAD_READY || t->state == THREAD_RUNNING)
		return ERR_NOT_SUSPENDED;

	enter_critical_section();
	t->state = THREAD_READY;
	insert_in_run_queue_head(t);
	thread_yield();
	exit_critical_section();

	return NO_ERROR;
}

static void thread_cleanup_dpc(void *thread)
{
	thread_t *t = (thread_t *)thread;

//	dprintf(SPEW, "thread_cleanup_dpc: thread %p (%s)\n", t, t->name);

#if THREAD_CHECKS
	ASSERT(t->state == THREAD_DEATH);
	ASSERT(t->blocking_wait_queue == NULL);
	ASSERT(!list_in_list(&t->queue_node));
#endif

	/* remove it from the master thread list */
	enter_critical_section();
	list_delete(&t->thread_list_node);
	exit_critical_section();

	/* free its stack and the thread structure itself */
	if (t->stack)
		free(t->stack);

	free(t);
}

void thread_exit(int retcode)
{
#if THREAD_CHECKS
	ASSERT(current_thread->magic == THREAD_MAGIC);
	ASSERT(current_thread->state == THREAD_RUNNING);
#endif

//	dprintf("thread_exit: current %p\n", current_thread);

	enter_critical_section();

	/* enter the dead state */
	current_thread->state = THREAD_DEATH;
	current_thread->retcode = retcode;

	/* schedule a dpc to clean ourselves up */
	dpc_queue(thread_cleanup_dpc, (void *)current_thread, DPC_FLAG_NORESCHED);

	/* reschedule */
	thread_resched();

	panic("somehow fell through thread_exit()\n");
}

static void idle_thread_routine(void)
{
	for(;;)
		arch_idle();
}

/* 
 * Internal reschedule routine. The current thread needs to already be in whatever
 * state and queues it needs to be in. This routine simply picks the next thread and
 * switches to it.
 */
void thread_resched(void)
{
	thread_t *oldthread;
	thread_t *newthread;

//	dprintf("thread_resched: current %p: ", current_thread);
//	dump_thread(current_thread);

#if THREAD_CHECKS
	ASSERT(in_critical_section());
#endif

#if THREAD_STATS
	thread_stats.reschedules++;
#endif

	oldthread = current_thread;

	// at the moment, can't deal with more than 32 priority levels
	ASSERT(NUM_PRIORITIES <= 32);

	// should at least find the idle thread
#if THREAD_CHECKS
	ASSERT(run_queue_bitmap != 0);
#endif

	int next_queue = HIGHEST_PRIORITY - __builtin_clz(run_queue_bitmap) - (32 - NUM_PRIORITIES);
	//dprintf(SPEW, "bitmap 0x%x, next %d\n", run_queue_bitmap, next_queue);

	newthread = list_remove_head_type(&run_queue[next_queue], thread_t, queue_node);

#if THREAD_CHECKS
	ASSERT(newthread);
#endif

	if (list_is_empty(&run_queue[next_queue]))
		run_queue_bitmap &= ~(1<<next_queue);

#if 0
	// XXX make this more efficient
	newthread = NULL;
	for (i=HIGHEST_PRIORITY; i >= LOWEST_PRIORITY; i--) {
		newthread = list_remove_head_type(&run_queue[i], thread_t, queue_node);
		if (newthread)
			break;
	}
#endif

//	dprintf("newthread: ");
//	dump_thread(newthread);

	newthread->state = THREAD_RUNNING;

	if (newthread == oldthread)
		return;

	/* set up quantum for the new thread if it was consumed */
	if (newthread->remaining_quantum <= 0) {
		newthread->remaining_quantum = 5; // XXX make this smarter
	}

#if THREAD_STATS
	thread_stats.context_switches++;

	if (oldthread == idle_thread) {
		bigtime_t now = current_time_hires();
		thread_stats.idle_time += now - thread_stats.last_idle_timestamp;
	}
	if (newthread == idle_thread) {
		thread_stats.last_idle_timestamp = current_time_hires();
	}
#endif

#if THREAD_CHECKS
	ASSERT(critical_section_count > 0);
	ASSERT(newthread->saved_critical_section_count > 0);
#endif

	/* do the switch */
	oldthread->saved_critical_section_count = critical_section_count;
	current_thread = newthread;
	critical_section_count = newthread->saved_critical_section_count;
	arch_context_switch(oldthread, newthread);
}

void thread_yield(void)
{
#if THREAD_CHECKS
	ASSERT(current_thread->magic == THREAD_MAGIC);
	ASSERT(current_thread->state == THREAD_RUNNING);
#endif

	enter_critical_section();

#if THREAD_STATS
	thread_stats.yields++;
#endif

	/* we are yielding the cpu, so stick ourselves into the tail of the run queue and reschedule */
	current_thread->state = THREAD_READY;
	current_thread->remaining_quantum = 0;
	insert_in_run_queue_tail(current_thread);
	thread_resched();

	exit_critical_section();
}

void thread_preempt(void)
{
#if THREAD_CHECKS
	ASSERT(current_thread->magic == THREAD_MAGIC);
	ASSERT(current_thread->state == THREAD_RUNNING);
#endif

	enter_critical_section();

#if THREAD_STATS
	if (current_thread != idle_thread)
		thread_stats.preempts++; /* only track when a meaningful preempt happens */
#endif

	/* we are being preempted, so we get to go back into the front of the run queue if we have quantum left */
	current_thread->state = THREAD_READY;
	if (current_thread->remaining_quantum > 0)
		insert_in_run_queue_head(current_thread);
	else
		insert_in_run_queue_tail(current_thread); /* if we're out of quantum, go to the tail of the queue */
	thread_resched();

	exit_critical_section();
}

void thread_block(void)
{
#if THREAD_CHECKS
	ASSERT(current_thread->magic == THREAD_MAGIC);
	ASSERT(current_thread->state == THREAD_BLOCKED);
#endif

	enter_critical_section();

	/* we are blocking on something. the blocking code should have already stuck us on a queue */
	thread_resched();

	exit_critical_section();
}

enum handler_return thread_timer_tick(void)
{
	if (current_thread == idle_thread)
		return INT_NO_RESCHEDULE;

	current_thread->remaining_quantum--;
	if (current_thread->remaining_quantum <= 0)
		return INT_RESCHEDULE;
	else
		return INT_NO_RESCHEDULE;
}

/* timer callback to wake up a sleeping thread */
static enum handler_return thread_sleep_handler(timer_t *timer, time_t now, void *arg)
{
	thread_t *t = (thread_t *)arg;

#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
	ASSERT(t->state == THREAD_SLEEPING);
#endif

	t->state = THREAD_READY;
	insert_in_run_queue_head(t);

	return INT_RESCHEDULE;
}

void thread_sleep(time_t delay)
{
	timer_t timer;

#if THREAD_CHECKS
	ASSERT(current_thread->magic == THREAD_MAGIC);
	ASSERT(current_thread->state == THREAD_RUNNING);
#endif

	timer_initialize(&timer);

	enter_critical_section();
	timer_set_oneshot(&timer, delay, thread_sleep_handler, (void *)current_thread);
	current_thread->state = THREAD_SLEEPING;
	thread_resched();
	exit_critical_section();
}

void thread_init_early(void)
{
	int i;

	/* initialize the run queues */
	for (i=0; i < NUM_PRIORITIES; i++)
		list_initialize(&run_queue[i]);

	/* initialize the thread list */
	list_initialize(&thread_list);

	/* create a thread to cover the current running state */
	thread_t *t = &bootstrap_thread;
	init_thread_struct(t, "bootstrap");

	/* half construct this thread, since we're already running */
	t->priority = HIGHEST_PRIORITY;
	t->state = THREAD_RUNNING;
	t->saved_critical_section_count = 1;
	list_add_head(&thread_list, &t->thread_list_node);
	current_thread = t;
}

void thread_init(void)
{
}

void thread_set_name(const char *name)
{
	strlcpy(current_thread->name, name, sizeof(current_thread->name));
}

void thread_set_priority(int priority)
{
	if (priority < LOWEST_PRIORITY)
		priority = LOWEST_PRIORITY;
	if (priority > HIGHEST_PRIORITY)
		priority = HIGHEST_PRIORITY;
	current_thread->priority = priority;
}

void thread_become_idle(void)
{
	thread_set_name("idle");
	thread_set_priority(IDLE_PRIORITY);
	idle_thread = current_thread;
	idle_thread_routine();
}

void dump_thread(thread_t *t)
{
	dprintf(INFO, "dump_thread: t %p (%s)\n", t, t->name);
	dprintf(INFO, "\tstate %d, priority %d, remaining quantum %d, critical section %d\n", t->state, t->priority, t->remaining_quantum, t->saved_critical_section_count);
	dprintf(INFO, "\tstack %p, stack_size %zd\n", t->stack, t->stack_size);
	dprintf(INFO, "\tentry %p, arg %p\n", t->entry, t->arg);
	dprintf(INFO, "\twait queue %p, wait queue ret %d\n", t->blocking_wait_queue, t->wait_queue_block_ret);
	dprintf(INFO, "\ttls:");
	int i;
	for (i=0; i < MAX_TLS_ENTRY; i++) {
		dprintf(INFO, " 0x%x", t->tls[i]);
	}
	dprintf(INFO, "\n");
}

void dump_all_threads(void)
{
	thread_t *t;

	enter_critical_section();
	list_for_every_entry(&thread_list, t, thread_t, thread_list_node) {
		dump_thread(t);
	}
	exit_critical_section();
}

/* wait queue */
void wait_queue_init(wait_queue_t *wait)
{
	wait->magic = WAIT_QUEUE_MAGIC;
	list_initialize(&wait->list);
	wait->count = 0;
}

static enum handler_return wait_queue_timeout_handler(timer_t *timer, time_t now, void *arg)
{
	thread_t *thread = (thread_t *)arg;

#if THREAD_CHECKS
	ASSERT(thread->magic == THREAD_MAGIC);
#endif

	if (thread_unblock_from_wait_queue(thread, false, ERR_TIMED_OUT) >= NO_ERROR)
		return INT_RESCHEDULE;

	return INT_NO_RESCHEDULE;
}

status_t wait_queue_block(wait_queue_t *wait, time_t timeout)
{
	timer_t timer;

#if THREAD_CHECKS
	ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
	ASSERT(current_thread->state == THREAD_RUNNING);
	ASSERT(in_critical_section());
#endif

	if (timeout == 0)
		return ERR_TIMED_OUT;

	list_add_tail(&wait->list, &current_thread->queue_node);
	wait->count++;
	current_thread->state = THREAD_BLOCKED;
	current_thread->blocking_wait_queue = wait;
	current_thread->wait_queue_block_ret = NO_ERROR;

	/* if the timeout is nonzero or noninfinite, set a callback to yank us out of the queue */
	if (timeout != INFINITE_TIME) {
		timer_initialize(&timer);
		timer_set_oneshot(&timer, timeout, wait_queue_timeout_handler, (void *)current_thread);
	}

	thread_block();

	/* we don't really know if the timer fired or not, so it's better safe to try to cancel it */
	if (timeout != INFINITE_TIME) {
		timer_cancel(&timer);
	}

	return current_thread->wait_queue_block_ret;
}

int wait_queue_wake_one(wait_queue_t *wait, bool reschedule, status_t wait_queue_error)
{
	thread_t *t;
	int ret = 0;

#if THREAD_CHECKS
	ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
	ASSERT(in_critical_section());
#endif

	t = list_remove_head_type(&wait->list, thread_t, queue_node);
	if (t) {
		wait->count--;
#if THREAD_CHECKS
		ASSERT(t->state == THREAD_BLOCKED);
#endif
		t->state = THREAD_READY;
		t->wait_queue_block_ret = wait_queue_error;
		t->blocking_wait_queue = NULL;

		/* if we're instructed to reschedule, stick the current thread on the head
		 * of the run queue first, so that the newly awakened thread gets a chance to run
		 * before the current one, but the current one doesn't get unnecessarilly punished.
		 */
		if (reschedule) {
			current_thread->state = THREAD_READY;
			insert_in_run_queue_head(current_thread);
		}
		insert_in_run_queue_head(t);
		if (reschedule)
			thread_resched();
		ret = 1;
	}

	return ret;
}

int wait_queue_wake_all(wait_queue_t *wait, bool reschedule, status_t wait_queue_error)
{
	thread_t *t;
	int ret = 0;

#if THREAD_CHECKS
	ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
	ASSERT(in_critical_section());
#endif

	if (reschedule && wait->count > 0) {
		/* if we're instructed to reschedule, stick the current thread on the head
		 * of the run queue first, so that the newly awakened threads get a chance to run
		 * before the current one, but the current one doesn't get unnecessarilly punished.
		 */
		current_thread->state = THREAD_READY;
		insert_in_run_queue_head(current_thread);
	}

	/* pop all the threads off the wait queue into the run queue */
	while ((t = list_remove_head_type(&wait->list, thread_t, queue_node))) {
		wait->count--;
#if THREAD_CHECKS
		ASSERT(t->state == THREAD_BLOCKED);
#endif
		t->state = THREAD_READY;
		t->wait_queue_block_ret = wait_queue_error;
		t->blocking_wait_queue = NULL;

		insert_in_run_queue_head(t);
		ret++;
	}

#if THREAD_CHECKS
	ASSERT(wait->count == 0);
#endif

	if (reschedule && ret > 0)
		thread_resched();

	return ret;
}

void wait_queue_destroy(wait_queue_t *wait, bool reschedule)
{
#if THREAD_CHECKS
	ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
	ASSERT(in_critical_section());
#endif
	wait_queue_wake_all(wait, reschedule, ERR_OBJECT_DESTROYED);
	wait->magic = 0;
}

status_t thread_unblock_from_wait_queue(thread_t *t, bool reschedule, status_t wait_queue_error)
{
	enter_critical_section();

#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
#endif

	if (t->state != THREAD_BLOCKED)
		return ERR_NOT_BLOCKED;

#if THREAD_CHECKS
	ASSERT(t->blocking_wait_queue != NULL);
	ASSERT(t->blocking_wait_queue->magic == WAIT_QUEUE_MAGIC);
	ASSERT(list_in_list(&t->queue_node));
#endif	

	list_delete(&t->queue_node);
	t->blocking_wait_queue->count--;
	t->blocking_wait_queue = NULL;
	t->state = THREAD_READY;
	t->wait_queue_block_ret = wait_queue_error;
	insert_in_run_queue_head(t);

	if (reschedule)
		thread_resched();

	exit_critical_section();

	return NO_ERROR;
}


