/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kernel/thread.h>
#include <kernel/preempt.h>

enum handler_return platform_irq(void);
enum handler_return platform_tick(void);

void or1k_irq(void) {
    enum handler_return ret;
    preempt_disable();
    ret = platform_irq();
    bool need = preempt_enable_no_resched();
    if (ret == INT_RESCHEDULE || need)
        thread_preempt();
}

void or1k_tick(void) {
    enum handler_return ret;
    preempt_disable();
    ret = platform_tick();
    bool need = preempt_enable_no_resched();
    if (ret == INT_RESCHEDULE || need)
        thread_preempt();
}
