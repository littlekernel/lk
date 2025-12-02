/*
 * Copyright (c) 2019 LK Trusty Authors. All Rights Reserved.
 * Copyright (c) 2025 Travis Geiselbrecht
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

#include <stdbool.h>
#include <sys/types.h>

struct iframe;

void arm_gicv3_init(void);
void arm_gicv3_init_percpu(void);
void arm_gicv3_configure_irq_locked(unsigned int cpu, unsigned int vector);
void arm_gicv3_suspend_cpu(unsigned int cpu);
void arm_gicv3_resume_cpu_locked(unsigned int cpu, bool gicd);
void arm_gicv3_wait_for_write_complete(void);
status_t arm_gicv3_sgi(u_int irq, u_int flags, u_int cpu_mask);
enum handler_return arm_gicv3_platform_irq(struct iframe *frame);
void arm_gicv3_platform_fiq(struct iframe *frame);

