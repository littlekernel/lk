/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <platform.h>
#include <arch/arm/cm.h>
#include <target.h>

extern void *vectab;

#if ARM_CM_DYNAMIC_PRIORITY_SIZE
unsigned int arm_cm_num_irq_pri_bits;
unsigned int arm_cm_irq_pri_mask;
#endif

/* if otherwise not set externally, load the VTOR for all armv7m+ cpus.
 * dynamic VTOR setting is optional on armv6m cores.
 */
#ifndef ARM_CM_SET_VTOR
#if (__CORTEX_M >= 0x03) || (CORTEX_SC >= 300)
#define ARM_CM_SET_VTOR 1
#else
#define ARM_CM_SET_VTOR 0
#endif
#endif

void arch_early_init(void) {

    arch_disable_ints();

#if ARM_CM_SET_VTOR
    /* set the vector table base */
    SCB->VTOR = (uint32_t)&vectab;
#endif

#if (__CORTEX_M >= 0x03) || (CORTEX_SC >= 300)
    uint i;
#if ARM_CM_DYNAMIC_PRIORITY_SIZE
    /* number of priorities */
    for (i=0; i < 7; i++) {
        __set_BASEPRI(1 << i);
        if (__get_BASEPRI() != 0)
            break;
    }
    arm_cm_num_irq_pri_bits = 8 - i;
    arm_cm_irq_pri_mask = ~((1 << i) - 1) & 0xff;
#endif

    /* clear any pending interrupts and set all the vectors to medium priority */
    uint groups = (SCnSCB->ICTR & 0xf) + 1;
    for (i = 0; i < groups; i++) {
        NVIC->ICER[i] = 0xffffffff;
        NVIC->ICPR[i] = 0xffffffff;
        for (uint j = 0; j < 32; j++) {
            NVIC_SetPriority(i*32 + j, arm_cm_medium_priority());
        }
    }

    /* leave BASEPRI at 0 */
    __set_BASEPRI(0);

    /* set priority grouping to 0 */
    NVIC_SetPriorityGrouping(0);

    /* enable certain faults */
    SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk);
#endif

    /* set the svc and pendsv priority level to pretty low */
    NVIC_SetPriority(SVCall_IRQn, arm_cm_lowest_priority());
    NVIC_SetPriority(PendSV_IRQn, arm_cm_lowest_priority());

    /* set systick and debugmonitor to medium priority */
    NVIC_SetPriority(SysTick_IRQn, arm_cm_medium_priority());

#if (__CORTEX_M >= 0x03)
    NVIC_SetPriority(DebugMonitor_IRQn, arm_cm_medium_priority());
#endif

    /* FPU settings ------------------------------------------------------------*/
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

#if ARM_WITH_CACHE
    arch_enable_cache(ARCH_CACHE_FLAG_UCACHE);
#endif
}

void arch_init(void) {
#if ENABLE_CYCLE_COUNTER
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // enable cycle counter
#endif
}

void arch_quiesce(void) {
#if ARM_WITH_CACHE
    arch_disable_cache(ARCH_CACHE_FLAG_UCACHE);
#endif
}

void arch_idle(void) {
    __asm__ volatile("wfi");
}

#if     (__CORTEX_M >= 0x03) || (CORTEX_SC >= 300)

void _arm_cm_set_irqpri(uint32_t pri) {
    if (pri == 0) {
        __disable_irq(); // cpsid i
        __set_BASEPRI(0);
    } else if (pri >= 256) {
        __set_BASEPRI(0);
        __enable_irq();
    } else {
        uint32_t _pri = pri & arm_cm_irq_pri_mask;

        if (_pri == 0)
            __set_BASEPRI(1 << (8 - arm_cm_num_irq_pri_bits));
        else
            __set_BASEPRI(_pri);
        __enable_irq(); // cpsie i
    }
}
#endif


void arm_cm_irq_entry(void) {
    // Set PRIMASK to 1
    // This is so that later calls to arch_ints_disabled() returns true while we're inside the int handler
    // Note: this will probably screw up future efforts to stack higher priority interrupts since we're setting
    // the cpu to essentially max interrupt priority here. Will have to rethink it then.
    __disable_irq();

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(__get_IPSR());

    target_set_debug_led(1, true);
}

void arm_cm_irq_exit(bool reschedule) {
    target_set_debug_led(1, false);

    if (reschedule)
        thread_preempt();

    KEVLOG_IRQ_EXIT(__get_IPSR());

    __enable_irq(); // clear PRIMASK
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
#if (__CORTEX_M >= 0x03)

    uint32_t *entry_vector = (uint32_t *)entry;

    __asm__ volatile(
        "mov r0,  %[arg0]; "
        "mov r1,  %[arg1]; "
        "mov r2,  %[arg2]; "
        "mov r3,  %[arg3]; "
        "mov sp,  %[SP]; "
        "bx  %[entry]; "
        :
        : [arg0]"r"(arg0),
        [arg1]"r"(arg1),
        [arg2]"r"(arg2),
        [arg3]"r"(arg3),
        [SP]"r"(entry_vector[0]),
        [entry]"r"(entry_vector[1])
        : "r0", "r1", "r2", "r3"
    );

    __UNREACHABLE;
#else
    PANIC_UNIMPLEMENTED;
#endif
}
