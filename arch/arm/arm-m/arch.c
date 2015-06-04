/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
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
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <platform.h>
#include <arch/arm/cm.h>

extern void *vectab;

#if ARM_CM_DYNAMIC_PRIORITY_SIZE
unsigned int arm_cm_num_irq_pri_bits;
unsigned int arm_cm_irq_pri_mask;
#endif

void arch_early_init(void)
{
	uint i;

	arch_disable_ints();

	/* set the vector table base */
	SCB->VTOR = (uint32_t)&vectab;

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

	/* set the svc and pendsv priority level to pretty low */
	NVIC_SetPriority(SVCall_IRQn, arm_cm_lowest_priority());
	NVIC_SetPriority(PendSV_IRQn, arm_cm_lowest_priority());

	/* set systick to medium priority */
	NVIC_SetPriority(SysTick_IRQn, arm_cm_medium_priority());
}

void arch_init(void)
{
#if ENABLE_CYCLE_COUNTER
	*REG32(SCB_DEMCR) |= 0x01000000; // global trace enable
	*REG32(DWT_CYCCNT) = 0;
	*REG32(DWT_CTRL) |= 1; // enable cycle counter
#endif
}

void arch_quiesce(void)
{
}

void arch_idle(void)
{
	__asm__ volatile("wfi");
}

void _arm_cm_set_irqpri(uint32_t pri)
{
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

void arm_cm_irq_entry(void)
{
	// Set PRIMASK to 1
	// This is so that later calls to arch_ints_disabled() returns true while we're inside the int handler
	// Note: this will probably screw up future efforts to stack higher priority interrupts since we're setting
	// the cpu to essentially max interrupt priority here. Will have to rethink it then.
	__disable_irq();

	THREAD_STATS_INC(interrupts);
	KEVLOG_IRQ_ENTER(__get_IPSR());
}

void arm_cm_irq_exit(bool reschedule)
{
	if (reschedule)
		arm_cm_trigger_preempt();

	KEVLOG_IRQ_EXIT(__get_IPSR());

	__enable_irq(); // clear PRIMASK
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3)
{
    PANIC_UNIMPLEMENTED;
}

// vim: set noexpandtab:
