/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
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
#include <kernel/thread.h>

#include <assert.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/mp.h>
#include <kernel/sched.h>
#include <kernel/preempt.h>
#include <kernel/timer.h>
#include <lib/heap.h>
#include <lk/backtrace.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>
#include <lk/trace.h>
#include <malloc.h>
#include <platform.h>
#include <printf.h>
#include <string.h>
#include <target.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#if THREAD_STATS
struct thread_stats thread_stats[SMP_MAX_CPUS];
#endif

#define STACK_DEBUG_BYTE (0x99)
#define STACK_DEBUG_WORD (0x99999999)

#define DEBUG_THREAD_CONTEXT_SWITCH 0

/* global thread list */
struct list_node thread_list;

/* master thread spinlock */
spin_lock_t thread_lock = SPIN_LOCK_INITIAL_VALUE;



static void init_thread_struct(thread_t *t, const char *name) {
    memset(t, 0, sizeof(thread_t));
    t->magic = THREAD_MAGIC;
    thread_init_pinned_cpu(t, -1);
    thread_set_last_cpu(t, -1);
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
thread_t *thread_create_etc(thread_t *t, const char *name, thread_start_routine entry, void *arg, int priority, void *stack, size_t stack_size) {
    unsigned int flags = 0;

    if (!t) {
        t = malloc(sizeof(thread_t));
        if (!t) {
            return NULL;
        }
        flags |= THREAD_FLAG_FREE_STRUCT;
    }

    init_thread_struct(t, name);

    t->entry = entry;
    t->arg = arg;
    t->priority = priority;
    t->state = THREAD_SUSPENDED;
    t->blocking_wait_queue = NULL;
    t->wait_queue_block_ret = NO_ERROR;
    thread_set_curr_cpu(t, -1);

    t->retcode = 0;
    wait_queue_init(&t->retcode_wait_queue);

#if WITH_KERNEL_VM
    t->aspace = NULL;
#endif

    /* create the stack */
    if (!stack) {
#if THREAD_STACK_BOUNDS_CHECK
        stack_size += THREAD_STACK_PADDING_SIZE;
        flags |= THREAD_FLAG_DEBUG_STACK_BOUNDS_CHECK;
#endif
        t->stack = malloc(stack_size);
        if (!t->stack) {
            if (flags & THREAD_FLAG_FREE_STRUCT) {
                free(t);
            }
            return NULL;
        }
        flags |= THREAD_FLAG_FREE_STACK;
#if THREAD_STACK_BOUNDS_CHECK
        memset(t->stack, STACK_DEBUG_BYTE, THREAD_STACK_PADDING_SIZE);
#endif
    } else {
        t->stack = stack;
    }
#if THREAD_STACK_HIGHWATER
    if (flags & THREAD_FLAG_DEBUG_STACK_BOUNDS_CHECK) {
        memset(t->stack + THREAD_STACK_PADDING_SIZE, STACK_DEBUG_BYTE,
               stack_size - THREAD_STACK_PADDING_SIZE);
    } else {
        memset(t->stack, STACK_DEBUG_BYTE, stack_size);
    }
#endif

    t->stack_size = stack_size;

    /* save whether or not we need to free the thread struct and/or stack */
    t->flags = flags;

    /* inherit thread local storage from the parent */
    thread_t *current_thread = get_current_thread();
    int i;
    for (i = 0; i < MAX_TLS_ENTRY; i++) {
        t->tls[i] = current_thread->tls[i];
    }
    t->tls[TLS_ENTRY_ERRNO] = 0; /* clear errno */

    /* set up the initial stack frame */
    arch_thread_initialize(t);

    /* add it to the global thread list */
    THREAD_LOCK(state);
    list_add_head(&thread_list, &t->thread_list_node);
    THREAD_UNLOCK(state);

    return t;
}

thread_t *thread_create(const char *name, thread_start_routine entry, void *arg, int priority, size_t stack_size) {
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
status_t thread_resume(thread_t *t) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(t->state != THREAD_DEATH);

    // Save interrupt state before entering critical section where interrupts
    // are always disabled.
    bool ints_disabled = arch_ints_disabled();

    THREAD_LOCK(state);
    if (t->state != THREAD_SUSPENDED) {
        THREAD_UNLOCK(state);
        return ERR_NOT_SUSPENDED;
    }

    t->state = THREAD_READY;
    const uint target = sched_insert_runnable(t);
    bool local_resched = false;
    if (!ints_disabled) { /* HACK, don't resched into bootstrap thread before idle thread is set up */
        local_resched = true;
    }

    // Send an IPI to wake up the target CPU if needed. This must happen with
    // the lock still held: arch_mp_send_ipi() asserts interrupts are disabled.
    sched_poke_cpus(1U << target);

    THREAD_UNLOCK(state);

    if (local_resched) {
        if (!preempt_set_pending_if_disabled()) {
            thread_preempt();
        }
    } else if (target == arch_curr_cpu_num()) {
        /* Interrupts were already disabled, so either this is very early boot,
         * where there is nothing to switch to yet and preemption is enabled, or
         * we are inside an interrupt handler, where the irq glue has preemption
         * disabled and will take the reschedule on the way out. Either way the
         * ipi above went nowhere -- mp_reschedule() masks out the local cpu --
         * and with per-cpu run queues no other cpu will pick this thread up.
         */
        preempt_set_pending_if_disabled();
    }

    return NO_ERROR;
}

status_t thread_detach_and_resume(thread_t *t) {
    status_t err;
    err = thread_detach(t);
    if (err < 0) {
        return err;
    }
    return thread_resume(t);
}

status_t thread_join(thread_t *t, int *retcode, lk_time_t timeout) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);

    THREAD_LOCK(state);

    if (t->flags & THREAD_FLAG_DETACHED) {
        /* the thread is detached, go ahead and exit */
        THREAD_UNLOCK(state);
        return ERR_THREAD_DETACHED;
    }

    /* wait for the thread to die */
    if (t->state != THREAD_DEATH) {
        status_t err = wait_queue_block(&t->retcode_wait_queue, timeout);
        if (err < 0) {
            THREAD_UNLOCK(state);
            return err;
        }
    }

    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(t->state == THREAD_DEATH);
    DEBUG_ASSERT(t->blocking_wait_queue == NULL);
    DEBUG_ASSERT(!list_in_list(&t->queue_node));

    /* save the return code */
    if (retcode) {
        *retcode = t->retcode;
    }

    /* remove it from the master thread list */
    list_delete(&t->thread_list_node);

    /* clear the structure's magic */
    t->magic = 0;

    THREAD_UNLOCK(state);

    /* free its stack and the thread structure itself */
    if (t->flags & THREAD_FLAG_FREE_STACK && t->stack) {
        free(t->stack);
    }

    if (t->flags & THREAD_FLAG_FREE_STRUCT) {
        free(t);
    }

    return NO_ERROR;
}

