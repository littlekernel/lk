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

/* The thread list lock, taken as thread_list_lock() from kernel/thread_lock.h.
 * Once the one lock for all of threading; the sched and wait queue locks have
 * since been split out of it. */
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

    /* until it first runs, the thread belongs to the sched lock of the cpu
     * that created it (see thread_lock.h) */
    thread_set_last_cpu(t, arch_curr_cpu_num());

    /* add it to the global thread list */
    arch_interrupt_saved_state_t state = thread_list_lock_irqsave();
    list_add_head(&thread_list, &t->thread_list_node);
    thread_list_unlock_irqrestore(state);

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

    /* A suspended thread is on no queue and is owned by the sched lock of the
     * cpu that created it; the scheduler takes that together with the lock of
     * the cpu it picks, and does the SUSPENDED to READY transition under both.
     * Interrupts stay off until the ipi is sent: arch_mp_send_ipi() asserts
     * they are. */
    arch_interrupt_saved_state_t state = arch_interrupt_save();
    uint target;
    if (!sched_insert_runnable_from(t, THREAD_SUSPENDED, &target)) {
        arch_interrupt_restore(state);
        return ERR_NOT_SUSPENDED;
    }

    bool local_resched = false;
    if (!ints_disabled) { /* HACK, don't resched into bootstrap thread before idle thread is set up */
        local_resched = true;
    }

    sched_poke_cpus(1U << target);

    arch_interrupt_restore(state);

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

    /* The detached flag and the exit handshake are protected by the lock of
     * the thread's retcode wait queue: thread_detach() sets the flag, and
     * thread_exit() marks the thread dead, under that lock and just before
     * waking this queue, so a joiner that sees either under the same lock sees
     * the state it is being told about.
     */
    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&t->retcode_wait_queue);

    if (t->flags & THREAD_FLAG_DETACHED) {
        /* the thread is detached, go ahead and exit */
        wait_queue_unlock_irqrestore(&t->retcode_wait_queue, state);
        return ERR_THREAD_DETACHED;
    }

    /* wait for the thread to die. The block releases the wait queue lock, and
     * this is the one path where that matters: a joiner woken with
     * ERR_THREAD_DETACHED must not touch |t| again, because the detached
     * thread may have exited and been freed by the time the joiner runs. */
    if (t->state != THREAD_DEATH) {
        status_t err = wait_queue_block(&t->retcode_wait_queue, timeout);
        if (err < 0) {
            arch_interrupt_restore(state);
            return err;
        }
    } else {
        spin_unlock(wait_queue_lock(&t->retcode_wait_queue));
    }

    /* No lock held. The thread is dead and stays that way, and its return
     * code was written before it woke us (or before we saw it dead above). */
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(t->state == THREAD_DEATH);
    DEBUG_ASSERT(t->blocking_wait_queue == NULL);
    DEBUG_ASSERT(!list_in_list(&t->run_queue_node));
    DEBUG_ASSERT(!list_in_list(&t->wait_queue_node));

    /* save the return code */
    if (retcode) {
        *retcode = t->retcode;
    }

    arch_interrupt_restore(state);

    /* Now reap it. The list lock ranks above the wait queue lock and the two
     * steps are independent, so the wait queue lock is dropped first rather
     * than the list lock nested inside it. */
    arch_interrupt_saved_state_t reap_state = thread_list_lock_irqsave();
    list_delete(&t->thread_list_node);
    thread_list_unlock_irqrestore(reap_state);

    /* The thread may still be on its way off the cpu it exited on: it marked
     * itself dead and woke us before switching away, and its stack is in use
     * until that switch completes. A dead thread is owned by the sched lock
     * of the cpu it last ran on, which thread_exit() takes after the wake and
     * hands across the switch. We can get there first, between the wake and
     * its taking the lock, so holding the lock is not enough on its own: wait
     * for curr_cpu to clear, which sched_context_switch_complete() does on the
     * far side of the switch, and which is the same thing a cpu handed a
     * still-running thread waits for in sched_resched(). The wait is done
     * without the lock: the exiter needs that very lock to switch out, and
     * taking and dropping it in a loop here starved it of it on arm64, at a
     * cost of a timer tick per join. curr_cpu is an atomic, so an unlocked
     * read is fine; the lock is taken once it reads -1, so that the thread is
     * retired under the lock that owns it like everything else. */
    const uint exit_cpu = (uint)thread_last_cpu(t);
#if WITH_SMP
    while (unlikely(__atomic_load_n(&t->curr_cpu, __ATOMIC_ACQUIRE) != -1)) {
        /* spin */
    }
