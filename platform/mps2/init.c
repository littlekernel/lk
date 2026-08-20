/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/arm/cm.h>
#include <lk/debug.h>
#include <platform.h>
#include <platform/mps2.h>

#if MPS2_WITH_SEMIHOSTING
#include <lib/cmdline.h>
#include <lib/semihosting.h>

/*
 * cmdline_init() holds on to this rather than copying it, so it has to live
 * for the life of the system.
 */
static char cmdline_buf[512];

static void mps2_read_cmdline(void) {
    ssize_t len = semihosting_get_cmdline(cmdline_buf, sizeof(cmdline_buf));
    if (len < 0) {
        return;
    }

    cmdline_init(cmdline_buf, len);
}
#endif // MPS2_WITH_SEMIHOSTING

void platform_early_init(void) {
    mps2_debug_early_init();

    /*
     * The SSE based machines (an505, an547) do not wire up a systick reference
     * clock at all, so the timer has to run off the core clock. arm-m's systick
     * driver always selects the core clock, which works on every machine here.
     */
    arm_cm_systick_init(MPS2_SYSCLK_HZ);
}

void platform_init(void) {
    mps2_debug_init();

#if MPS2_WITH_SEMIHOSTING
    /*
     * These machines have no way to hand the guest a command line, so pull it
     * from the host. This has to land before the shell app starts looking for
     * lk.autorun, which happens well after platform init.
     */
    mps2_read_cmdline();
#endif

    dprintf(INFO, "sysclk %u Hz\n", (unsigned int)MPS2_SYSCLK_HZ);
}
