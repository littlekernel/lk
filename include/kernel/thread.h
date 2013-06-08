/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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
#include <arch/defines.h>
#include <arch/ops.h>
#include <arch/thread.h>
#include <kernel/wait.h>
#include <debug.h>

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

#define THREAD_FLAG_DETACHED 0x1
#define THREAD_FLAG_FREE_STACK 0x2
#define THREAD_FLAG_FREE_STRUCT 0x4

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
	unsigned int flags;

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
	struct wait_queue retcode_wait_queue;

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
#ifdef CUSTOM_DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE CUSTOM_DEFAULT_STACK_SIZE
#else
#define DEFAULT_STACK_SIZE ARCH_DEFAULT_STACK_SIZE
#endif

/* functions */
void thread_init_early(void);
void thread_init(void);
void thread_become_idle(void) __NO_RETURN;
void thread_set_name(const char *name);
void thread_set_priority(int priority);
thread_t *thread_create(const char *name, thread_start_routine entry, void *arg, int priority, size_t stack_size);
thread_t *thread_create_etc(thread_t *t, const char *name, thread_start_routine entry, void *arg, int priority, void *stack, size_t stack_size);
status_t thread_resume(thread_t *);
void thread_exit(int retcode) __NO_RETURN;
void thread_sleep(lk_time_t delay);
status_t thread_detach(thread_t *t);
status_t thread_join(thread_t *t, int *retcode, lk_time_t timeout);
status_t thread_detach_and_resume(thread_t *t);

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
	CF;
	if (critical_section_count == 0)
		arch_disable_ints();
	critical_section_count++;
	CF;
}

static inline __ALWAYS_INLINE void exit_critical_section(void)
{
	CF;
	critical_section_count--;
	if (critical_section_count == 0)
		arch_enable_ints();
	CF;
}

static inline __ALWAYS_INLINE bool in_critical_section(void)
{
	CF;
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

/* thread level statistics */
#if LK_DEBUGLEVEL > 1
#define THREAD_STATS 1
#else
#define THREAD_STATS 0
#endif
#if THREAD_STATS
struct thread_stats {
	lk_bigtime_t idle_time;
	lk_bigtime_t last_idle_timestamp;
	int reschedules;
	int context_switches;
	int preempts;
	int yields;
	int interrupts; /* platform code increment this */
	int timer_ints; /* timer code increment this */
	int timers; /* timer code increment this */
};

extern struct thread_stats thread_stats;

#define THREAD_STATS_INC(name) do { thread_stats.name++; } while(0)

#else

#define THREAD_STATS_INC(name) do { } while (0)

#endif

#endif

