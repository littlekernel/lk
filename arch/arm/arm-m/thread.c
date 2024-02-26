/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <stdlib.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <assert.h>
#include <kernel/thread.h>
#include <arch/arm/cm.h>

#define LOCAL_TRACE 0

/* macros for saving and restoring a context switch frame, depending on what version of
 * the architecture you are */
#if  (__CORTEX_M >= 0x03)

/* cortex-m3 and above (armv7-m) */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)

#define SAVE_REGS \
        "tst        lr, #0x10;" /* check if thread used the FPU */ \
        "it         eq;" \
        "vpusheq    { s16-s31 };" /* also triggers lazy stacking of s0-s15 */ \
        "push       { r4-r11, lr };" /* note: saves 9 words */
#define RESTORE_REGS_PC \
        "pop        { r4-r11, lr };" \
        "tst        lr, #0x10;" \
        "it         eq;" \
        "vpopeq     { s16-s31 };" \
        "bx         lr;"

#else

#define SAVE_REGS       "push   { r4-r11, lr };" /* note: saves 9 words */
#define RESTORE_REGS_PC "pop    { r4-r11, pc };"

#endif

#else

/* cortex-m0 and cortex-m0+ (armv6-m) */
#define SAVE_REGS \
        "push   { r4-r7, lr };" \
        "mov    r4, r8;" \
        "mov    r5, r9;" \
        "mov    r6, r10;" \
        "mov    r7, r11;" \
        "push   { r4-r7 };" /* note: saves 9 words */
#define RESTORE_REGS_PC \
        "pop    { r4-r7 };" \
        "mov    r8 , r4;" \
        "mov    r9 , r5;" \
        "mov    r10, r6;" \
        "mov    r11, r7;" \
        "pop    { r4-r7, pc };"

#endif

/* since we're implicitly uniprocessor, store a pointer to the current thread here */
thread_t *_current_thread;

static thread_t *_prev_running_thread = NULL;

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void) {
    int ret;

    LTRACEF("thread %p calling %p with arg %p\n", _current_thread, _current_thread->entry, _current_thread->arg);
#if LOCAL_TRACE
    dump_thread(_current_thread);
#endif

    ret = _current_thread->entry(_current_thread->arg);

    LTRACEF("thread %p exiting with %d\n", _current_thread, ret);

    thread_exit(ret);
}

void arch_thread_initialize(struct thread *t) {
    LTRACEF("thread %p, stack %p\n", t, t->stack);

    /* find the top of the stack and align it on an 8 byte boundary */
    uint32_t *sp = (void *)ROUNDDOWN((vaddr_t)t->stack + t->stack_size, 8);

    struct arm_cm_exception_frame *frame = (void *)sp;
    frame--;

    /* arrange for pc to point to our starting routine */
    frame->pc = (uint32_t)&initial_thread_func;
    /* set thumb mode bit */
    frame->psr = xPSR_T_Msk;
    /* set EXC_RETURN value to thread mode using MSP and no FP */
    frame->exc_return = 0xfffffff9;

    t->arch.sp = (addr_t)frame;
}

static vaddr_t pendsv_swap_sp(vaddr_t old_frame) {
    /* make sure the stack is 8 byte aligned */
    DEBUG_ASSERT(((uintptr_t)__GET_FRAME() & 0x7) == 0);

    DEBUG_ASSERT_MSG(!spin_lock_held(&thread_lock),
                     "PENDSV: thread lock was held when preempted! pc %#x\n", ((struct arm_cm_exception_frame *)old_frame)->pc);

    DEBUG_ASSERT(_prev_running_thread != NULL);
    DEBUG_ASSERT(_current_thread != NULL);

#if     (__CORTEX_M >= 0X03) || (__CORTEX_SC >= 300)
    __CLREX();
#endif

    _prev_running_thread->arch.sp = old_frame;
    _prev_running_thread = NULL;
    return _current_thread->arch.sp;
}

/*
 * raw pendsv exception handler, triggered by arch_context_switch()
 * to do the actual switch.
 */
__NAKED void _pendsv(void) {
    __asm__ volatile(
        SAVE_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "cpsid  i;"
        "bl     %c0;"
        "cpsie  i;"
        "mov    sp, r0;"
        RESTORE_REGS_PC
        :: "i" (pendsv_swap_sp)
    );
    __UNREACHABLE;
}

/*
 * The raw context switch routine. Called by the scheduler when it decides to switch.
 * Called either in the context of a thread yielding or blocking (interrupts disabled,
 * on the system stack), or at the end of an interrupt handler via thread_preempt()
 * on a thread that is being preempted (interrupts disabled, in handler mode).
 */
void arch_context_switch(struct thread *oldthread, struct thread *newthread) {
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    LTRACEF("FPCCR.LSPACT %lu, FPCAR 0x%x, CONTROL.FPCA %lu\n",
            FPU->FPCCR & FPU_FPCCR_LSPACT_Msk, FPU->FPCAR, __get_CONTROL() & CONTROL_FPCA_Msk);
#endif

    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    const bool in_interrupt_context = arch_in_int_handler();

    /*
     * An interrupt handler might preempt a pending context switch,
     * but this should never happen in thread mode.
     */
    DEBUG_ASSERT(in_interrupt_context || (_prev_running_thread == NULL));

    /*
     * Since interrupt handlers could preempt PendSV and trigger additional
     * switches before the first switch happens, we need to remember which
     * thread was last running to ensure the context is saved to the correct
     * thread struct.
     */
    if (_prev_running_thread == NULL) {
        _prev_running_thread = oldthread;

        /*
         * Only trigger preempt if a context switch was not already pending.
         * This prevents a race where another interrupt could preempt PendSV
         * after it has started running, but before it has done the sp swap,
         * and mark it pending again, which could lead to a tail chained
         * second call to _pendsv() with _prev_running_thread set to NULL.
         */
        arm_cm_trigger_preempt();
    }

    /*
     * Make sure either pendsv is queued up either via the previous if statement
     * or via a nested preemption.
     */
    DEBUG_ASSERT(arm_cm_is_preempt_triggered());

    if (!in_interrupt_context) {
        /* we're in thread context, so jump to PendSV immediately */

        /* drop the lock and enable interrupts so PendSV can run */
        spin_unlock(&thread_lock);
        arch_enable_ints();

        /* should jump to PendSV here */

        arch_disable_ints();
        spin_lock(&thread_lock);
    } else {
        /*
         * If we're in interrupt context, then we've come through
         * thread_preempt() from arm_cm_irq_exit(). The switch will happen when
         * the current handler exits and tail-chains to PendSV.
         */
    }
}

void arch_dump_thread(thread_t *t) {
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
        dprintf(INFO, "sp 0x%lx", t->arch.sp);
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        const struct arm_cm_exception_frame *frame = (struct arm_cm_exception_frame *)t->arch.sp;
        const bool fpused = (frame->exc_return & 0x10) == 0;
        dprintf(INFO, ", fpused %u", fpused);
#endif
        dprintf(INFO, "\n");
    }
}

