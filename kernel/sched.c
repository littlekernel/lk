/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/**
 * @file
 * @brief  Kernel scheduler
 *
 * Per-cpu run queues, the choice of which cpu a runnable thread lands on, the
 * idle threads, and the context switch itself. Split out of thread.c, which
 * keeps the thread lifecycle: create, exit, join, detach, and the debug dumps.
 *
 * @defgroup thread Threads
 * @{
 */
#include <kernel/sched.h>

#include <assert.h>
#include <arch/ops.h>
#include <kernel/debug.h>
#include <kernel/mp.h>
#include <kernel/preempt.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>
#include <lk/trace.h>
#include <platform.h>
#include <printf.h>
#include <string.h>
#include <target.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#define STACK_DEBUG_WORD (0x99999999)

#define DEBUG_THREAD_CONTEXT_SWITCH 0

/* Per-cpu scheduler state.
 *
 * Each cpu has its own run queue and only ever pulls work off of its own. Which
 * cpu a thread lands on is decided once, at the point it becomes runnable, by
 * find_target_cpu(); nothing rebalances it afterwards and no cpu steals from
 * another. That makes initial placement the whole scheduling policy, so a bug
 * there is a thread that never runs rather than a thread that runs late.
 *
 * This is all still covered by the global thread_lock. Splitting the queues
 * buys affinity and stops threads bouncing between cpus; splitting the lock is
 * a separate change.
 */
struct percpu_sched {
    struct list_node run_queue[NUM_PRIORITIES];
    uint32_t run_queue_bitmap;
    uint runnable_count;
#if PLATFORM_HAS_DYNAMIC_TIMER
    /* preemption timer */
    timer_t preempt_timer;
#endif
} __CPU_ALIGN;

static struct percpu_sched percpu_sched[SMP_MAX_CPUS];

/* make sure the bitmap is large enough to cover our number of priorities */
STATIC_ASSERT(NUM_PRIORITIES <= sizeof(uint32_t) * 8);

/* the idle thread(s) (statically allocated) */
#if WITH_SMP
static thread_t _idle_threads[SMP_MAX_CPUS];
#define idle_thread(cpu) (&_idle_threads[cpu])
#else
static thread_t _idle_thread;
#define idle_thread(cpu) (&_idle_thread)
#endif

/* run queue manipulation */
static void run_queue_insert_checks(uint cpu, thread_t *t) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(t->state == THREAD_READY);
    DEBUG_ASSERT(!list_in_list(&t->queue_node));
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));
    DEBUG_ASSERT(cpu < SMP_MAX_CPUS);
    /* pinning is enforced here, at insert time, so that the cpu pulling threads
     * off of its own queue never has to filter */
    DEBUG_ASSERT(thread_pinned_cpu(t) < 0 || thread_pinned_cpu(t) == (int)cpu);
}

void sched_insert_runnable_head_on(uint cpu, thread_t *t) {
    run_queue_insert_checks(cpu, t);

    struct percpu_sched *s = &percpu_sched[cpu];
    list_add_head(&s->run_queue[t->priority], &t->queue_node);
    s->run_queue_bitmap |= (1 << t->priority);
    s->runnable_count++;
    thread_set_last_cpu(t, (int)cpu);
}

void sched_insert_runnable_tail_on(uint cpu, thread_t *t) {
    run_queue_insert_checks(cpu, t);

    struct percpu_sched *s = &percpu_sched[cpu];
    list_add_tail(&s->run_queue[t->priority], &t->queue_node);
    s->run_queue_bitmap |= (1 << t->priority);
    s->runnable_count++;
    thread_set_last_cpu(t, (int)cpu);
}

/* Confirm a thread really is queued on the cpu the caller thinks it is.
 *
 * list_in_list() cannot tell one list from another, so it happily passes a
 * thread queued on some other cpu. That matters because list_delete() would
 * then still unlink it correctly -- the node knows its own list -- while the
 * bookkeeping around it decremented the wrong cpu's runnable_count and cleared
 * the wrong run queue bitmap bit. Silent cross-cpu corruption, so check for
 * real rather than approximately.
 */
static bool thread_is_queued_on(uint cpu, thread_t *t) {
    struct list_node *node;
    list_for_every(&percpu_sched[cpu].run_queue[t->priority], node) {
        if (node == &t->queue_node) {
            return true;
        }
    }
    return false;
}

