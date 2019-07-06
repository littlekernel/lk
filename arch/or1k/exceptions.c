/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kernel/thread.h>

enum handler_return platform_irq(void);
enum handler_return platform_tick(void);

void or1k_irq(void) {
    if (platform_irq() == INT_RESCHEDULE)
        thread_preempt();
}

void or1k_tick(void) {
    if (platform_tick() == INT_RESCHEDULE)
        thread_preempt();
}
