/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/main.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/rosco-m68k.h>
#include <string.h>
#include <sys/types.h>
#include <kernel/novm.h>

#include "platform_p.h"

#define LOCAL_TRACE 0

extern uint8_t __bss_end;

// firmware structure left behind by boot rom
// described at
// https://github.com/rosco-m68k/rosco_m68k/blob/develop/code/firmware/rosco_m68k_firmware/InterfaceReference.md
#define ROSCO_SDB_MAGIC (0xb105d47a)
struct rosco_system_data_block {
    uint32_t magic;
    uint32_t status;
    uint16_t timer_tick_counter;
    uint16_t system_flags;
    uint32_t upticks_counter;
    uint8_t  easy68k_flag_echo_on;
    uint8_t  easy68k_flag_prompt_on;
    uint8_t  easy68k_flag_lf_display;
    uint8_t  timer_tick_internal;
    uint32_t mem_size;
    uint32_t default_uart_base;
    uint32_t cpu_info;
};
STATIC_ASSERT(sizeof(struct rosco_system_data_block) == 0x20);

void platform_early_init(void) {
    duart_early_init();

    volatile struct rosco_system_data_block *sdb = (void *)lk_boot_args[0];

    // default to 1MB memory at 0
    uint32_t membase = 0x0;
    uint32_t memsize = 0x100000; // 1MB

    if (sdb->magic != ROSCO_SDB_MAGIC) {
        dprintf(INFO, "ROSCO-M68K: firmware failed magic check at %p\n", sdb);
    } else {
        dprintf(INFO, "ROSCO-M68K: firmware structure at %p - %p:\n", sdb, sdb + 0x1f);
        hexdump((void *)sdb, sizeof(*sdb));

        printf("cpu family %u speed %u\n", sdb->cpu_info >> 29, sdb->cpu_info & 0x1fffffff);

        memsize = sdb->mem_size;
    }

    dprintf(INFO, "ROSCO-M68K: memory base %#x size %#x\n", membase, memsize);
    novm_add_arena("mem", membase, memsize);

    // build a table of illegal instructions around 0 to try to catch bad branches
    uint16_t ins = 0x4afa;
    for (uintptr_t i = 0; i < 256; i++) {
        memcpy((void *)i, &ins, 2);
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