/* Take a runnable thread back off of the run queue it is sitting on. Only used
 * to move a thread between cpus; the scheduler itself pops via get_top_thread().
 */
static void run_queue_remove(uint cpu, thread_t *t) {
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));
    DEBUG_ASSERT(cpu < SMP_MAX_CPUS);
    DEBUG_ASSERT(t->state == THREAD_READY);
    DEBUG_ASSERT(list_in_list(&t->queue_node));
    /* the caller derives `cpu` from last_cpu; this is what makes that safe */
    DEBUG_ASSERT(thread_is_queued_on(cpu, t));

    struct percpu_sched *s = &percpu_sched[cpu];
    list_delete(&t->queue_node);
    if (list_is_empty(&s->run_queue[t->priority])) {
        s->run_queue_bitmap &= ~(1 << t->priority);
    }
    s->runnable_count--;
}

/* Pick the cpu with the shortest run queue out of `mask`, breaking ties towards
 * the local cpu, whose cache we are already warm in. `mask` is never empty.
 */
static uint least_loaded_cpu(mp_cpu_mask_t mask, uint local_cpu) {
    uint best = local_cpu;
    uint best_count = UINT32_MAX;

    if (mask & (1U << local_cpu)) {
        best_count = percpu_sched[local_cpu].runnable_count;
    }
    for (mp_cpu_mask_t m = mask; m != 0; m &= m - 1) {
        const uint c = (uint)__builtin_ctz(m);
        if (percpu_sched[c].runnable_count < best_count) {
            best_count = percpu_sched[c].runnable_count;
            best = c;
        }
    }
    return best;
}

/* Pick the cpu a newly runnable thread should be steered at.
 *
 * The policy is deliberately dumb: prefer the cpu the thread last ran on if it
 * is genuinely free, else the least loaded idle cpu, else the least loaded cpu.
 * There is no load balancing and nothing rebalances a thread after the fact, so
 * this decision is the whole of the scheduling policy.
 *
 * Only cpus that have come up are candidates. A cpu that has not yet marked
 * itself active cannot be signalled at all -- mp_reschedule() masks it out of
 * the target set, and mp_mbx_reschedule_irq() would ignore the ipi even if one
 * arrived -- so steering a thread at one would strand it. This is not a
 * theoretical window: an SMP build booted with fewer cpus than SMP_MAX_CPUS
 * leaves those bits clear forever.
 *
 * Cpus running realtime threads are avoided when there is any alternative.
 */
static uint find_target_cpu(thread_t *t) {
#if WITH_SMP
    const uint local_cpu = arch_curr_cpu_num();

    const int pinned_cpu = thread_pinned_cpu(t);
    if (pinned_cpu >= 0) {
        /* no choice to make, and the caller is obligated to deliver the ipi */
        return (uint)pinned_cpu;
    }

    mp_cpu_mask_t candidates = mp_get_active_mask();
    if (unlikely(candidates == 0)) {
        /* early boot: no cpu has finished coming up yet, including this one */
        return local_cpu;
    }
    if ((candidates & ~mp_get_realtime_mask()) != 0) {
        candidates &= ~mp_get_realtime_mask();
    }

    /* An idle cpu is only *free* if nothing is queued on it yet.
     *
     * mp.idle_cpus does not clear until the target cpu actually context
     * switches, which needs the ipi delivered and taken -- a long time. So
     * within a burst of wakeups every cpu we just handed work to still looks
     * idle, and picking by idleness alone piles the whole burst onto whichever
     * cpu the mask happens to name first. runnable_count is updated at enqueue,
     * so it sees that work immediately and the choice self-corrects: hand a cpu
     * one thread and the next pick moves on.
     */
    const mp_cpu_mask_t idle = candidates & mp_get_idle_mask();
    const int last_cpu = thread_last_cpu(t);

    /* warm and genuinely free is the best of both */
    if (last_cpu >= 0 && (idle & (1U << last_cpu)) &&
        percpu_sched[last_cpu].runnable_count == 0) {
        return (uint)last_cpu;
    }

    /* an idle cpu if there is one, otherwise spread over everybody */
    return least_loaded_cpu(idle != 0 ? idle : candidates, local_cpu);
#else
    return 0;
#endif
}

/* Steer a newly runnable thread at a cpu and put it on that cpu's run queue.
 * Returns the target so the caller can poke it -- separately, so that a caller
 * waking a run of threads can batch the ipis into one call.
 */
