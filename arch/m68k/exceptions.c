/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/m68k.h>
#include <inttypes.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

// defined in platform interrupt controller
extern enum handler_return m68k_platform_irq(uint8_t irq);

void m68k_exception(void *frame, uint8_t code) {
    LTRACEF("frame %p, code %hhu\n", frame, code);

    panic("unimplemented exception %hhu\n", code);
}

void m68k_irq(void *frame, uint8_t code) {
    LTRACEF("frame %p, code %hhu\n", frame, code);

    if (unlikely(code == 0)) {
        // spurious interrupt
        return;
    }

    enum handler_return ret = m68k_platform_irq(code);
    if (ret == INT_RESCHEDULE) {
        thread_preempt();
    }
}
