/*
 * Copyright (c) 2012-2013 Travis Geiselbrecht
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
#ifndef __ARCH_ARM_CM_H
#define __ARCH_ARM_CM_H

/* support header for all cortex-m class cpus */

#include <compiler.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <platform/platform_cm.h>

#if ARM_CPU_CORTEX_M3
#include <core_cm3.h>
#elif ARM_CPU_CORTEX_M4
#include <core_cm4.h>
#else
#error "unknown cortex-m core"
#endif

/* registers dealing with the cycle counter */
#define DWT_CTRL (0xE0001000)
#define DWT_CYCCNT (0xE0001004)
#define SCB_DEMCR (0xE000EDFC)

struct arm_cm_exception_frame {
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t psr;
};

struct arm_cm_exception_frame_short {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t psr;
};

struct arm_cm_exception_frame_long {
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t lr;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t exc_lr;
	uint32_t pc;
	uint32_t psr;
};

#if ARM_M_DYNAMIC_PRIORITY_SIZE
extern unsigned int arm_cm_num_irq_pri_bits;
extern unsigned int arm_cm_irq_pri_mask;
#else
/* if we don't want to calculate the nubmer of priority bits, then assume
 * the cpu implements 3 (8 priority levels), which is the minimum according to spec.
 */
#ifndef ARM_M_PRIORITY_BITS
#define ARM_M_PRIORITY_BITS 3
#endif
static const unsigned int arm_cm_num_irq_pri_bits = 8 - ARM_M_PRIORITY_BITS;
static const unsigned int arm_cm_irq_pri_mask = ~((1 << ARM_M_PRIORITY_BITS) - 1) & 0xff;
#endif

void _arm_cm_set_irqpri(uint32_t pri);

static void arm_cm_set_irqpri(uint32_t pri)
{
	if (__ISCONSTANT(pri)) {
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
	} else {
		_arm_cm_set_irqpri(pri);
	}
}


static inline uint32_t arm_cm_highest_priority(void)
{
	return (1 << (8 - arm_cm_num_irq_pri_bits));
}

static inline uint32_t arm_cm_lowest_priority(void)
{
	return (255 & arm_cm_irq_pri_mask) & 0xff;
}

static inline uint32_t arm_cm_medium_priority(void)
{
	return (128 & arm_cm_irq_pri_mask) & 0xff;
}

static inline void arm_cm_trigger_interrupt(int vector)
{
	NVIC->STIR = vector;
}

static inline void arm_cm_trigger_preempt(void)
{
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/* systick */
void arm_cm_systick_init(void);
void arm_cm_systick_set_periodic(uint32_t systick_clk_freq, lk_time_t period);
void arm_cm_systick_cancel_periodic(void);
/* extern void _systick(void); // override this */

/* interrupt glue */
/*
 * Platform code should put this as the first and last line of their irq handlers.
 * Pass true to reschedule to request a preempt.
 */
void arm_cm_irq_entry(void);
void arm_cm_irq_exit(bool reschedule);

#endif