uint sched_insert_runnable(thread_t *t) {
    const uint target = find_target_cpu(t);
    sched_insert_runnable_head_on(target, t);
    return target;
}

/* Make the cpu a thread was just steered at notice it.
 *
 * MP_RESCHEDULE_FLAG_REALTIME is deliberate: find_target_cpu() already avoids
 * cpus running realtime threads whenever it has a choice, so by the time we get
 * here the target is either not realtime or is the only cpu this thread can run
 * on. Letting mp_reschedule() filter it out at that point would drop the wakeup
 * on the floor, and with per-cpu run queues nobody else will pick the thread up.
 *
 * mp_reschedule() masks out the local cpu itself; a thread steered at the local
 * cpu is picked up by the caller's own reschedule, or by the pending-preempt
 * flag if preemption is disabled.
 */
void sched_poke_cpus(mp_cpu_mask_t targets) {
    mp_reschedule(targets, MP_RESCHEDULE_FLAG_REALTIME);
}

/**
 * @brief Flag a thread as real time
 *
 * @param t Thread to flag
 *
 * @return NO_ERROR on success
 */
status_t thread_set_real_time(thread_t *t) {
    if (!t) {
        return ERR_INVALID_ARGS;
    }

    DEBUG_ASSERT(t->magic == THREAD_MAGIC);

    THREAD_LOCK(state);
#if PLATFORM_HAS_DYNAMIC_TIMER
    if (t == get_current_thread()) {
        /* if we're currently running, cancel the preemption timer. */
        timer_cancel(&percpu_sched[arch_curr_cpu_num()].preempt_timer);
    }
#endif
    t->flags |= THREAD_FLAG_REAL_TIME;
    THREAD_UNLOCK(state);

    return NO_ERROR;
}

#if WITH_SMP
/**
 * @brief  Pin a thread to a cpu, or -1 to unpin it
 *
 * A thread that is already runnable and sitting on the wrong cpu's run queue is
 * moved to the right one and that cpu is poked. A blocked or suspended thread
 * just records the pin; find_target_cpu() honors it when the thread next wakes.
 */
void thread_set_pinned_cpu(thread_t *t, int cpu) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(cpu >= -1 && cpu < (int)SMP_MAX_CPUS);

    THREAD_LOCK(state);

    t->pinned_cpu = cpu;

    if (t->state == THREAD_READY) {
        /* the thread is queued on a cpu that may no longer be allowed to run
         * it. nothing filters at pop time any more, so move it now. */
        const uint queued_cpu = (uint)thread_last_cpu(t);
        if (cpu >= 0 && (uint)cpu != queued_cpu) {
            run_queue_remove(queued_cpu, t);
            sched_poke_cpus(1U << sched_insert_runnable(t));
        }
    } else if (t->state == THREAD_RUNNING) {
        /* Moving a running thread means asking the cpu running it to reschedule
         * and give it up, which nothing needs. Pinning it where it already runs
         * is the only supported case -- and the only one anything does. */
        DEBUG_ASSERT(cpu < 0 || cpu == thread_curr_cpu(t));
    }

    THREAD_UNLOCK(state);
}
#endif

static bool thread_is_realtime(thread_t *t) {
    return (t->flags & THREAD_FLAG_REAL_TIME) && t->priority > DEFAULT_PRIORITY;
}

static bool thread_is_real_time_or_idle(thread_t *t) {
    return !!(t->flags & (THREAD_FLAG_REAL_TIME | THREAD_FLAG_IDLE));
}

void sched_idle_routine(void) {
    for (;;) {
        arch_idle();
    }
}

