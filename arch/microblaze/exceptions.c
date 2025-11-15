/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/compiler.h>
#include <lk/trace.h>
#include <arch/microblaze.h>
#include <kernel/thread.h>
#include <kernel/preempt.h>

void microblaze_irq(void) __attribute__((interrupt_handler));

enum handler_return platform_irq_handler(void);

void microblaze_irq(void) {
    enum handler_return ret;
    preempt_disable();
    ret = platform_irq_handler();
    bool need = preempt_enable_no_resched();
    if (ret == INT_RESCHEDULE || need)
        thread_preempt();
}

