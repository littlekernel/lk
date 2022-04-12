/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/rosco-m68k.h>
#include <sys/types.h>
#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif
#include <kernel/novm.h>
#if WITH_LIB_CONSOLE
#include <lib/console.h>
#endif

#include "platform_p.h"

#define LOCAL_TRACE 0

extern uint8_t __bss_end;

void platform_early_init(void) {
    duart_early_init();

    dprintf(INFO, "ROSCO-M68K: firmware structure at 0x400 - 0x41f:\n");
    hexdump((void *)0x400, 0x20);

    uint32_t cpu_info = *(uint32_t *)0x41c;
    printf("cpu family %u speed %u\n", cpu_info >> 29, cpu_info & 0x1fffffff);

    // TODO: probe memory
    // TODO: consider using firmware struct left around 0x400
    uint32_t membase = 0x0;
    uint32_t memsize = 0x100000; // 1MB

    dprintf(INFO, "ROSCO-M68K: memory base %#x size %#x\n", membase, memsize);
    novm_add_arena("mem", membase, memsize);

    // build a table of illegal instructions around 0 to try to catch bad branches
    volatile uint16_t *ptr = 0;
    for (int i = 0; i < 256; i++) {
        ptr[i] = 0x4afa; // undefined opcode
    }
}

void platform_init(void) {
    duart_init();
}

enum handler_return m68k_platform_irq(uint8_t irq) {
    LTRACEF("irq %u\n", irq);

    switch (irq) {
        case 0x45: // DUART irq
            return duart_irq();
        default:
            panic("unhandled platform irq %u\n", irq);
    }
    return INT_NO_RESCHEDULE;
}
