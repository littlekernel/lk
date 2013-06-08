/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
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
 * @brief  Kernel threading
 *
 * This file is the core kernel threading interface.
 *
 * @defgroup thread Threads
 * @{
 */
#include <debug.h>
#include <assert.h>
#include <list.h>
#include <malloc.h>
#include <string.h>
#include <err.h>
#include <lib/dpc.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/debug.h>
#include <platform.h>
#include <target.h>
#include <lib/heap.h>

#if LK_DEBUGLEVEL > 1
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
int critical_section_count;

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

#if PLATFORM_HAS_DYNAMIC_TIMER
/* preemption timer */
static timer_t preempt_timer;
#endif

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

/**
 * @brief  Create a new thread
 *
 * This function creates a new thread.  The thread is initially suspended, so you
 * need to call thread_resume() to execute it.
 *
 * @param  name        Name of thread
 * @param  entry       Entry point of thread
 * @param  arg         Arbitrary argument passed to entry()
 * @param  priority    Execution priority for the thread.
 * @param  stack_size  Stack size for the thread.
 *
 * Thread priority is an integer from 0 (lowest) to 31 (highest).  Some standard
 * prioritys are defined in <kernel/thread.h>:
 *
 *  HIGHEST_PRIORITY
 *  DPC_PRIORITY
 *  HIGH_PRIORITY
 *  DEFAULT_PRIORITY
 *  LOW_PRIORITY
 *  IDLE_PRIORITY
 *  LOWEST_PRIORITY
 *
 * Stack size is typically set to DEFAULT_STACK_SIZE
 *
 * @return  Pointer to thread object, or NULL on failure.
 */
thread_t *thread_create_etc(thread_t *t, const char *name, thread_start_routine entry, void *arg, int priority, void *stack, size_t stack_size)
{
	unsigned int flags = 0;

	if (!t) {
		t = malloc(sizeof(thread_t));
		if (!t)
			return NULL;
		flags |= THREAD_FLAG_FREE_STRUCT;
	}

	init_thread_struct(t, name);

	t->entry = entry;
	t->arg = arg;
	t->priority = priority;
	t->saved_critical_section_count = 1; /* we always start inside a critical section */
	t->state = THREAD_SUSPENDED;
	t->blocking_wait_queue = NULL;
	t->wait_queue_block_ret = NO_ERROR;

	t->retcode = 0;
	wait_queue_init(&t->retcode_wait_queue);

	/* create the stack */
	if (!stack) {
		t->stack = malloc(stack_size);
		if (!t->stack) {
			if (flags & THREAD_FLAG_FREE_STRUCT)
				free(t);
			return NULL;
		}
		flags |= THREAD_FLAG_FREE_STACK;
	}

	t->stack_size = stack_size;

	/* save whether or not we need to free the thread struct and/or stack */
	t->flags = flags;

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

thread_t *thread_create(const char *name, thread_start_routine entry, void *arg, int priority, size_t stack_size)
{
	return thread_create_etc(NULL, name, entry, arg, priority, NULL, stack_size);
}

/**
 * @brief  Make a suspended thread executable.
 *
 * This function is typically called to start a thread which has just been
 * created with thread_create()
 *
 * @param t  Thread to resume
 *
 * @return NO_ERROR on success, ERR_NOT_SUSPENDED if thread was not suspended.
 */
status_t thread_resume(thread_t *t)
{
#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
	ASSERT(t->state != THREAD_DEATH);
#endif

	enter_critical_section();
	if (t->state == THREAD_SUSPENDED) {
		t->state = THREAD_READY;
		insert_in_run_queue_head(t);
		thread_yield();
	}
	exit_critical_section();

	return NO_ERROR;
}

status_t thread_detach_and_resume(thread_t *t)
{
	status_t err;
	err = thread_detach(t);
	if (err < 0)
		return err;
	return thread_resume(t);
}

status_t thread_join(thread_t *t, int *retcode, lk_time_t timeout)
{
#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
#endif

	enter_critical_section();

	if (t->flags & THREAD_FLAG_DETACHED) {
		/* the thread is detached, go ahead and exit */
		exit_critical_section();
		return ERR_THREAD_DETACHED;
	}

	/* wait for the thread to die */
	if (t->state != THREAD_DEATH) {
		status_t err = wait_queue_block(&t->retcode_wait_queue, timeout);
		if (err < 0) {
			exit_critical_section();
			return err;
		}
	}

#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
	ASSERT(t->state == THREAD_DEATH);
	ASSERT(t->blocking_wait_queue == NULL);
	ASSERT(!list_in_list(&t->queue_node));
#endif

	/* save the return code */
	if (retcode)
		*retcode = t->retcode;

	/* remove it from the master thread list */
	list_delete(&t->thread_list_node);

	/* clear the structure's magic */
	t->magic = 0;

	exit_critical_section();

	/* free its stack and the thread structure itself */
	if (t->flags & THREAD_FLAG_FREE_STACK && t->stack)
		free(t->stack);

	if (t->flags & THREAD_FLAG_FREE_STRUCT)
		free(t);

	return NO_ERROR;
}

status_t thread_detach(thread_t *t)
{
#if THREAD_CHECKS
	ASSERT(t->magic == THREAD_MAGIC);
#endif

	enter_critical_section();

	/* if anyone is blocked on this thread, wake them up with a specific return code */
	wait_queue_wake_all(&current_thread->retcode_wait_queue, false, ERR_THREAD_DETACHED);

	/* if it's already dead, then just do what join would have and exit */
	if (t->state == THREAD_DEATH) {
		t->flags &= ~THREAD_FLAG_DETACHED; /* makes susre thread_join continues */
		exit_critical_section();
		return thread_join(t, NULL, 0);
	} else {
		t->flags |= THREAD_FLAG_DETACHED;
		exit_critical_section();
		return NO_ERROR;
	}
}

/**
 * @brief  Terminate the current thread
 *
 * Current thread exits with the specified return code.
 *
 * This function does not return.
 */
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

	/* if we're detached, then do our teardown here */
	if (current_thread->flags & THREAD_FLAG_DETACHED) {
		/* remove it from the master thread list */
		list_delete(&current_thread->thread_list_node);

		/* clear the structure's magic */
		current_thread->magic = 0;

		/* free its stack and the thread structure itself */
		if (current_thread->flags & THREAD_FLAG_FREE_STACK && current_thread->stack)
			heap_delayed_free(current_thread->stack);

		if (current_thread->flags & THREAD_FLAG_FREE_STRUCT)
			heap_delayed_free(current_thread);
	} else {
		/* signal if anyone is waiting */
		wait_queue_wake_all(&current_thread->retcode_wait_queue, false, 0);
	}

	/* reschedule */
	thread_resched();

	panic("somehow fell through thread_exit()\n");
}

static void idle_thread_routine(void)
{
	for (;;)
		arch_idle();
}

/**
 * @brief  Cause another thread to be executed.
 *
 * Internal reschedule routine. The current thread needs to already be in whatever
 * state and queues it needs to be in. This routine simply picks the next thread and
 * switches to it.
 *
 * This is probably not the function you're looking for. See
 * thread_yield() instead.
 */
void thread_resched(void)
{
	thread_t *oldthread;
	thread_t *newthread;

//	printf("thread_resched: current %p: ", current_thread);
//	dump_thread(current_thread);

#if THREAD_CHECKS
	ASSERT(in_critical_section());
#endif

	THREAD_STATS_INC(reschedules);

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

	if (list_is_empty(&run_queue[next_queue]))
		run_queue_bitmap &= ~(1<<next_queue);

#if THREAD_CHECKS
	ASSERT(newthread);
#endif

//	printf("newthread: ");
//	dump_thread(newthread);

	newthread->state = THREAD_RUNNING;

	if (newthread == oldthread)
		return;

	/* set up quantum for the new thread if it was consumed */
	if (newthread->remaining_quantum <= 0) {
		newthread->remaining_quantum = 5; // XXX make this smarter
	}

#if THREAD_STATS
	THREAD_STATS_INC(context_switches);

	if (oldthread == idle_thread) {
		lk_bigtime_t now = current_time_hires();
		thread_stats.idle_time += now - thread_stats.last_idle_timestamp;
	}
	if (newthread == idle_thread) {
		thread_stats.last_idle_timestamp = current_time_hires();
	}
#endif

	KEVLOG_THREAD_SWITCH(oldthread, newthread);

#if THREAD_CHECKS
	ASSERT(critical_section_count > 0);
	ASSERT(newthread->saved_critical_section_count > 0);
#endif

#if PLATFORM_HAS_DYNAMIC_TIMER
	/* if we're switching from idle to a real thread, set up a periodic
	 * timer to run our preemption tick.
	 */
	if (oldthread == idle_thread) {
		timer_set_periodic(&preempt_timer, 10, (timer_callback)thread_timer_tick, NULL);
	} else if (newthread == idle_thread) {
		timer_cancel(&preempt_timer);
	}
#endif

	/* set some optional target debug leds */
	target_set_debug_led(0, newthread != idle_thread);

	/* do the switch */
	oldthread->saved_critical_section_count = critical_section_count;
	current_thread = newthread;
	critical_section_count = newthread->saved_critical_section_count;
	arch_context_switch(oldthread, newthread);
}

/**
 * @brief Yield the cpu to another thread
 *
 * This function places the current thread at the end of the run queue
 * and yields the cpu to another waiting thread (if any.)
 *
 * This function will return at some later time. Possibly immediately if
 * no other threads are waiting to execute.
 */
void thread_yield(void)
{
#if THREAD_CHECKS
	ASSERT(current_thread->magic == THREAD_MAGIC);
	ASSERT(current_thread->state == THREAD_RUNNING);
#endif

	enter_critical_section();

	THREAD_STATS_INC(yields);

	/* we are yielding the cpu, so stick ourselves into the tail of the run queue and reschedule */
	current_thread->state = THREAD_READY;
	current_thread->remaining_quantum = 0;
	insert_in_run_queue_tail(current_thread);
	thread_resched();

	exit_critical_section();
}

/**
 * @brief  Briefly yield cpu to another thread
 *
 * This function is similar to thread_yield(), except that it will
 * restart more quickly.
 *
 * This function places the current thread at the head of the run
 * queue and then yields the cpu to another thread.
 *
 * Exception:  If the time slice for this thread has expired, then
 * the thread goes to the end of the run queue.
 *
 * This function will return at some later time. Possibly immediately if
 * no other threads are waiting to execute.
 */
void thread_preempt(void)
{
#if THREAD_CHECKS
	ASSERT(current_thread->magic == THREAD_MAGIC);
	ASSERT(current_thread->state == THREAD_RUNNING);
#endif

	enter_critical_section();

#if THREAD_STATS
	if (current_thread != idle_thread)
		THREAD_STATS_INC(preempts); /* only track when a meaningful preempt happens */
#endif

	KEVLOG_THREAD_PREEMPT(current_thread);

	/* we are being preempted, so we get to go back into the front of the run queue if we have quantum left */
	current_thread->state = THREAD_READY;
	if (current_thread->remaining_quantum > 0)
		insert_in_run_queue_head(current_thread);
	else
		insert_in_run_queue_tail(current_thread); /* if we're out of quantum, go to the tail of the queue */
	thread_resched();

	exit_critical_section();
}

/**
 * @brief  Suspend thread until woken.
 *
 * This function schedules another thread to execute.  This function does not
 * return until the thread is made runable again by some other module.
 *
 * You probably don't want to call this function directly; it's meant to be called
 * from other modules, such as mutex, which will presumably set the thread's
 * state to blocked and add it to some queue or another.
 */
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
static enum handler_return thread_sleep_handler(timer_t *timer, lk_time_t now, void *arg)
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

/**
 * @brief  Put thread to sleep; delay specified in ms
 *
 * This function puts the current thread to sleep until the specified
 * delay in ms has expired.
 *
 * Note that this function could sleep for longer than the specified delay if
 * other threads are running.  When the timer expires, this thread will
 * be placed at the head of the run queue.
 */
void thread_sleep(lk_time_t delay)
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

/**
 * @brief  Initialize threading system
 *
 * This function is called once, from kmain()
 */
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
	t->flags = THREAD_FLAG_DETACHED;
	wait_queue_init(&t->retcode_wait_queue);
	list_add_head(&thread_list, &t->thread_list_node);
	current_thread = t;
}

