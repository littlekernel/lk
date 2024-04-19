/*
 * Copyright (c) 2012-2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __ARCH_ARM_CM_H
#define __ARCH_ARM_CM_H

/* support header for all cortex-m class cpus */

#include <assert.h>
#include <lk/compiler.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <platform/platform_cm.h>

#if ARM_CPU_CORTEX_M0
#include <core_cm0.h>
#elif ARM_CPU_CORTEX_M0_PLUS
#include <core_cm0plus.h>
#elif ARM_CPU_CORTEX_M3
#include <core_cm3.h>
#elif ARM_CPU_CORTEX_M4
#include <core_cm4.h>
#elif ARM_CPU_CORTEX_M55
#include <core_cm55.h>
#elif ARM_CPU_CORTEX_M7
#include <core_cm7.h>
#else
#error "unknown cortex-m core"
#endif

/* registers dealing with the cycle counter */
#define DWT_CTRL (0xE0001000)
#define DWT_CYCCNT (0xE0001004)
#define SCB_DEMCR (0xE000EDFC)

struct arm_cm_exception_frame {
#if (__CORTEX_M >= 0x03)
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
#else
    /* frame format is slightly different due to ordering of push/pops */
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
#endif
    uint32_t exc_return;

    /* hardware pushes this */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
};

static_assert(sizeof(struct arm_cm_exception_frame) == 17 * 4, "");

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)

/* exception frame when fpu context save is enabled */
struct arm_cm_exception_frame_fpu {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t exc_return;

    float s16_31[16];

    /* hardware pushes this */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;

    /* additional state the cpu pushes when using an extended frame */
    float    s0_15[16];
    uint32_t fpscr;
    uint32_t reserved;
};

static_assert(sizeof(struct arm_cm_exception_frame_fpu) == (9 + 16 + 8 + 16 + 2) * 4, "");

#endif

#if ARM_CM_DYNAMIC_PRIORITY_SIZE
extern unsigned int arm_cm_num_irq_pri_bits;
extern unsigned int arm_cm_irq_pri_mask;
#else
/* if we don't want to calculate the number of priority bits, then assume
 * the cpu implements 3 (8 priority levels), which is the minimum according to spec.
 */
#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS 3
#endif
static const unsigned int arm_cm_num_irq_pri_bits = __NVIC_PRIO_BITS;
static const unsigned int arm_cm_irq_pri_mask = ~((1 << __NVIC_PRIO_BITS) - 1) & 0xff;
#endif

#if     (__CORTEX_M >= 0x03) || (CORTEX_SC >= 300)

void _arm_cm_set_irqpri(uint32_t pri);

static void arm_cm_set_irqpri(uint32_t pri) {
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
#endif

static inline uint32_t arm_cm_highest_priority(void) {
    return 0;
}

static inline uint32_t arm_cm_lowest_priority(void) {
    return (1 << arm_cm_num_irq_pri_bits) - 1;
}

static inline uint32_t arm_cm_medium_priority(void) {
    return (1 << (arm_cm_num_irq_pri_bits - 1));
}

#if     (__CORTEX_M >= 0x03) || (CORTEX_SC >= 300)
static inline void arm_cm_trigger_interrupt(int vector) {
    NVIC->STIR = vector;
}
#endif

static inline void arm_cm_trigger_preempt(void) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

static inline bool arm_cm_is_preempt_triggered(void) {
    return SCB->ICSR & SCB_ICSR_PENDSVSET_Msk;
}

/* systick */
void arm_cm_systick_init(uint32_t mhz);

/* interrupt glue */
/*
 * Platform code should put this as the first and last line of their irq handlers.
 * Pass true to reschedule to request a preempt.
 */
void arm_cm_irq_entry(void);
void arm_cm_irq_exit(bool reschedule);

/* built in exception vectors */
void _nmi(void);
void _hardfault(void);
void _memmanage(void);
void _busfault(void);
void _usagefault(void);
void _svc(void);
void _debugmonitor(void);
void _pendsv(void);
void _systick(void);

#endif

