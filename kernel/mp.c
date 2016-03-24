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

#include <kernel/mp.h>

#include <stdlib.h>
#include <debug.h>
#include <assert.h>
#include <trace.h>
#include <arch/mp.h>
#include <kernel/spinlock.h>

#define LOCAL_TRACE 0

#if WITH_SMP
/* a global state structure, aligned on cpu cache line to minimize aliasing */
struct mp_state mp __CPU_ALIGN;

void mp_init(void)
{
}

void mp_reschedule(mp_cpu_mask_t target, uint flags)
{
    uint local_cpu = arch_curr_cpu_num();

    LTRACEF("local %d, target 0x%x\n", local_cpu, target);

    /* mask out cpus that are not active and the local cpu */
    target &= mp.active_cpus;

    /* mask out cpus that are currently running realtime code */
    if ((flags & MP_RESCHEDULE_FLAG_REALTIME) == 0) {
        target &= ~mp.realtime_cpus;
    }
    target &= ~(1U << local_cpu);

    LTRACEF("local %d, post mask target now 0x%x\n", local_cpu, target);

    arch_mp_send_ipi(target, MP_IPI_RESCHEDULE);
}

void mp_set_curr_cpu_active(bool active)
{
    atomic_or((volatile int *)&mp.active_cpus, 1U << arch_curr_cpu_num());
}

enum handler_return mp_mbx_reschedule_irq(void)
{
    uint cpu = arch_curr_cpu_num();

    LTRACEF("cpu %u\n", cpu);

    THREAD_STATS_INC(reschedule_ipis);

    return (mp.active_cpus & (1U << cpu)) ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}
#endif