/**
 * @brief Complete thread initialization
 *
 * This function is called once at boot time
 */
void thread_init(void)
{
#if PLATFORM_HAS_DYNAMIC_TIMER
	timer_initialize(&preempt_timer);
#endif
}

/**
 * @brief Change name of current thread
 */
void thread_set_name(const char *name)
{
	strlcpy(current_thread->name, name, sizeof(current_thread->name));
}

/**
 * @brief Change priority of current thread
 *
 * See thread_create() for a discussion of priority values.
 */
void thread_set_priority(int priority)
{
	if (priority < LOWEST_PRIORITY)
		priority = LOWEST_PRIORITY;
	if (priority > HIGHEST_PRIORITY)
		priority = HIGHEST_PRIORITY;
	current_thread->priority = priority;
}

/**
 * @brief  Become an idle thread
 *
 * This function marks the current thread as the idle thread -- the one which
 * executes when there is nothing else to do.  This function does not return.
 * This function is called once at boot time.
 */
void thread_become_idle(void)
{
	thread_set_name("idle");
	thread_set_priority(IDLE_PRIORITY);
	idle_thread = current_thread;

	/* release the implicit boot critical section and yield to the scheduler */
	exit_critical_section();
	thread_yield();

	idle_thread_routine();
}

static const char *thread_state_to_str(enum thread_state state)
{
	switch (state) {
		case THREAD_SUSPENDED: return "susp";
		case THREAD_READY: return "rdy";
		case THREAD_RUNNING: return "run";
		case THREAD_BLOCKED: return "blok";
		case THREAD_SLEEPING: return "slep";
		case THREAD_DEATH: return "deth";
		default: return "unkn";
	}
}