static thread_t *get_top_thread(uint cpu) {
    struct percpu_sched *s = &percpu_sched[cpu];

    if (s->run_queue_bitmap) {
        /* find the highest priority queue with a thread in it. everything on
         * this cpu's queues is runnable here -- pinning was enforced at insert
         * time -- so there is no filtering to do, just take the head. */
        const uint next_queue = sizeof(s->run_queue_bitmap) * 8 - 1 -
                                __builtin_clz(s->run_queue_bitmap);

        thread_t *newthread = list_remove_head_type(&s->run_queue[next_queue], thread_t, queue_node);
        DEBUG_ASSERT(newthread);
        /* a stranded thread is otherwise invisible until something else times
         * out, so catch a bad find_target_cpu() decision right here */
        DEBUG_ASSERT(thread_pinned_cpu(newthread) < 0 ||
                     thread_pinned_cpu(newthread) == (int)cpu);

        if (list_is_empty(&s->run_queue[next_queue])) {
            s->run_queue_bitmap &= ~(1 << next_queue);
        }
        s->runnable_count--;

        return newthread;
    }

    /* no threads to run, select the idle thread for this cpu */
    return idle_thread(cpu);
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
void sched_resched(void) {
    thread_t *oldthread;
    thread_t *newthread;

    thread_t *current_thread = get_current_thread();
    uint cpu = arch_curr_cpu_num();

    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));
    DEBUG_ASSERT(current_thread->state != THREAD_RUNNING);

    THREAD_STATS_INC(reschedules);

    newthread = get_top_thread(cpu);

    DEBUG_ASSERT(newthread);

    newthread->state = THREAD_RUNNING;

    oldthread = current_thread;

    if (newthread == oldthread) {
        return;
    }

    /* set up quantum for the new thread if it was consumed */
    if (newthread->remaining_quantum <= 0) {
        newthread->remaining_quantum = 5; // XXX make this smarter
    }

    /* mark the cpu ownership of the threads. last_cpu outlives curr_cpu and is
     * what find_target_cpu() uses to keep a thread near where it last ran. */
    thread_set_curr_cpu(oldthread, -1);
    thread_set_curr_cpu(newthread, cpu);
    thread_set_last_cpu(newthread, cpu);

#if WITH_SMP
    if (thread_is_idle(newthread)) {
        mp_set_cpu_idle(cpu);
    } else {
        mp_set_cpu_busy(cpu);
    }

    if (thread_is_realtime(newthread)) {
        mp_set_cpu_realtime(cpu);
    } else {
        mp_set_cpu_non_realtime(cpu);
    }
#endif

#if THREAD_STATS
    THREAD_STATS_INC(context_switches);

    lk_bigtime_t now = current_time_hires();
    if (thread_is_idle(oldthread)) {
        thread_stats[cpu].idle_time += now - thread_stats[cpu].last_idle_timestamp;
    } else {
        oldthread->stats.total_run_time += now - oldthread->stats.last_run_timestamp;
    }
    if (thread_is_idle(newthread)) {
        thread_stats[cpu].last_idle_timestamp = now;
    } else {
        newthread->stats.last_run_timestamp = now;
        newthread->stats.schedules++;
    }
#endif

    KEVLOG_THREAD_SWITCH(oldthread, newthread);

#if PLATFORM_HAS_DYNAMIC_TIMER
    if (thread_is_real_time_or_idle(newthread)) {
        if (!thread_is_real_time_or_idle(oldthread)) {
            /* if we're switching from a non real time to a real time, cancel
             * the preemption timer. */
#if DEBUG_THREAD_CONTEXT_SWITCH
            dprintf(ALWAYS, "arch_context_switch: stop preempt, cpu %d, old %p (%s), new %p (%s)\n",
                    cpu, oldthread, oldthread->name, newthread, newthread->name);
#endif
            timer_cancel(&percpu_sched[cpu].preempt_timer);
        }
    } else if (thread_is_real_time_or_idle(oldthread)) {
        /* if we're switching from a real time (or idle thread) to a regular one,
         * set up a periodic timer to run our preemption tick. */
#if DEBUG_THREAD_CONTEXT_SWITCH
        dprintf(ALWAYS, "arch_context_switch: start preempt, cpu %d, old %p (%s), new %p (%s)\n",
                cpu, oldthread, oldthread->name, newthread, newthread->name);
#endif
        timer_set_periodic(&percpu_sched[cpu].preempt_timer, 10, thread_timer_tick, NULL);
    }
#endif

    /* set some optional target debug leds */
    target_set_debug_led(0, !thread_is_idle(newthread));

    /* do the switch */
    set_current_thread(newthread);

#if DEBUG_THREAD_CONTEXT_SWITCH
    dprintf(ALWAYS, "arch_context_switch: cpu %d, old %p (%s, pri %d, flags 0x%x), new %p (%s, pri %d, flags 0x%x)\n",
            cpu, oldthread, oldthread->name, oldthread->priority,
            oldthread->flags, newthread, newthread->name,
            newthread->priority, newthread->flags);
#endif

