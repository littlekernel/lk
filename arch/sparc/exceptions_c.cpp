//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//

#include <arch/sparc.h>
#include <stdio.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <platform/interrupts.h>

#define LOCAL_TRACE 1

extern "C" {
enum handler_return platform_irq(uint32_t irq);
void sparc_exception(uint32_t exception, uint32_t pc, uint32_t npc, uint32_t psr);
}

extern "C" void sparc_exception(uint32_t exception, uint32_t pc, uint32_t npc, uint32_t psr) {
    LTRACEF("exc %#x at PC %#x, nPC %#x, PSR %#x\n", exception, pc, npc, psr);

    switch (exception) {
        case 0x11 ... 0x1f: // IRQs 1-15
            platform_irq(exception - 0x10);
            break;
        default:
            // unhandled exception
            panic("unhandled exception");
    }

    // TODO: handle preemption
    PANIC_UNIMPLEMENTED;
}