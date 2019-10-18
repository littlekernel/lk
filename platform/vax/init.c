/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <platform.h>
#include <platform/timer.h>
#include <arch/vax.h>

#include "platform_p.h"

uint32_t vax_cputype;
uint32_t vax_boardtype;

void platform_early_init(void) {
    // decode what kind of vax we are so we can make a few platform decisions
    // generally follows logic in netbsd sys/arch/vax/...
    uint32_t sid = mfpr(PR_SID);
    vax_cputype = sid >> 24;
    vax_boardtype = vax_cputype << 24;

    switch (vax_cputype) {
        case 10: // CVAX
        case 11: // RIGEL
        case 18: // MARIAH
        case 19: // NVAX
        case 20: { // SOC
            uint32_t sie = *(uint32_t *)(0x20040004);
            vax_boardtype |= sie >> 24;
            break;
        }
    }

    platform_early_console_init();

    printf("\n");
    printf("VAX: sid %#x\n", sid);
    printf("VAX: cputype %#x\n", vax_cputype);
    printf("VAX: boardtype %#x\n", vax_boardtype);
}

// stubbed out time
static lk_time_t t = 0;
lk_time_t current_time() {
    return ++t;
}

lk_bigtime_t current_time_hires() {
    return (++t) * 1000;
}
