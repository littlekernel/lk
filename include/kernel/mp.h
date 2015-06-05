/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <kernel/thread.h>

void mp_init(void);

typedef uint32_t mp_cpu_mask_t;

#define MP_CPU_ALL_BUT_LOCAL (UINT32_MAX)

/* by default, mp_mbx_reschedule does not signal to cpus that are running realtime
 * threads. Override this behavior.
 */
#define MP_RESCHEDULE_FLAG_REALTIME (0x1)

void mp_reschedule(mp_cpu_mask_t target, uint flags);
void mp_set_curr_cpu_active(bool active);

typedef enum {
    MP_IPI_GENERIC,
    MP_IPI_RESCHEDULE,
} mp_ipi_t;

/* called from arch code during reschedule irq */
enum handler_return mp_mbx_reschedule_irq(void);

/* global mp state to track what the cpus are up to */
struct mp_state {
    volatile mp_cpu_mask_t active_cpus;

    /* only safely accessible with thread lock held */
    mp_cpu_mask_t idle_cpus;
    mp_cpu_mask_t realtime_cpus;
};

extern struct mp_state mp;

/* must be called with the thread lock held */
static inline void mp_set_cpu_idle(uint cpu)
{
    mp.idle_cpus |= 1UL << cpu;
}

static inline void mp_set_cpu_busy(uint cpu)
{
    mp.idle_cpus &= ~(1UL << cpu);
}

static inline mp_cpu_mask_t mp_get_idle_mask(void)
{
    return mp.idle_cpus;
}

static inline void mp_set_cpu_realtime(uint cpu)
{
    mp.realtime_cpus |= 1UL << cpu;
}

static inline void mp_set_cpu_non_realtime(uint cpu)
{
    mp.realtime_cpus &= ~(1UL << cpu);
}

static inline mp_cpu_mask_t mp_get_realtime_mask(void)
{
    return mp.realtime_cpus;
}