#if THREAD_STACK_BOUNDS_CHECK
    /* check that the old thread has not blown its stack just before pushing its context */
    if (oldthread->flags & THREAD_FLAG_DEBUG_STACK_BOUNDS_CHECK) {
        STATIC_ASSERT((THREAD_STACK_PADDING_SIZE % sizeof(uint32_t)) == 0);
        uint32_t *s = (uint32_t *)oldthread->stack;
        for (size_t i = 0; i < THREAD_STACK_PADDING_SIZE / sizeof(uint32_t); i++) {
            if (unlikely(s[i] != STACK_DEBUG_WORD)) {
                /* NOTE: will probably blow the stack harder here, but hopefully enough
                 * state exists to at least get some sort of debugging done.
                 */
                panic("stack overrun at %p: thread %p (%s), stack %p\n", &s[i],
                      oldthread, oldthread->name, oldthread->stack);
            }
        }
    }
#endif

#ifdef WITH_LIB_UTHREAD
    uthread_context_switch(oldthread, newthread);
#endif

#if WITH_KERNEL_VM
    /* see if we need to swap mmu context */
    if (newthread->aspace != oldthread->aspace) {
        vmm_context_switch(oldthread->aspace, newthread->aspace);
    }
#endif

    /* do the low level context switch */
    arch_context_switch(oldthread, newthread);
}

/**
 * @brief Yield the cpu to another thread
 *
 * This function places the current thread at the end of the run queue
 * and yields the cpu to another waiting thread, while also giving up
 * the remainder of its time slice.
 *
 * This function will return at some later time. Possibly immediately if
 * no other threads are waiting to execute.
 */
void thread_yield(void) {
    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(current_thread->magic == THREAD_MAGIC);
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);
    DEBUG_ASSERT(!thread_is_idle(current_thread));

    THREAD_LOCK(state);

    THREAD_STATS_INC(yields);

    /* we are yielding the cpu, so stick ourselves into the tail of the run queue and reschedule */
    current_thread->state = THREAD_READY;
    current_thread->remaining_quantum = 0;
    sched_insert_runnable_tail_on(arch_curr_cpu_num(), current_thread);
    sched_resched();

    THREAD_UNLOCK(state);
}

/**
 * @brief Reschedule if the current cpu is being preempted
 *
 * This function is similar to thread_yield(), except that the current thread
 * does not go to the end of the run queue unless its time slice has expired.
 *
 * This is most likely to be called at the end of an interrupt handler if the
 * scheduler has determined that a reschedule is needed.
 *
 * This function will return at some later time. Possibly immediately if
 * no other threads are waiting to execute.
 */
void thread_preempt(void) {
    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(current_thread->magic == THREAD_MAGIC);
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);

#if THREAD_STATS
    if (!thread_is_idle(current_thread)) {
        THREAD_STATS_INC(preempts); /* only track when a meaningful preempt happens */
    }
#endif

    KEVLOG_THREAD_PREEMPT(current_thread);

    THREAD_LOCK(state);

    /* we are being preempted, so we get to go back into the front of the run queue if we have quantum left */
    current_thread->state = THREAD_READY;
    if (likely(!thread_is_idle(current_thread))) { /* idle thread doesn't go in the run queue */
        if (current_thread->remaining_quantum > 0) {
            sched_insert_runnable_head_on(arch_curr_cpu_num(), current_thread);
        } else {
            sched_insert_runnable_tail_on(arch_curr_cpu_num(), current_thread); /* if we're out of quantum, go to the tail of the queue */
        }
    }
    sched_resched();

    THREAD_UNLOCK(state);
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
void thread_block(void) {
    __UNUSED thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(current_thread->magic == THREAD_MAGIC);
    DEBUG_ASSERT(current_thread->state == THREAD_BLOCKED);
    DEBUG_ASSERT(spin_lock_held(&thread_lock));
    DEBUG_ASSERT(!thread_is_idle(current_thread));

    /* we are blocking on something. the blocking code should have already stuck us on a queue */
    sched_resched();
}

/**
 * @brief  Make a blocked thread runnable again.
 *
 * This function makes a previously blocked thread runnable again by placing
 * it at the head of the run queue.
 *
 * @param t         Thread to unblock
 */
void thread_unblock(thread_t *t) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(t->state == THREAD_BLOCKED);
    DEBUG_ASSERT(spin_lock_held(&thread_lock));
    DEBUG_ASSERT(!thread_is_idle(t));

    t->state = THREAD_READY;
    sched_poke_cpus(1U << sched_insert_runnable(t));

    //if (resched) {
        if (!preempt_set_pending_if_disabled()) {
            sched_resched();
        }
    //}
}

