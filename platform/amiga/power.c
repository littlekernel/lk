/*
 * Copyright (c) 2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "platform_p.h"
#include <platform.h>

static volatile uint8_t *const cia_base = (volatile uint8_t *)CIA_A_BASE;
static volatile uint16_t *const paula_base = (void *)0xDFF000;

static void amiga_reboot(void) {
    cia_base[CIA_A_DDRB >> 1] |= 0x80; // Set port direction
    cia_base[CIA_A_PRB >> 1] &= ~0x80; // Assert reset bit
}

static void amiga_shutdown(void) {
    // Might be able to get away with just the 'stop'
    paula_base[0x9c >> 1] = 0x7fff; // INTREQ
    paula_base[0x9a >> 1] = 0x7fff; // INTENA
    paula_base[0x96 >> 1] = 0x7fff; // DMACON

    __asm__ volatile("stop #0x2700");
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    dprintf(INFO, "halt action %s, reason %s\n",
            platform_halt_action_string(suggested_action),
            platform_halt_reason_string(reason));
    platform_halt_default(suggested_action, reason, &amiga_reboot, &amiga_shutdown);
}