status_t thread_detach(thread_t *t) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);

    /* The wake below must not reschedule inline. A context switch inside the
     * locked region hands the thread lock to the incoming thread, so a woken
     * joiner would be free to run thread_join() to completion and free |t|
     * before we get back to inspect it. Defer the reschedule past the unlock.
     */
    preempt_disable();

    THREAD_LOCK(state);

    /* if another thread is blocked inside thread_join() on this thread,
     * wake them up with a specific return code */
    wait_queue_wake_all(&t->retcode_wait_queue, ERR_THREAD_DETACHED);

    /* if it's already dead, then just do what join would have and exit */
    if (t->state == THREAD_DEATH) {
        t->flags &= ~THREAD_FLAG_DETACHED; /* makes sure thread_join continues */
        THREAD_UNLOCK(state);
        preempt_enable();
        return thread_join(t, NULL, 0);
    } else {
        t->flags |= THREAD_FLAG_DETACHED;
        THREAD_UNLOCK(state);
        preempt_enable();
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
void thread_exit(int retcode) {
    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(current_thread->magic == THREAD_MAGIC);
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);
    DEBUG_ASSERT(!thread_is_idle(current_thread));

    //  dprintf("thread_exit: current %p\n", current_thread);

    THREAD_LOCK(state);
    (void)state; /* silence unused variable warning */

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
        if (current_thread->flags & THREAD_FLAG_FREE_STACK && current_thread->stack) {
            heap_delayed_free(current_thread->stack);

            /* make sure its not going to get a bounds check performed on the half-freed stack */
            current_thread->flags &= ~THREAD_FLAG_DEBUG_STACK_BOUNDS_CHECK;
        }

        if (current_thread->flags & THREAD_FLAG_FREE_STRUCT) {
            heap_delayed_free(current_thread);
        }
    } else {
        /* Signal if anyone is waiting. This must not reschedule from inside the
         * wake: doing so marks the current thread READY and puts it back on the
         * run queue to be resumed later, undoing the THREAD_DEATH set above. We
         * are about to reschedule explicitly and never come back, so drop the
         * pending reschedule on the floor rather than acting on it.
         */
        preempt_disable();
        wait_queue_wake_all(&current_thread->retcode_wait_queue, 0);
        (void)preempt_enable_no_resched();
    }

    /* reschedule */
    sched_resched();

    /* should never return here */

    panic("somehow fell through thread_exit()\n");
}


