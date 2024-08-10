/*
 * Copyright (c) 2012 Google, Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/compiler.h>
#include <lk/console_cmd.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>

#if WITH_LIB_CONSOLE
#include <lib/console.h>
#endif

// Default implementation of the system halt and reset function. Platforms may use
// this as a helper to implement the logic by passing in a few hooks to do the
// actual shutdown or reboot.
void platform_halt_default(platform_halt_action suggested_action,
                           platform_halt_reason reason,
                           platform_reboot_hook prh,
                           platform_shutdown_hook psh) {
    const char *reason_string = platform_halt_reason_string(reason);
    switch (suggested_action) {
        case HALT_ACTION_SHUTDOWN:
            if (psh) {
                dprintf(ALWAYS, "Shutting down, reason '%s'\n", reason_string);
                psh();
            }
            break;
        case HALT_ACTION_REBOOT:
            if (prh) {
                dprintf(ALWAYS, "Rebooting, reason '%s'\n", reason_string);
                prh();
            }
            break;
        case HALT_ACTION_HALT:
#if ENABLE_PANIC_SHELL
            if (reason == HALT_REASON_SW_PANIC) {
                dprintf(ALWAYS, "CRASH: starting debug shell, reason '%s'\n", reason_string);
                arch_disable_ints();
                panic_shell_start();
            }
#endif  // ENABLE_PANIC_SHELL
            break;
    }

    dprintf(ALWAYS, "HALT: spinning forever, reason '%s'\n", reason_string);
    arch_disable_ints();
    for (;;)
        arch_idle();
}

// Default implementation of platform halt, which simply start the debug shell or spins forever.
__WEAK void platform_halt(platform_halt_action suggested_action,
                          platform_halt_reason reason) {
    platform_halt_default(suggested_action, reason, NULL, NULL);
}

__WEAK platform_halt_reason platform_get_reboot_reason(void) {
    return HALT_REASON_UNKNOWN;
}

const char *platform_halt_action_string(platform_halt_action action) {
    switch(action) {
        case HALT_ACTION_HALT:
            return "halt";
        case HALT_ACTION_REBOOT:
            return "reboot";
        case HALT_ACTION_SHUTDOWN:
            return "shutdown";
    }
    return "unknown";
}

const char *platform_halt_reason_string(platform_halt_reason reason) {
    switch(reason) {
        case HALT_REASON_UNKNOWN:
            return "unknown";
        case HALT_REASON_POR:
            return "power on reset";
        case HALT_REASON_HW_WATCHDOG:
            return "hardware watchdog";
        case HALT_REASON_LOWVOLTAGE:
            return "low voltage";
        case HALT_REASON_HIGHVOLTAGE:
            return "high voltage";
        case HALT_REASON_THERMAL:
            return "thermal";
        case HALT_REASON_OTHER_HW:
            return "other hardware";
        case HALT_REASON_SW_RESET:
            return "software reset";
        case HALT_REASON_SW_WATCHDOG:
            return "software watchdog";
        case HALT_REASON_SW_PANIC:
            return "software panic";
        case HALT_REASON_SW_UPDATE:
            return "software update";
    }
    return "unknown";
}

static int cmd_reboot(int argc, const console_cmd_args *argv) {
    platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RESET);
    return 0;
}

static int cmd_poweroff(int argc, const console_cmd_args *argv) {
    platform_halt(HALT_ACTION_SHUTDOWN, HALT_REASON_SW_RESET);
    return 0;
}

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 1
STATIC_COMMAND("reboot", "soft reset", &cmd_reboot)
STATIC_COMMAND("poweroff", "powerdown", &cmd_poweroff)
#endif
STATIC_COMMAND_END(platform_power);
