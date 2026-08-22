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

/* On every arch but one the local sched lock is held across arch_context_switch()
 * and handed to the incoming thread, which releases it: either by returning from
 * its own sched_resched() and unlocking as usual, or, for a thread running for
 * the first time, via sched_initial_thread_entry(). The exception is cortex-m,
 * which drops the lock before the switch (see its arch_thread.h). */
#ifndef ARCH_CONTEXT_SWITCH_DROPS_LOCK
#define ARCH_CONTEXT_SWITCH_DROPS_LOCK 0
#endif

/* Per-cpu scheduler state.
 *
 * Each cpu has its own run queue and only ever pulls work off of its own. Which
 * cpu a thread lands on is decided once, at the point it becomes runnable, by
 * find_target_cpu(); nothing rebalances it afterwards and no cpu steals from
 * another. That makes initial placement the whole scheduling policy, so a bug
 * there is a thread that never runs rather than a thread that runs late.
 *
 * All of this is protected by sched_lock(cpu), one lock per cpu, which lives
 * in its own cache line in sched_lock_slots[] (see kernel/thread_lock.h for
 * the lock order and for which lock owns a thread in which state).
 */
struct percpu_sched {
    struct list_node run_queue[NUM_PRIORITIES];
    uint32_t run_queue_bitmap;
    uint runnable_count;
#if !ARCH_CONTEXT_SWITCH_DROPS_LOCK
    /* The thread this cpu is in the middle of switching away from. Set by
     * sched_resched() just before arch_context_switch() and consumed by
     * sched_context_switch_complete() on the incoming side, so the incoming
     * thread can finish off the outgoing one once its context is safely saved. */
    thread_t *previous_thread;
#endif
#if PLATFORM_HAS_DYNAMIC_TIMER
    /* preemption timer */
    timer_t preempt_timer;
#endif
} __CPU_ALIGN;

static struct percpu_sched percpu_sched[SMP_MAX_CPUS];

struct sched_lock_slot sched_lock_slots[SMP_MAX_CPUS];

/* Take two cpus' sched locks. Two locks of the same rank need an order, and
 * the order is by cpu number, lowest first. The same cpu twice is one lock. */
static void sched_lock_pair(uint a, uint b) {
    DEBUG_ASSERT(arch_ints_disabled());
    if (a == b) {
        spin_lock(sched_lock(a));
    } else if (a < b) {
        spin_lock(sched_lock(a));
        spin_lock(sched_lock(b));
    } else {
        spin_lock(sched_lock(b));
        spin_lock(sched_lock(a));
    }
}

static void sched_unlock_pair(uint a, uint b) {
    spin_unlock(sched_lock(a));
    if (a != b) {
        spin_unlock(sched_lock(b));
    }
}

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
    DEBUG_ASSERT(!list_in_list(&t->run_queue_node));
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(sched_lock_held(cpu));
    DEBUG_ASSERT(cpu < SMP_MAX_CPUS);
    /* pinning is enforced here, at insert time, so that the cpu pulling threads
     * off of its own queue never has to filter */
    DEBUG_ASSERT(thread_pinned_cpu(t) < 0 || thread_pinned_cpu(t) == (int)cpu);
}

void sched_insert_runnable_head_on(uint cpu, thread_t *t) {
    run_queue_insert_checks(cpu, t);

    struct percpu_sched *s = &percpu_sched[cpu];
    list_add_head(&s->run_queue[t->priority], &t->run_queue_node);
    s->run_queue_bitmap |= (1 << t->priority);
    s->runnable_count++;
    thread_set_last_cpu(t, (int)cpu);
}

