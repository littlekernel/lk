/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#ifndef __ARCH_ARM_CM3_H
#define __ARCH_ARM_CM3_H

#include <platform/platform_cm3.h>
#include <core_cm3.h>

struct cm3_exception_frame {
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

struct cm3_exception_frame_short {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t psr;
};

struct cm3_exception_frame_long {
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

void cm3_set_irqpri(uint32_t pri);

extern unsigned int cm3_num_irq_pri_bits;
extern unsigned int cm3_irq_pri_mask;

static __ALWAYS_INLINE inline uint32_t cm3_highest_priority(void)
{
	return (1 << (8 - cm3_num_irq_pri_bits));
}

static __ALWAYS_INLINE inline uint32_t cm3_lowest_priority(void)
{
	return (255 & cm3_irq_pri_mask) & 0xff;
}

static __ALWAYS_INLINE inline uint32_t cm3_medium_priority(void)
{
	return (128 & cm3_irq_pri_mask) & 0xff;
}

static __ALWAYS_INLINE inline void cm3_trigger_interrupt(int vector)
{
	NVIC->STIR = vector;
}

static __ALWAYS_INLINE inline void cm3_trigger_preempt(void)
{
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

#endif