/**
 * @brief  Dump debugging info about the specified thread.
 */
void dump_thread(thread_t *t)
{
	dprintf(INFO, "dump_thread: t %p (%s)\n", t, t->name);
	dprintf(INFO, "\tstate %s, priority %d, remaining quantum %d, critical section %d\n",
				  thread_state_to_str(t->state), t->priority, t->remaining_quantum,
				  t->saved_critical_section_count);
	dprintf(INFO, "\tstack %p, stack_size %zd\n", t->stack, t->stack_size);
	dprintf(INFO, "\tentry %p, arg %p, flags 0x%x\n", t->entry, t->arg, t->flags);
	dprintf(INFO, "\twait queue %p, wait queue ret %d\n", t->blocking_wait_queue, t->wait_queue_block_ret);
	dprintf(INFO, "\ttls:");
	int i;
	for (i=0; i < MAX_TLS_ENTRY; i++) {
		dprintf(INFO, " 0x%x", t->tls[i]);
	}
	dprintf(INFO, "\n");
}

/**
 * @brief  Dump debugging info about all threads
 */
void dump_all_threads(void)
{
	thread_t *t;

	enter_critical_section();
	list_for_every_entry(&thread_list, t, thread_t, thread_list_node) {
		dump_thread(t);
	}
	exit_critical_section();
}

