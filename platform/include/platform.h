/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>

/* TODO: move all callers to using time.h directly */
#include <platform/time.h>

__BEGIN_CDECLS

typedef enum {
    HALT_ACTION_HALT = 0,       // Spin forever.
    HALT_ACTION_REBOOT,         // Reset the CPU.
    HALT_ACTION_SHUTDOWN,       // Shutdown and power off.
} platform_halt_action;

const char *platform_halt_action_string(platform_halt_action action);

typedef enum {
    HALT_REASON_UNKNOWN = 0,
    HALT_REASON_POR,            // Cold-boot
    HALT_REASON_HW_WATCHDOG,    // HW watchdog timer
    HALT_REASON_LOWVOLTAGE,     // LV/Brownout condition
    HALT_REASON_HIGHVOLTAGE,    // High voltage condition.
    HALT_REASON_THERMAL,        // Thermal reason (probably overtemp)
    HALT_REASON_OTHER_HW,       // Other hardware (platform) specific reason
    HALT_REASON_SW_RESET,       // Generic Software Initiated Reboot
    HALT_REASON_SW_WATCHDOG,    // Reboot triggered by a SW watchdog timer
    HALT_REASON_SW_PANIC,       // Reboot triggered by a SW panic or ASSERT
    HALT_REASON_SW_UPDATE,      // SW triggered reboot in order to begin firmware update
} platform_halt_reason;

const char *platform_halt_reason_string(platform_halt_reason reason);

/* if the platform has knowledge of what caused the latest reboot, it can report
 * it to applications with this function.  */
platform_halt_reason platform_get_reboot_reason(void);

/* platform_halt is a method which is called from various places in the LK
 * system, and may be implemented by platforms and called by applications.  This
 * call represents the end of the life of SW for a device; there is no returning
 * from this function.  Callers will provide a reason for the halt, and a
 * suggested action for the platform to take, but it is the platform's
 * responsibility to determine the final action taken.  For example, in the case
 * of a failed ASSERT or a panic, LK will call platform halt and suggest a Halt
 * action, but a release build on a platform with no debug channel may choose to
 * reboot instead as there is no one to tell about the ASSERT, and no one
 * waiting to debug the device in its halted state.  If not overloaded by the
 * platform, the default behavior of platform halt will be to dprintf the
 * reason, and then halt execution by turning off interrupts and spinning
 * forever.
 */
__WEAK void platform_halt(platform_halt_action suggested_action,
                   platform_halt_reason reason) __NO_RETURN;

/* Default implementation of the above routine, which platforms can call with
 * appropriate hooks to implement platform specific reboot and shutdown behavior.
 */
typedef void (*platform_reboot_hook)(void);
typedef void (*platform_shutdown_hook)(void);
void platform_halt_default(platform_halt_action suggested_action,
                   platform_halt_reason reason,
                   platform_reboot_hook prh,
                   platform_shutdown_hook psh) __NO_RETURN;

/* called during chain loading to make sure drivers and platform is put into a stopped state */
void platform_quiesce(void);

/* super early platform initialization, before almost everything */
void platform_early_init(void);

/* later init, after the kernel has come up */
void platform_init(void);

/* called by the arch init code to get the platform to set up any mmu mappings it may need */
void platform_init_mmu_mappings(void);

/* Called by LK to get the device tree */
const void *get_fdt(void);

__END_CDECLS