void sched_insert_runnable_tail_on(uint cpu, thread_t *t) {
    run_queue_insert_checks(cpu, t);

    struct percpu_sched *s = &percpu_sched[cpu];
    list_add_tail(&s->run_queue[t->priority], &t->run_queue_node);
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
        if (node == &t->run_queue_node) {
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
    DEBUG_ASSERT(sched_lock_held(cpu));
    DEBUG_ASSERT(cpu < SMP_MAX_CPUS);
    DEBUG_ASSERT(t->state == THREAD_READY);
    DEBUG_ASSERT(list_in_list(&t->run_queue_node));
    /* the caller derives `cpu` from last_cpu; this is what makes that safe */
    DEBUG_ASSERT(thread_is_queued_on(cpu, t));

    struct percpu_sched *s = &percpu_sched[cpu];
    list_delete(&t->run_queue_node);
    if (list_is_empty(&s->run_queue[t->priority])) {
        s->run_queue_bitmap &= ~(1 << t->priority);
    }
    s->runnable_count--;
}

/* A cpu's queue depth, read without its lock. find_target_cpu() runs unlocked
 * so that it does not have to touch every cpu's lock to choose one, and a
 * count that is a little stale only costs a slightly worse choice. */
static uint sched_runnable_count(uint cpu) {
    return __atomic_load_n(&percpu_sched[cpu].runnable_count, __ATOMIC_RELAXED);
}

/* Pick the cpu with the shortest run queue out of `mask`, breaking ties towards
 * the local cpu, whose cache we are already warm in. `mask` is never empty.
 */
static uint least_loaded_cpu(mp_cpu_mask_t mask, uint local_cpu) {
    uint best = local_cpu;
    uint best_count = UINT32_MAX;

    if (mask & (1U << local_cpu)) {
        best_count = sched_runnable_count(local_cpu);
    }
    for (mp_cpu_mask_t m = mask; m != 0; m &= m - 1) {
        const uint c = (uint)__builtin_ctz(m);
        const uint count = sched_runnable_count(c);
        if (count < best_count) {
            best_count = count;
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
 *
 * Runs with no sched lock held, and everything it reads is a hint: the caller
 * takes the chosen cpu's lock and then checks the choice is still legal with
 * sched_target_is_valid().
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
        sched_runnable_count(last_cpu) == 0) {
        return (uint)last_cpu;
    }

    /* an idle cpu if there is one, otherwise spread over everybody */
    return least_loaded_cpu(idle != 0 ? idle : candidates, local_cpu);
#else
    return 0;
#endif
}

/* Is a cpu chosen unlocked still a legal place for this thread, now that its
 * lock is held? The pin is the thing that can change in between; a cpu going
 * away cannot happen today but is cheap to cover. */
static bool sched_target_is_valid(uint target, thread_t *t) {
    const int pinned_cpu = thread_pinned_cpu(t);
    if (pinned_cpu >= 0) {
        return (uint)pinned_cpu == target;
    }
#if WITH_SMP
    const mp_cpu_mask_t active = mp_get_active_mask();
    return active == 0 || (active & (1U << target)) != 0 || target == arch_curr_cpu_num();
#else
    return true;
#endif
}

/* Make runnable a thread that is on no run queue -- blocked on a wait queue,
 * suspended, or asleep on a timer -- and so is owned by the sched lock of the
 * cpu it last ran on (or was created on). That lock and the target's are held
 * together across the state change, so nobody can see the thread between the
 * two: the pair is what makes the transition atomic, and it is also why the
 * target is rechecked after locking, since the pin could have moved before
 * that.
 *
 * Returns false, having changed nothing, if the thread is not in |from|: for
 * thread_resume() that is a second resume racing the first, for a wait queue
 * that is a waiter whose timeout got there first (or the reverse).
 *
 * |wait_ret| is the value wait_queue_block() returns to a woken thread. It is
 * written under the owner's lock and before the state, so whichever of a wake
 * and a timeout wins also gets to say what the thread sees. |dequeued| says
 * the caller has taken the thread off its wait queue's list, so that the
 * thread need not: blocking_wait_queue is cleared to tell it so, under the
 * owner's lock, which is what the thread reads it under. A timeout has not,
 * and leaves the pointer for the thread to follow (sched_leave_wait_queue()).
 * That also covers a waker finding a thread the timeout beat it to: the node
 * is gone either way, so the pointer is cleared either way.
 *
 * |local_held| is for the wake path, which already holds the local cpu's
 * sched lock with the current thread queued under it. The common case -- a
 * thread woken by the cpu it last ran on, going back there -- then needs no
 * further lock at all. Anything else drops the local lock for the pair and
 * takes it back after, which is fine: the current thread is queued but only
 * this cpu pops this queue, and interrupts are off.
 */
static bool sched_make_ready_from(thread_t *t, enum thread_state from, status_t wait_ret,
                                  bool dequeued, bool local_held, uint *target_out) {
    DEBUG_ASSERT(arch_ints_disabled());
    const uint local = arch_curr_cpu_num();

    for (;;) {
        const int owner = thread_last_cpu(t);
        DEBUG_ASSERT(owner >= 0);
        const uint target = find_target_cpu(t);
        const bool under_local = local_held && (uint)owner == local && target == local;

        if (!under_local) {
            if (local_held) {
                spin_unlock(sched_lock(local));
            }
            sched_lock_pair((uint)owner, target);
        }

        /* retry: the thread moved on while the locks were being taken, or the
         * pin did; done: it is no longer in |from|, or it is queued now */
        bool retry = false;
        bool done = false;
        if (unlikely(thread_last_cpu(t) != owner)) {
            /* somebody else moved it first; the lock we hold is no longer its */
            retry = true;
        } else if (t->state != from) {
            /* whoever got here first has already transitioned it */
            if (dequeued) {
                t->blocking_wait_queue = NULL;
            }
        } else if (unlikely(!sched_target_is_valid(target, t))) {
            /* the pin moved while the target was being chosen */
            retry = true;
        } else {
            if (from == THREAD_BLOCKED) {
                t->wait_queue_block_ret = wait_ret;
                if (dequeued) {
                    t->blocking_wait_queue = NULL;
                }
            }
            t->state = THREAD_READY;
            sched_insert_runnable_head_on(target, t);
            done = true;
        }

        if (!under_local) {
            sched_unlock_pair((uint)owner, target);
            if (local_held) {
                spin_lock(sched_lock(local));
            }
        }

        if (retry) {
            continue;
        }
        if (done) {
            *target_out = target;
        }
        return done;
    }
}

bool sched_insert_runnable_from(thread_t *t, enum thread_state from, uint *target_out) {
    DEBUG_ASSERT(from != THREAD_BLOCKED);
    return sched_make_ready_from(t, from, NO_ERROR, false, false, target_out);
}

bool sched_unblock(thread_t *t, status_t wait_ret, uint *target_out) {
    return sched_make_ready_from(t, THREAD_BLOCKED, wait_ret, false, false, target_out);
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

    /* The flag takes effect at the thread's next context switch, which is
     * where the scheduler reads it; the one thing to do now is stop the local
     * preemption timer if |t| is the thread running here, which needs the
     * local sched lock. A thread running elsewhere keeps its timer until that
     * cpu next switches. See thread_flags_set() for why the bit is not simply
     * or'd in. */
    arch_interrupt_saved_state_t state = sched_lock_local_irqsave();
    thread_flags_set(t, THREAD_FLAG_REAL_TIME);
#if PLATFORM_HAS_DYNAMIC_TIMER
    if (t == get_current_thread()) {
        timer_cancel(&percpu_sched[arch_curr_cpu_num()].preempt_timer);
    }
#endif
    sched_unlock_local_irqrestore(state);

    return NO_ERROR;
}

#if WITH_SMP
/**
 * @brief  Pin a thread to a cpu, or -1 to unpin it
 *
 * A thread that is already runnable and sitting on the wrong cpu's run queue is
 * moved to the right one and that cpu is poked. A thread running on the wrong
 * cpu is moved the next time that cpu reschedules, which is requested here if
 * it is another cpu; a thread repinning itself follows up with thread_yield().
 * A blocked or suspended thread just records the pin; find_target_cpu() honors
 * it when the thread next wakes.
 *
 * Whatever its state, the thread is owned by the sched lock of last_cpu (see
 * thread_lock.h for the ownership table), plus the lock of the cpu it is
 * being pinned to if a queued thread has to move. last_cpu can only be trusted
 * once that lock is held, so it is reread under it and the locks retaken if
 * the thread moved in between.
 */
void thread_set_pinned_cpu(thread_t *t, int cpu) {
    DEBUG_ASSERT(t->magic == THREAD_MAGIC);
    DEBUG_ASSERT(cpu >= -1 && cpu < (int)SMP_MAX_CPUS);

    arch_interrupt_saved_state_t state = arch_interrupt_save();

    for (;;) {
        const int owner = thread_last_cpu(t);
        DEBUG_ASSERT(owner >= 0);
        const uint second = (cpu >= 0) ? (uint)cpu : (uint)owner;
        sched_lock_pair((uint)owner, second);

        if (thread_last_cpu(t) != owner) {
            /* it was queued or run somewhere else in between */
            sched_unlock_pair((uint)owner, second);
            continue;
        }
        if (t->state == THREAD_READY && !list_in_list(&t->run_queue_node)) {
            /* In transit: the thread itself, between marking itself READY on
             * its way off the cpu and queueing itself under this lock. That
             * finishes promptly, with interrupts off, so wait it out. */
            sched_unlock_pair((uint)owner, second);
            continue;
        }

        t->pinned_cpu = cpu;

        if (t->state == THREAD_READY) {
            /* the thread is queued on a cpu that may no longer be allowed to run
             * it. nothing filters at pop time any more, so move it now. */
            if (cpu >= 0 && cpu != owner) {
                run_queue_remove((uint)owner, t);
                sched_insert_runnable_head_on((uint)cpu, t);
                sched_poke_cpus(1U << (uint)cpu);
            }
        } else if (t->state == THREAD_RUNNING && cpu >= 0 && cpu != thread_curr_cpu(t)) {
            /* It is running somewhere it is no longer allowed to be. The cpu running
             * it has to give it up: its preempt path requeues it through
             * sched_requeue_current(), which honors the pin. If that cpu is another
             * one, poke it. If it is this one the caller is the thread itself, and
             * moving it means a context switch the caller has to ask for --
             * thread_yield() is the idiom (see arch/x86/test). */
            if (t != get_current_thread()) {
                sched_poke_cpus(1U << (uint)thread_curr_cpu(t));
            }
        }

        sched_unlock_pair((uint)owner, second);
        break;
    }

    arch_interrupt_restore(state);
}
#endif

#if !ARCH_CONTEXT_SWITCH_DROPS_LOCK
/* The incoming side of a context switch. Runs on the thread that was just
 * switched to, with the local sched lock held -- handed over by the outgoing
 * thread rather than taken here -- and finishes off that outgoing thread.
 *
 * This is the point at which the previous thread is truly off this cpu: its
 * context is saved and nothing here will touch it again. Until now it was still
 * running here as far as any other cpu was concerned, whatever its state field
 * said, so this is where curr_cpu is cleared rather than before the switch.
 * A cpu that has been handed the thread waits for exactly this before running
 * it (see sched_resched()), and a dying thread's stack can only be freed after
 * it: a detached one is torn down right here, and a joined one by its joiner,
 * which takes this cpu's sched lock to get in line behind this point.
 */
static void sched_context_switch_complete(void) {
    struct percpu_sched *s = &percpu_sched[arch_curr_cpu_num()];

    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(sched_lock_local_held());

    thread_t *prev = s->previous_thread;
    DEBUG_ASSERT(prev != NULL);
    DEBUG_ASSERT(prev != get_current_thread());
    s->previous_thread = NULL;

#if WITH_SMP
    /* release: the saved context is published along with the cleared cpu */
    __atomic_store_n(&prev->curr_cpu, -1, __ATOMIC_RELEASE);
#endif

    if (unlikely(prev->state == THREAD_DEATH && (prev->flags & THREAD_FLAG_DETACHED))) {
        /* nobody will join it; free it now that we are off its stack */
        thread_reap_detached(prev);
    }
}

/* A thread running for the first time was switched to exactly like any other,
 * under the local sched lock, but has no sched_resched() frame to return into
 * and so nothing that would release it. The arch's initial_thread_func calls
 * this first, before enabling interrupts. */
void sched_initial_thread_entry(void) {
    DEBUG_ASSERT(arch_ints_disabled());

    sched_context_switch_complete();
    spin_unlock(sched_lock(arch_curr_cpu_num()));
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

        thread_t *newthread = list_remove_head_type(&s->run_queue[next_queue], thread_t, run_queue_node);
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
    DEBUG_ASSERT(sched_lock_local_held());
    DEBUG_ASSERT(current_thread->state != THREAD_RUNNING);

    THREAD_STATS_INC(reschedules);

    newthread = get_top_thread(cpu);

    DEBUG_ASSERT(newthread);

    newthread->state = THREAD_RUNNING;

    oldthread = current_thread;

    if (newthread == oldthread) {
        return;
    }

#if WITH_SMP && !ARCH_CONTEXT_SWITCH_DROPS_LOCK
    /* The thread may have been handed to this cpu while still running on
     * another: it goes READY and onto a queue before it switches out, and with
     * per-cpu locks nothing stops this cpu popping it first. Its context is
     * not saved until its old cpu's switch completes, which is when that cpu
     * clears curr_cpu (sched_context_switch_complete()). Wait for it. The old
     * cpu needs only its own lock to get there, so this cannot deadlock. */
    while (unlikely(__atomic_load_n(&newthread->curr_cpu, __ATOMIC_ACQUIRE) != -1)) {
        DEBUG_ASSERT(thread_curr_cpu(newthread) != (int)cpu);
    }
#endif

    /* set up quantum for the new thread if it was consumed */
    if (newthread->remaining_quantum <= 0) {
        newthread->remaining_quantum = 5; // XXX make this smarter
    }

    /* mark the cpu ownership of the threads. last_cpu outlives curr_cpu and is
     * what find_target_cpu() uses to keep a thread near where it last ran.
     * oldthread's curr_cpu is cleared on the far side of the switch, in
     * sched_context_switch_complete(), where the arch lets us. */
#if ARCH_CONTEXT_SWITCH_DROPS_LOCK
    thread_set_curr_cpu(oldthread, -1);
#endif
    thread_set_curr_cpu(newthread, cpu);
    thread_set_last_cpu(newthread, cpu);

#if WITH_SMP
    /* The mp masks live on one line shared by every cpu and each update is an
     * atomic read-modify-write, so only touch them when this switch actually
     * changes the cpu's status. Most switches do not: a busy cpu moving
     * between two regular threads would otherwise do four no-op RMWs on the
     * line every time, bouncing it between all of the cpus in the system.
     * The current status comes from the mask rather than from oldthread, as a
     * running thread's priority or flags can change under it without a switch
     * (thread_set_priority, thread_set_real_time); reading the mask keeps this
     * self-correcting the way the unconditional update was. Plain loads of a
     * line that is only written on a real transition stay shared and cheap. */
    const bool now_idle = thread_is_idle(newthread);
    if (mp_is_cpu_idle(cpu) != now_idle) {
        if (now_idle) {
            mp_set_cpu_idle(cpu);
        } else {
            mp_set_cpu_busy(cpu);
        }
    }

    const bool now_realtime = thread_is_realtime(newthread);
    if (mp_is_cpu_realtime(cpu) != now_realtime) {
        if (now_realtime) {
            mp_set_cpu_realtime(cpu);
        } else {
            mp_set_cpu_non_realtime(cpu);
        }
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

#if ARCH_CONTEXT_SWITCH_DROPS_LOCK
    /* the arch drops the lock around the switch itself and takes it back
     * before returning; nothing is handed off and there is nothing to finish */
    arch_context_switch(oldthread, newthread);
#else
    /* Do the low level context switch. The local sched lock goes with it: it
     * is not released here but handed to newthread, which drops it when it
     * resumes (or, for a brand new thread, in sched_initial_thread_entry()).
     * It is the lock of this cpu that is handed, whichever thread took it.
     *
     * Record who is being switched away from so the incoming thread can finish
     * the job. When arch_context_switch() returns we are oldthread again, on
     * whatever cpu picked us up, and the thread to finish is whoever switched
     * to us there -- not the newthread of this frame. */
    struct percpu_sched *s = &percpu_sched[cpu];
    DEBUG_ASSERT(s->previous_thread == NULL);
    s->previous_thread = oldthread;

    arch_context_switch(oldthread, newthread);

    sched_context_switch_complete();
#endif
}

/* Put the current thread, which is giving up the cpu, back on a run queue and
 * take the local sched lock for the reschedule that follows. Called with
 * interrupts disabled and no sched lock held; returns with the local one held.
 *
 * Normally the thread goes on the local cpu's queue, at the head if it still
 * has quantum and at the tail if it was out or yielded. But if it has been
 * pinned elsewhere since it was scheduled (thread_set_pinned_cpu() on a running
 * thread), the local queue is the one place it must not go: nothing filters at
 * pop time. Hand it to its cpu instead and poke that cpu. That takes both
 * cpus' locks, and the remote one is released again here: only the local lock
 * may be held across the switch. The other cpu can pop the thread the moment
 * its lock is dropped, but cannot run it until this cpu has switched off of it
 * -- sched_resched() waits for that.
 *
 * The pin is read before any lock is held, so it is reread after, and the
 * locks retaken if it moved. Once the local lock is held it cannot move: a
 * running thread is pinned under the sched lock of the cpu running it.
 */
static void sched_requeue_current(thread_t *current_thread, bool at_head) {
    const uint local = arch_curr_cpu_num();

    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);
    DEBUG_ASSERT(!thread_is_idle(current_thread));

    for (;;) {
        const int pinned = thread_pinned_cpu(current_thread);
        if (pinned < 0 || (uint)pinned == local) {
            spin_lock(sched_lock(local));
            if (unlikely(thread_pinned_cpu(current_thread) != pinned)) {
                spin_unlock(sched_lock(local));
                continue;
            }
            current_thread->state = THREAD_READY;
            if (at_head) {
                sched_insert_runnable_head_on(local, current_thread);
            } else {
                sched_insert_runnable_tail_on(local, current_thread);
            }
            return;
        }

        sched_lock_pair(local, (uint)pinned);
        if (unlikely(thread_pinned_cpu(current_thread) != pinned)) {
            sched_unlock_pair(local, (uint)pinned);
            continue;
        }
        current_thread->state = THREAD_READY;
        sched_insert_runnable_head_on((uint)pinned, current_thread);
        spin_unlock(sched_lock((uint)pinned));
        sched_poke_cpus(1U << (uint)pinned);
        return;
    }
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

    arch_interrupt_saved_state_t state = arch_interrupt_save();

    THREAD_STATS_INC(yields);

    /* we are yielding the cpu, so stick ourselves into the tail of the run queue and reschedule */
    current_thread->remaining_quantum = 0;
    sched_requeue_current(current_thread, false);
    sched_resched();

    sched_unlock_local_irqrestore(state);
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

    arch_interrupt_saved_state_t state = arch_interrupt_save();

    /* we are being preempted, so we get to go back into the front of the run queue if we have quantum left */
    if (likely(!thread_is_idle(current_thread))) { /* idle thread doesn't go in the run queue */
        /* back to the head if we have quantum left, the tail if we ran out */
        sched_requeue_current(current_thread, current_thread->remaining_quantum > 0);
    } else {
        spin_lock(sched_lock(arch_curr_cpu_num()));
        current_thread->state = THREAD_READY;
    }
    sched_resched();

    sched_unlock_local_irqrestore(state);
}

/* The timeout of a wait_queue_block(): wake the thread with ERR_TIMED_OUT. The
 * queue is nowhere in this. A blocked thread is owned by the sched lock of its
 * last cpu, not by its queue, so this is the same transition a waker makes
 * after taking the thread off the list, minus the list: the thread pulls its
 * own node off on the way out of wait_queue_block(), and a waker that finds
 * the node first skips it. Whichever of the two gets the sched lock first wins
 * and the other does nothing, which is how a wake and a timeout can race
 * without a lock that covers both.
 *
 * That is also why this never has to find the queue. The alternative -- walk
 * thread -> blocking_wait_queue -> lock -- has no way to know the queue is
 * still there to lock by the time it is reached.
 */
static enum handler_return wait_queue_timeout_handler(timer_t *timer, lk_time_t now, void *arg) {
    thread_t *t = (thread_t *)arg;

    DEBUG_ASSERT(t->magic == THREAD_MAGIC);

    uint target;
    if (!sched_unblock(t, ERR_TIMED_OUT, &target)) {
        /* woken in the meantime */
        return INT_NO_RESCHEDULE;
    }
    sched_poke_cpus(1U << target);

    if (target != arch_curr_cpu_num()) {
        /* it went to another cpu, which the ipi above has poked */
        return INT_NO_RESCHEDULE;
    }
    if (preempt_set_pending_if_disabled()) {
        return INT_NO_RESCHEDULE;
    }
    return INT_RESCHEDULE;
}

/* Block the current thread, which the wait queue code has already put on
 * |wq|'s list under |wq|'s lock, still held.
 *
 * The state change is owned by the local sched lock, like every other change
 * to a running thread's state, and is made under both locks: the wait queue
 * lock is what a waker pops the thread under, and it must find the thread
 * BLOCKED when it does or it will take the node and leave the thread behind.
 * The timeout timer is armed at the same point for the same reason. Then the
 * wait queue lock is dropped -- only the sched lock is held across a switch --
 * and the thread switches away.
 *
 * It returns here once woken, on whichever cpu picked it up, holding that cpu's
 * sched lock, which is dropped. No lock is held on return; interrupts are still
 * off. A thread woken by a waker never has to touch the wait queue again: the
 * waker took it off the list, and its return value is in wait_queue_block_ret.
 * One woken by its timeout is still on the list, which blocking_wait_queue
 * still set says -- read under the sched lock, which is where it is cleared --
 * and the caller follows up with sched_leave_wait_queue(). Returns whether it
 * has to.
 */
bool sched_block(struct wait_queue *wq, timer_t *timeout_timer, lk_time_t timeout) {
    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(wait_queue_lock_held(wq));
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);
    DEBUG_ASSERT(!thread_is_idle(current_thread));

    spin_lock(sched_lock(arch_curr_cpu_num()));
    current_thread->state = THREAD_BLOCKED;
    if (timeout_timer != NULL) {
        /* the timer lock is the innermost of the lot, so this nesting is the
         * right way around */
        timer_set_oneshot(timeout_timer, timeout, wait_queue_timeout_handler, current_thread);
    }
    spin_unlock(wait_queue_lock(wq));

    sched_resched();

    const bool still_queued = current_thread->blocking_wait_queue != NULL;
    spin_unlock(sched_lock(arch_curr_cpu_num()));
    return still_queued;
}

/* Take the current thread off the wait queue its timeout left it on.
 *
 * The queue's lock is needed to touch the list, and the queue may be on its
 * way out: a wait_queue_destroy() racing the timeout pops the stale node too,
 * and its caller then frees the memory. What makes following the pointer safe
 * is that a waker popping the node clears blocking_wait_queue under this
 * thread's sched lock, before its caller can get as far as freeing anything.
 * So the pointer is read under that lock, and while it is non-NULL the queue
 * is still there. The queue's lock ranks above the sched lock, though, and
 * that waker holds it while waiting for ours: it can only be tried, and if it
 * is busy, ours is dropped to let the waker in and the pointer reread.
 */
void sched_leave_wait_queue(void) {
    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);

    for (;;) {
        spin_lock(sched_lock(arch_curr_cpu_num()));
        struct wait_queue *wq = current_thread->blocking_wait_queue;
        if (wq == NULL) {
            /* a waker got to the node first */
            spin_unlock(sched_lock(arch_curr_cpu_num()));
            return;
        }
        if (spin_trylock(wait_queue_lock(wq)) == 0) {
            if (list_in_list(&current_thread->wait_queue_node)) {
                list_delete(&current_thread->wait_queue_node);
                wq->count--;
            }
            spin_unlock(wait_queue_lock(wq));
            current_thread->blocking_wait_queue = NULL;
            spin_unlock(sched_lock(arch_curr_cpu_num()));
            return;
        }
        spin_unlock(sched_lock(arch_curr_cpu_num()));
    }
}

