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
#ifndef __KERNEL_THREAD_H
#define __KERNEL_THREAD_H

#include <sys/types.h>
#include <list.h>
#include <compiler.h>
#include <arch/ops.h>
#include <arch/thread.h>

enum thread_state {
	THREAD_SUSPENDED = 0,
	THREAD_READY,
	THREAD_RUNNING,
	THREAD_BLOCKED,
	THREAD_SLEEPING,
	THREAD_DEATH,
};

typedef int (*thread_start_routine)(void *arg);

/* thread local storage */
enum thread_tls_list {
	MAX_TLS_ENTRY
};

#define THREAD_MAGIC 'thrd'

typedef struct thread {
	int magic;
	struct list_node thread_list_node;

	/* active bits */
	struct list_node queue_node;
	int priority;
	enum thread_state state;	
	int saved_critical_section_count;
	int remaining_quantum;

	/* if blocked, a pointer to the wait queue */
	struct wait_queue *blocking_wait_queue;
	status_t wait_queue_block_ret;

	/* architecture stuff */
	struct arch_thread arch;

	/* stack stuff */
	void *stack;
	size_t stack_size;

	/* entry point */
	thread_start_routine entry;
	void *arg;

	/* return code */
	int retcode;

	/* thread local storage */
	uint32_t tls[MAX_TLS_ENTRY];

	char name[32];
} thread_t;

/* thread priority */
#define NUM_PRIORITIES 32
#define LOWEST_PRIORITY 0
#define HIGHEST_PRIORITY (NUM_PRIORITIES - 1)
#define DPC_PRIORITY (NUM_PRIORITIES - 2)
#define IDLE_PRIORITY LOWEST_PRIORITY
#define LOW_PRIORITY (NUM_PRIORITIES / 4)
#define DEFAULT_PRIORITY (NUM_PRIORITIES / 2)
#define HIGH_PRIORITY ((NUM_PRIORITIES / 4) * 3)

/* stack size */
#define DEFAULT_STACK_SIZE 8192

/* functions */
void thread_init_early(void);
void thread_init(void);
void thread_become_idle(void) __NO_RETURN;
void thread_set_name(const char *name);
void thread_set_priority(int priority);
thread_t *thread_create(const char *name, thread_start_routine entry, void *arg, int priority, size_t stack_size);
status_t thread_resume(thread_t *);
void thread_exit(int retcode) __NO_RETURN;
void thread_sleep(time_t delay);

void dump_thread(thread_t *t);
void dump_all_threads(void);

/* scheduler routines */
void thread_yield(void); /* give up the cpu voluntarily */
void thread_preempt(void); /* get preempted (inserted into head of run queue) */
void thread_block(void); /* block on something and reschedule */

/* called on every timer tick for the scheduler to do quantum expiration */
enum handler_return thread_timer_tick(void);

/* the current thread */
extern thread_t *current_thread;

/* the idle thread */
extern thread_t *idle_thread;

/* critical sections */
extern int critical_section_count;

static inline __ALWAYS_INLINE void enter_critical_section(void)
{
	critical_section_count++;
	if (critical_section_count == 1)
		arch_disable_ints();
}

static inline __ALWAYS_INLINE void exit_critical_section(void)
{
	critical_section_count--;
	if (critical_section_count == 0)
		arch_enable_ints();
}

static inline __ALWAYS_INLINE bool in_critical_section(void)
{
	return critical_section_count > 0;
}

/* only used by interrupt glue */
static inline void inc_critical_section(void) { critical_section_count++; }
static inline void dec_critical_section(void) { critical_section_count--; }

/* thread local storage */
static inline __ALWAYS_INLINE uint32_t tls_get(uint entry)
{
	return current_thread->tls[entry];
}

static inline __ALWAYS_INLINE uint32_t tls_set(uint entry, uint32_t val)
{
	uint32_t oldval = current_thread->tls[entry];
	current_thread->tls[entry] = val;
	return oldval;
}

/* wait queue stuff */
#define WAIT_QUEUE_MAGIC 'wait'

typedef struct wait_queue {
	int magic;
	struct list_node list;
	int count;
} wait_queue_t;

/* wait queue primitive */
/* NOTE: must be inside critical section when using these */
void wait_queue_init(wait_queue_t *);

/* 
 * release all the threads on this wait queue with a return code of ERR_OBJECT_DESTROYED.
 * the caller must assure that no other threads are operating on the wait queue during or
 * after the call.
 */
void wait_queue_destroy(wait_queue_t *, bool reschedule);

/*
 * block on a wait queue.
 * return status is whatever the caller of wait_queue_wake_*() specifies.
 * a timeout other than INFINITE_TIME will set abort after the specified time
 * and return ERR_TIMED_OUT. a timeout of 0 will immediately return.
 */
status_t wait_queue_block(wait_queue_t *, time_t timeout);

/* 
 * release one or more threads from the wait queue.
 * reschedule = should the system reschedule if any is released.
 * wait_queue_error = what wait_queue_block() should return for the blocking thread.
 */
int wait_queue_wake_one(wait_queue_t *, bool reschedule, status_t wait_queue_error);
int wait_queue_wake_all(wait_queue_t *, bool reschedule, status_t wait_queue_error);

/* 
 * remove the thread from whatever wait queue it's in.
 * return an error if the thread is not currently blocked (or is the current thread) 
 */
status_t thread_unblock_from_wait_queue(thread_t *t, bool reschedule, status_t wait_queue_error);

/* thread level statistics */
#if DEBUGLEVEL > 1
#define THREAD_STATS 1
#else
#define THREAD_STATS 0
#endif
#if THREAD_STATS
struct thread_stats {
	bigtime_t idle_time;
	bigtime_t last_idle_timestamp;
	int reschedules;
	int context_switches;
	int preempts;
	int yields;
	int interrupts; /* platform code increment this */
	int timer_ints; /* timer code increment this */
	int timers; /* timer code increment this */
};

extern struct thread_stats thread_stats;

#endif

#endif