#endif
    reap_state = spin_lock_irqsave(sched_lock(exit_cpu));
    t->magic = 0;
    spin_unlock_irqrestore(sched_lock(exit_cpu), reap_state);

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
     * locked region drops the wait queue lock, so a woken joiner would be free
     * to run thread_join() to completion and free |t| before we get back to
     * inspect it. Defer the reschedule past the unlock.
     */
    preempt_disable();

    /* The detached flag lives under the lock of the retcode wait queue: it is
     * what thread_join() reads it under, and the check for an already dead
     * thread below relies on thread_exit() marking the thread dead under this
     * lock before it wakes the queue. */
    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&t->retcode_wait_queue);

    /* if another thread is blocked inside thread_join() on this thread,
     * wake them up with a specific return code */
    wait_queue_wake_all(&t->retcode_wait_queue, ERR_THREAD_DETACHED);

    /* if it's already dead, then just do what join would have and exit */
    if (t->state == THREAD_DEATH) {
        thread_flags_clear(t, THREAD_FLAG_DETACHED); /* makes sure thread_join continues */
        wait_queue_unlock_irqrestore(&t->retcode_wait_queue, state);
        preempt_enable();
        return thread_join(t, NULL, 0);
    } else {
        thread_flags_set(t, THREAD_FLAG_DETACHED);
        wait_queue_unlock_irqrestore(&t->retcode_wait_queue, state);
        preempt_enable();
        return NO_ERROR;
    }
}

/* Free what a detached thread leaves behind. Called once nothing runs on its
 * stack: by the scheduler on the far side of the thread's final context
 * switch, with the sched lock held and interrupts off, so the frees are
 * delayed ones. The thread is already off the thread list. */
void thread_reap_detached(thread_t *t) {
    DEBUG_ASSERT(t->state == THREAD_DEATH);
    DEBUG_ASSERT(t->flags & THREAD_FLAG_DETACHED);
    DEBUG_ASSERT(!list_in_list(&t->thread_list_node));

    t->magic = 0;

    if (t->flags & THREAD_FLAG_FREE_STACK && t->stack) {
        heap_delayed_free(t->stack);

        /* the delayed free list node now sits in the stack's guard words.
         * Where this runs before the final switch (cortex-m), that switch
         * would otherwise see it as an overrun. */
        thread_flags_clear(t, THREAD_FLAG_DEBUG_STACK_BOUNDS_CHECK);
    }
    if (t->flags & THREAD_FLAG_FREE_STRUCT) {
        heap_delayed_free(t);
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

    /* The thread is marked dead under the retcode wait queue's lock, which is
     * what thread_join() and thread_detach() read it under, and before the
     * joiners are woken. The detached flag is read under the same lock. */
    arch_interrupt_saved_state_t state = wait_queue_lock_irqsave(&current_thread->retcode_wait_queue);
    (void)state; /* never restored: this thread does not come back */

    current_thread->state = THREAD_DEATH;
    current_thread->retcode = retcode;
    const bool detached = current_thread->flags & THREAD_FLAG_DETACHED;

    if (!detached) {
        /* Signal if anyone is waiting. This must not reschedule from inside the
         * wake: doing so marks the current thread READY and puts it back on the
         * run queue to be resumed later, undoing the THREAD_DEATH set above. We
         * are about to reschedule explicitly and never come back, so drop the
         * pending reschedule on the floor rather than acting on it.
         */
        preempt_disable();
        wait_queue_wake_all(&current_thread->retcode_wait_queue, 0);
        (void)preempt_enable_no_resched();

        /* The joiner frees our stack, and must not do so while we are still
         * on it. It waits on the local sched lock, which is taken here, before
         * the wait queue lock it saw us die under is released, and is handed
         * across the switch: nobody gets it until the switch is complete. */
        spin_lock(sched_lock(arch_curr_cpu_num()));
        spin_unlock(wait_queue_lock(&current_thread->retcode_wait_queue));
    } else {
        /* Nobody will join us. Leave the list now; the list lock is a
         * different rank from the wait queue lock, so it is taken after it is
         * dropped, never inside it. The stack and struct are freed on the far
         * side of the switch, by thread_reap_detached(), once we are off them
         * -- except where the arch hands nothing across the switch, in which
         * case the delayed free is queued here and is safe because that arch
         * has only the one cpu to run the freer on, after we are gone. */
        spin_unlock(wait_queue_lock(&current_thread->retcode_wait_queue));

        spin_lock(thread_list_lock());
        list_delete(&current_thread->thread_list_node);
        spin_unlock(thread_list_lock());

#if ARCH_CONTEXT_SWITCH_DROPS_LOCK
        thread_reap_detached(current_thread);
#endif

        spin_lock(sched_lock(arch_curr_cpu_num()));
    }

    /* reschedule, handing the local sched lock to whatever runs next */
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
    thread_set_last_cpu(t, 0);
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
    thread_set_last_cpu(t, cpu);
    thread_init_pinned_cpu(t, cpu);
    wait_queue_init(&t->retcode_wait_queue);

    arch_interrupt_saved_state_t state = thread_list_lock_irqsave();
    list_add_head(&thread_list, &t->thread_list_node);
    thread_list_unlock_irqrestore(state);
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
    arch_interrupt_saved_state_t state = thread_list_lock_irqsave();
    dump_all_threads_unlocked();
    thread_list_unlock_irqrestore(state);
}

#if THREAD_STATS
void dump_threads_stats(void) {
    thread_t *t;

    arch_interrupt_saved_state_t state = thread_list_lock_irqsave();
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
    thread_list_unlock_irqrestore(state);
}
#endif

/** @} */