enum handler_return thread_timer_tick(struct timer *t, lk_time_t now, void *arg) {
    thread_t *current_thread = get_current_thread();

    if (thread_is_real_time_or_idle(current_thread)) {
        return INT_NO_RESCHEDULE;
    }

    current_thread->remaining_quantum--;
    if (current_thread->remaining_quantum <= 0) {
        return INT_RESCHEDULE;
    } else {
        return INT_NO_RESCHEDULE;
    }
}

/* timer callback to wake up a sleeping thread */
static enum handler_return thread_sleep_handler(timer_t *timer, lk_time_t now, void *arg) {
    thread_t *t = (thread_t *)arg;

    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(t->state == THREAD_SLEEPING);

    THREAD_LOCK(state);

    t->state = THREAD_READY;
    const uint target = sched_insert_runnable(t);
    /* the ipi goes out under the lock: arch_mp_send_ipi() asserts interrupts
     * are disabled */
    sched_poke_cpus(1U << target);

    THREAD_UNLOCK(state);

    if (target != arch_curr_cpu_num()) {
        /* the sleeper went to another cpu, which the ipi above has poked.
         * rescheduling here would not find it. */
        return INT_NO_RESCHEDULE;
    }

    if (preempt_set_pending_if_disabled()) {
        return INT_NO_RESCHEDULE;
    }
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
void thread_sleep(lk_time_t delay) {
    timer_t timer;

    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(current_thread->magic == THREAD_MAGIC);
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);
    DEBUG_ASSERT(!thread_is_idle(current_thread));

    timer_initialize(&timer);

    THREAD_LOCK(state);
    timer_set_oneshot(&timer, delay, thread_sleep_handler, (void *)current_thread);
    current_thread->state = THREAD_SLEEPING;
    sched_resched();
    THREAD_UNLOCK(state);
}

/**
 * @brief Change priority of current thread
 *
 * See thread_create() for a discussion of priority values.
 */
void thread_set_priority(int priority) {
    thread_t *current_thread = get_current_thread();

    THREAD_LOCK(state);

    if (priority <= IDLE_PRIORITY) {
        priority = IDLE_PRIORITY + 1;
    }
    if (priority > HIGHEST_PRIORITY) {
        priority = HIGHEST_PRIORITY;
    }
    current_thread->priority = priority;

    current_thread->state = THREAD_READY;
    sched_insert_runnable_head_on(arch_curr_cpu_num(), current_thread);
    sched_resched();

    THREAD_UNLOCK(state);
}

/**
 * @brief  Become an idle thread
 *
 * This function marks the current thread as the idle thread -- the one which
 * executes when there is nothing else to do.  This function does not return.
 * This function is called once at boot time.
 */
void thread_become_idle(void) {
    DEBUG_ASSERT(arch_ints_disabled());

    thread_t *t = get_current_thread();

#if WITH_SMP
    char name[16];
    snprintf(name, sizeof(name), "idle %u", arch_curr_cpu_num());
    thread_set_name(name);
#else
    thread_set_name("idle");
#endif

    /* mark ourself as idle */
    t->priority = IDLE_PRIORITY;
    t->flags |= THREAD_FLAG_IDLE;
    thread_set_pinned_cpu(t, arch_curr_cpu_num());

    mp_set_curr_cpu_active(true);
    mp_set_cpu_idle(arch_curr_cpu_num());

    /* enable interrupts and start the scheduler */
    arch_enable_ints();
    thread_preempt();

    sched_idle_routine();
}

/* Initialize the run queues. Runs before any thread can be made runnable,
 * including the bootstrap thread that thread_init_early() half constructs. */
void sched_init_early(void) {
    /* initialize every cpu's run queues, not just this one's: secondary cpus
     * come up long after threads start being steered at them */
    for (uint c = 0; c < SMP_MAX_CPUS; c++) {
        for (int i = 0; i < NUM_PRIORITIES; i++) {
            list_initialize(&percpu_sched[c].run_queue[i]);
        }
    }
}

void sched_init(void) {
#if PLATFORM_HAS_DYNAMIC_TIMER
    for (uint i = 0; i < SMP_MAX_CPUS; i++) {
        timer_initialize(&percpu_sched[i].preempt_timer);
    }
#endif
}

thread_t *sched_idle_thread(uint cpu) {
    return idle_thread(cpu);
}

/** @} */
