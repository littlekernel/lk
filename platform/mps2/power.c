/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/arm/cm.h>
#include <platform.h>

#if MPS2_WITH_SEMIHOSTING
#include <lib/semihosting.h>
#endif

static void mps2_reboot(void) {
    NVIC_SystemReset();
}

#if MPS2_WITH_SEMIHOSTING
static void mps2_shutdown(void) {
    /*
     * There is no power controller on these boards, so the only way to stop
     * the machine is to ask the host to do it. Falls through to the default
     * spin if nothing is listening.
     */
    semihosting_exit(ADP_STOPPED_APPLICATION_EXIT);
}
#else
#define mps2_shutdown NULL
#endif

void platform_halt(platform_halt_action suggested_action,
                   platform_halt_reason reason) {
    platform_halt_default(suggested_action, reason, mps2_reboot, mps2_shutdown);
}