/**
 * @brief  Initialize threading system
 *
 * This function is called once, from lk_main()
 */
void thread_init_early(void) {
    DEBUG_ASSERT(arch_curr_cpu_num() == 0);

    /* the run queues have to exist before the bootstrap thread below is half
     * constructed into one */
    sched_init_early();

    /* initialize the thread list */
    list_initialize(&thread_list);

    /* create a thread to cover the current running state */
    thread_t *t = sched_idle_thread(0);
    init_thread_struct(t, "bootstrap");

    /* half construct this thread, since we're already running */
    t->priority = HIGHEST_PRIORITY;
    t->state = THREAD_RUNNING;
    t->flags = THREAD_FLAG_DETACHED;
    thread_set_curr_cpu(t, 0);
    thread_init_pinned_cpu(t, 0);
    wait_queue_init(&t->retcode_wait_queue);
    list_add_head(&thread_list, &t->thread_list_node);
    set_current_thread(t);
}

/**
 * @brief Complete thread initialization
 *
 * This function is called once at boot time
 */
void thread_init(void) {
    sched_init();
}

/**
 * @brief Change name of current thread
 */
void thread_set_name(const char *name) {
    thread_t *current_thread = get_current_thread();
    strlcpy(current_thread->name, name, sizeof(current_thread->name));
}


#if WITH_SMP
// Get this secondary cpu onto thread context
void thread_secondary_cpu_init_early(void) {
    DEBUG_ASSERT(arch_ints_disabled());
    // Look up our idle thread and mark it as current on this cpu
    uint cpu = arch_curr_cpu_num();
    thread_t *t = sched_idle_thread(cpu);
    DEBUG_ASSERT(t && t->state == THREAD_RUNNING && t->flags & THREAD_FLAG_IDLE);
    set_current_thread(t);
}

// Construct a secondary's cpu idle thread such that it appears to already be running
void thread_create_secondary_cpu_idle_thread(uint cpu) {
    thread_t *t = sched_idle_thread(cpu);

    char name[16];
    snprintf(name, sizeof(name), "idle %u", cpu);
    init_thread_struct(t, name);

    // Half construct this thread as if it were running at max priority, since the secondary cpu will
    // start executing this thread when it starts.
    t->priority = HIGHEST_PRIORITY;
    t->state = THREAD_RUNNING;
    t->flags = THREAD_FLAG_DETACHED | THREAD_FLAG_IDLE;
    thread_set_curr_cpu(t, cpu);
    thread_init_pinned_cpu(t, cpu);
    wait_queue_init(&t->retcode_wait_queue);

    THREAD_LOCK(state);
    list_add_head(&thread_list, &t->thread_list_node);
    THREAD_UNLOCK(state);
}