/* Place a run of threads just taken off a wait queue, linked through their
 * wait_queue_node on |threads| (which is emptied), and tell their cpus. Each
 * is BLOCKED and owned by the sched lock of its last cpu, and one that has
 * already been woken another way -- a timeout that got in first -- is skipped.
 * Returns the number actually woken. No reschedule: for a caller that cannot
 * switch right now, or that is about to switch for good (thread_exit()).
 *
 * The threads are placed from the tail, so that the one that blocked first
 * ends up at the very head of the run queue it lands on.
 */
int sched_wake_list(struct list_node *threads, status_t wait_ret) {
    DEBUG_ASSERT(arch_ints_disabled());

    mp_cpu_mask_t mask = 0;
    int woken = 0;
    thread_t *t;
    while ((t = list_remove_tail_type(threads, thread_t, wait_queue_node)) != NULL) {
        uint target;
        if (sched_make_ready_from(t, THREAD_BLOCKED, wait_ret, true, false, &target)) {
            mask |= 1U << target;
            woken++;
        }
    }
    if (mask != 0) {
        sched_poke_cpus(mask);
    }
    return woken;
}

/* The same, for a caller that switches away right after so the woken threads
 * get to run first: the current thread goes to the head of the local run
 * queue before they are placed, so that any landing here go in ahead of it,
 * and the cpu reschedules. The caller holds |wq|'s lock, which is dropped
 * before the switch -- only the local sched lock is held across one -- and is
 * not retaken: no lock is held on return, interrupts are still off.
 *
 * The local sched lock is taken once for all of it. The current thread stays
 * on the local queue while the rest is done; nothing can pop it meanwhile,
 * because only this cpu pops this queue and interrupts are off.
 */