/** @} */


/**
 * @defgroup  wait  Wait Queue
 * @{
 */
void wait_queue_init(wait_queue_t *wait)
{
	*wait = (wait_queue_t)WAIT_QUEUE_INITIAL_VALUE(*wait);
}

static enum handler_return wait_queue_timeout_handler(timer_t *timer, lk_time_t now, void *arg)
{
	thread_t *thread = (thread_t *)arg;

#if THREAD_CHECKS
	ASSERT(thread->magic == THREAD_MAGIC);
#endif

	if (thread_unblock_from_wait_queue(thread, ERR_TIMED_OUT) >= NO_ERROR)
		return INT_RESCHEDULE;

	return INT_NO_RESCHEDULE;
}

/**
 * @brief  Block until a wait queue is notified.
 *
 * This function puts the current thread at the end of a wait
 * queue and then blocks until some other thread wakes the queue
 * up again.
 *
 * @param  wait     The wait queue to enter
 * @param  timeout  The maximum time, in ms, to wait
 *
 * If the timeout is zero, this function returns immediately with
 * ERR_TIMED_OUT.  If the timeout is INFINITE_TIME, this function
 * waits indefinitely.  Otherwise, this function returns with
 * ERR_TIMED_OUT at the end of the timeout period.
 *
 * @return ERR_TIMED_OUT on timeout, else returns the return
 * value specified when the queue was woken by wait_queue_wake_one().
 */
status_t wait_queue_block(wait_queue_t *wait, lk_time_t timeout)
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

/**
 * @brief  Wake up one thread sleeping on a wait queue
 *
 * This function removes one thread (if any) from the head of the wait queue and
 * makes it executable.  The new thread will be placed at the head of the
 * run queue.
 *
 * @param wait  The wait queue to wake
 * @param reschedule  If true, the newly-woken thread will run immediately.
 * @param wait_queue_error  The return value which the new thread will receive
 * from wait_queue_block().
 *
 * @return  The number of threads woken (zero or one)
 */
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


/**
 * @brief  Wake all threads sleeping on a wait queue
 *
 * This function removes all threads (if any) from the wait queue and
 * makes them executable.  The new threads will be placed at the head of the
 * run queue.
 *
 * @param wait  The wait queue to wake
 * @param reschedule  If true, the newly-woken threads will run immediately.
 * @param wait_queue_error  The return value which the new thread will receive
 * from wait_queue_block().
 *
 * @return  The number of threads woken (zero or one)
 */
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

/**
 * @brief  Free all resources allocated in wait_queue_init()
 *
 * If any threads were waiting on this queue, they are all woken.
 */
void wait_queue_destroy(wait_queue_t *wait, bool reschedule)
{
#if THREAD_CHECKS
	ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
	ASSERT(in_critical_section());
#endif
	wait_queue_wake_all(wait, reschedule, ERR_OBJECT_DESTROYED);
	wait->magic = 0;
}

/**
 * @brief  Wake a specific thread in a wait queue
 *
 * This function extracts a specific thread from a wait queue, wakes it, and
 * puts it at the head of the run queue.
 *
 * @param t  The thread to wake
 * @param wait_queue_error  The return value which the new thread will receive
 *   from wait_queue_block().
 *
 * @return ERR_NOT_BLOCKED if thread was not in any wait queue.
 */
status_t thread_unblock_from_wait_queue(thread_t *t, status_t wait_queue_error)
{
#if THREAD_CHECKS
	ASSERT(in_critical_section());
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

	return NO_ERROR;
}