// We should be on the idle thread for the secondary cpu, drop to idle priority and
// start properly scheduling by enabling interrupts and yielding to the scheduler.
void thread_secondary_cpu_entry(void) {
    uint cpu = arch_curr_cpu_num();
    thread_t *t = get_current_thread();

    DEBUG_ASSERT(t && t->state == THREAD_RUNNING && t->flags & THREAD_FLAG_IDLE);

    t->priority = IDLE_PRIORITY;

    // Mark the local cpu as active and idle
    mp_set_curr_cpu_active(true);
    mp_set_cpu_idle(cpu);

    // Enable interrupts and start the scheduler on this cpu
    arch_enable_ints();
    thread_preempt();

    // Fall through to the idle thread routine
    sched_idle_routine();
}
#endif // WITH_SMP

static const char *thread_state_to_str(enum thread_state state) {
    switch (state) {
        case THREAD_SUSPENDED:
            return "susp";
        case THREAD_READY:
            return "rdy";
        case THREAD_RUNNING:
            return "run";
        case THREAD_BLOCKED:
            return "blok";
        case THREAD_SLEEPING:
            return "slep";
        case THREAD_DEATH:
            return "deth";
        default:
            return "unkn";
    }
}

static size_t thread_stack_used(const thread_t *t) {
#ifdef THREAD_STACK_HIGHWATER
    uint8_t *stack_base;
    size_t stack_size;
    size_t i;

    stack_base = t->stack;
    stack_size = t->stack_size;

    for (i = 0; i < stack_size; i++) {
        if (stack_base[i] != STACK_DEBUG_BYTE) {
            break;
        }
    }
    return stack_size - i;
#else
    return 0;
#endif
}
/**
 * @brief  Dump debugging info about the specified thread.
 */
/* Architectures that can walk a stack override this. */
__WEAK bool arch_thread_get_backtrace_regs(const thread_t *t, uintptr_t *pc, uintptr_t *fp) {
    return false;
}

void dump_thread(const thread_t *t) {
    dprintf(INFO, "dump_thread: t %p (%s)\n", t, t->name);
#if WITH_SMP
    dprintf(INFO, "\tstate %s, curr_cpu %d, pinned_cpu %d, priority %d, remaining quantum %d\n",
            thread_state_to_str(t->state), t->curr_cpu, t->pinned_cpu, t->priority, t->remaining_quantum);
#else
    dprintf(INFO, "\tstate %s, priority %d, remaining quantum %d\n",
            thread_state_to_str(t->state), t->priority, t->remaining_quantum);
#endif
#ifdef THREAD_STACK_HIGHWATER
    dprintf(INFO, "\tstack %p, stack_size %zd, stack_used %zd\n",
            t->stack, t->stack_size, thread_stack_used(t));
#else
    dprintf(INFO, "\tstack %p, stack_size %zd\n", t->stack, t->stack_size);
#endif
    dprintf(INFO, "\tentry %p, arg %p, flags 0x%x\n", t->entry, t->arg, t->flags);
    dprintf(INFO, "\twait queue %p, wait queue ret %d\n", t->blocking_wait_queue, t->wait_queue_block_ret);
#if WITH_KERNEL_VM
    dprintf(INFO, "\taspace %p\n", t->aspace);
#endif
#if (MAX_TLS_ENTRY > 0)
    dprintf(INFO, "\ttls:");
    int i;
    for (i = 0; i < MAX_TLS_ENTRY; i++) {
        dprintf(INFO, " 0x%lx", t->tls[i]);
    }
    dprintf(INFO, "\n");
#endif
    arch_dump_thread(t);

    /* A running thread's registers are in the cpu rather than in its saved
     * frame, so only a parked one can be walked from the outside.
     */
    if (t->state != THREAD_RUNNING) {
        uintptr_t pc, fp;
        if (arch_thread_get_backtrace_regs(t, &pc, &fp)) {
            backtrace_print_thread(t, pc, fp);
        }
    }
}