int sched_wake_list_and_resched(struct wait_queue *wq, struct list_node *threads,
                                status_t wait_ret) {
    thread_t *current_thread = get_current_thread();

    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(wait_queue_lock_held(wq));
    DEBUG_ASSERT(current_thread->state == THREAD_RUNNING);

    if (likely(!thread_is_idle(current_thread))) {
        /* returns holding the local sched lock */
        sched_requeue_current(current_thread, true);
    } else {
        spin_lock(sched_lock(arch_curr_cpu_num()));
        current_thread->state = THREAD_READY;
    }

    mp_cpu_mask_t mask = 0;
    int woken = 0;
    thread_t *t;
    while ((t = list_remove_tail_type(threads, thread_t, wait_queue_node)) != NULL) {
        uint target;
        if (sched_make_ready_from(t, THREAD_BLOCKED, wait_ret, true, true, &target)) {
            mask |= 1U << target;
            woken++;
        }
    }
    if (mask != 0) {
        /* mp_reschedule() masks the local cpu out; the switch below covers it */
        sched_poke_cpus(mask);
    }

    spin_unlock(wait_queue_lock(wq));

    sched_resched();

    spin_unlock(sched_lock(arch_curr_cpu_num()));
    return woken;
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

    /* A sleeping thread is on no queue; the sched lock of the cpu it slept on
     * owns it, and the transition to runnable takes that lock together with
     * the target's. Only the timer can wake a sleeper, so the state check
     * cannot fail. Interrupts are off here (timer callback), which
     * arch_mp_send_ipi() needs. */
    uint target;
    __UNUSED bool ok = sched_insert_runnable_from(t, THREAD_SLEEPING, &target);
    DEBUG_ASSERT(ok);
    sched_poke_cpus(1U << target);

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

    /* The timer is armed under the sched lock: the timer lock is the innermost
     * of the lot, so this nesting is the right way around. */
    arch_interrupt_saved_state_t state = sched_lock_local_irqsave();
    timer_set_oneshot(&timer, delay, thread_sleep_handler, (void *)current_thread);
    current_thread->state = THREAD_SLEEPING;
    sched_resched();
    sched_unlock_local_irqrestore(state);
}

/**
 * @brief Change priority of current thread
 *
 * See thread_create() for a discussion of priority values.
 */
void thread_set_priority(int priority) {
    thread_t *current_thread = get_current_thread();

    arch_interrupt_saved_state_t state = arch_interrupt_save();

    if (priority <= IDLE_PRIORITY) {
        priority = IDLE_PRIORITY + 1;
    }
    if (priority > HIGHEST_PRIORITY) {
        priority = HIGHEST_PRIORITY;
    }
    current_thread->priority = priority;

    sched_requeue_current(current_thread, true);
    sched_resched();

    sched_unlock_local_irqrestore(state);
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
        spin_lock_init(sched_lock(c));
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