void dump_all_threads_unlocked(void) {
    thread_t *t;
    list_for_every_entry(&thread_list, t, thread_t, thread_list_node) {
        if (t->magic != THREAD_MAGIC) {
            dprintf(INFO, "bad magic on thread struct %p, aborting.\n", t);
            hexdump(t, sizeof(thread_t));
            break;
        }
        dump_thread(t);
    }
}

/**
 * @brief  Dump debugging info about all threads
 */
void dump_all_threads(void) {
    THREAD_LOCK(state);
    dump_all_threads_unlocked();
    THREAD_UNLOCK(state);
}

#if THREAD_STATS
void dump_threads_stats(void) {
    thread_t *t;

    THREAD_LOCK(state);
    list_for_every_entry(&thread_list, t, thread_t, thread_list_node) {
        if (t->magic != THREAD_MAGIC) {
            dprintf(INFO, "bad magic on thread struct %p, aborting.\n", t);
            hexdump(t, sizeof(thread_t));
            break;
        }
        if (thread_is_idle(t)) {
            continue;
        }
        // thread specific stats
        dprintf(INFO, "\t(%s):\n", t->name);
        dprintf(INFO, "\t\tScheduled: %ld\n", t->stats.schedules);
        uint percent = (t->stats.total_run_time * 10000) / current_time_hires();
        dprintf(INFO, "\t\tTotal run time: %lld, %u.%02u%%\n", t->stats.total_run_time,
                percent / 100, percent % 100);
        dprintf(INFO, "\t\tLast time run: %lld\n", t->stats.last_run_timestamp);
    }
    THREAD_UNLOCK(state);
}
#endif

/** @} */

/**
 * @defgroup  wait  Wait Queue
 * @{
 */
void wait_queue_init(wait_queue_t *wait) {
    *wait = (wait_queue_t)WAIT_QUEUE_INITIAL_VALUE(*wait);
}

static enum handler_return wait_queue_timeout_handler(timer_t *timer, lk_time_t now, void *arg) {
    thread_t *thread = (thread_t *)arg;

    DEBUG_ASSERT(thread->magic == THREAD_MAGIC);

    spin_lock(&thread_lock);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (thread_unblock_from_wait_queue(thread, ERR_TIMED_OUT) >= NO_ERROR) {
        ret = INT_RESCHEDULE;
        if (preempt_set_pending_if_disabled()) {
            // If preemption was disabled, we can't reschedule now.
            ret = INT_NO_RESCHEDULE;
        }
    }

    spin_unlock(&thread_lock);

    return ret;
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
status_t wait_queue_block(wait_queue_t *wait, lk_time_t timeout) {
    timer_t timer;

    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    if (timeout == 0) {
        return ERR_TIMED_OUT;
    }

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

    sched_resched();

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
 * @param wait_queue_error  The return value which the new thread will receive
 * from wait_queue_block().
 *
 * @return  The number of threads woken (zero or one)
 */
int wait_queue_wake_one(wait_queue_t *wait, status_t wait_queue_error) {
    thread_t *t;
    int ret = 0;

    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    t = list_remove_head_type(&wait->list, thread_t, queue_node);
    if (t) {
        wait->count--;
        DEBUG_ASSERT(t->state == THREAD_BLOCKED);
        t->state = THREAD_READY;
        t->wait_queue_block_ret = wait_queue_error;
        t->blocking_wait_queue = NULL;

        /* If preemption is disabled -- by an interrupt handler, or by a caller
         * batching a run of wakeups -- just record that a reschedule is owed and
         * let whoever reenables preemption take it. Otherwise switch now.
         */
        const bool resched_now = !preempt_set_pending_if_disabled();

        /* if we're rescheduling, stick the current thread on the head
         * of the run queue first, so that the newly awakened thread gets a chance to run
         * before the current one, but the current one doesn't get unnecessarily punished.
         */
        if (resched_now) {
            current_thread->state = THREAD_READY;
            sched_insert_runnable_head_on(arch_curr_cpu_num(), current_thread);
        }
        sched_poke_cpus(1U << sched_insert_runnable(t));
        if (resched_now) {
            sched_resched();
        }
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
 * @param wait_queue_error  The return value which the new thread will receive
 * from wait_queue_block().
 *
 * @return  The number of threads woken (zero or one)
 */
int wait_queue_wake_all(wait_queue_t *wait, status_t wait_queue_error) {
    thread_t *t;
    int ret = 0;
    uint32_t cpu_mask = 0;

    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    if (wait->count == 0) {
        /* Nothing to wake. Return before touching the current thread's run queue
         * state: the self-insert below is only unwound by the sched_resched() at
         * the end of this function, which is gated on having woken something.
         * Inserting here would leave the current thread sitting on the run queue
         * while it is still running.
         */
        return 0;
    }

    /* If preemption is disabled -- by an interrupt handler, or by a caller
     * batching a run of wakeups -- just record that a reschedule is owed and
     * let whoever reenables preemption take it. Otherwise switch now.
     */
    const bool resched_now = !preempt_set_pending_if_disabled();
    if (resched_now) {
        /* stick the current thread on the head of the run queue first, so that the
         * newly awakened threads get a chance to run before the current one, but the
         * current one doesn't get unnecessarilly punished.
         */
        current_thread->state = THREAD_READY;
        sched_insert_runnable_head_on(arch_curr_cpu_num(), current_thread);
    }

    /* pop all the threads off the wait queue into the run queue */
    while ((t = list_remove_tail_type(&wait->list, thread_t, queue_node))) {
        wait->count--;
        DEBUG_ASSERT(t->state == THREAD_BLOCKED);
        t->state = THREAD_READY;
        t->wait_queue_block_ret = wait_queue_error;
        t->blocking_wait_queue = NULL;
        /* accumulate the targets and send one batch of ipis at the end rather
         * than one per thread woken */
        cpu_mask |= (1U << sched_insert_runnable(t));
        ret++;
    }

    DEBUG_ASSERT(wait->count == 0);

    if (ret > 0) {
        sched_poke_cpus(cpu_mask);
        if (resched_now) {
            sched_resched();
        }
    }

    return ret;
}

/**
 * @brief  Free all resources allocated in wait_queue_init()
 *
 * If any threads were waiting on this queue, they are all woken.
 */
void wait_queue_destroy(wait_queue_t *wait) {
    DEBUG_ASSERT(wait->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    wait_queue_wake_all(wait, ERR_OBJECT_DESTROYED);
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
status_t thread_unblock_from_wait_queue(thread_t *t, status_t wait_queue_error) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    if (t->state != THREAD_BLOCKED) {
        return ERR_NOT_BLOCKED;
    }

    DEBUG_ASSERT(t->blocking_wait_queue != NULL);
    DEBUG_ASSERT(t->blocking_wait_queue->magic == WAIT_QUEUE_MAGIC);
    DEBUG_ASSERT(list_in_list(&t->queue_node));

    list_delete(&t->queue_node);
    t->blocking_wait_queue->count--;
    t->blocking_wait_queue = NULL;
    t->state = THREAD_READY;
    t->wait_queue_block_ret = wait_queue_error;
    sched_poke_cpus(1U << sched_insert_runnable(t));

    return NO_ERROR;
}
