/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __PLATFORM_H
#define __PLATFORM_H

#include <sys/types.h>
#include <compiler.h>

__BEGIN_CDECLS;

typedef enum {
    HALT_ACTION_HALT = 0,       // Spin forever.
    HALT_ACTION_REBOOT,         // Reset the CPU.
    HALT_ACTION_SHUTDOWN,       // Shutdown and power off.
} platform_halt_action;

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

lk_time_t current_time(void);
lk_bigtime_t current_time_hires(void);

/* super early platform initialization, before almost everything */
void platform_early_init(void);

/* later init, after the kernel has come up */
void platform_init(void);

/* called by the arch init code to get the platform to set up any mmu mappings it may need */
void platform_init_mmu_mappings(void);

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
void platform_halt(platform_halt_action suggested_action,
                   platform_halt_reason reason) __NO_RETURN;

/* called during chain loading to make sure drivers and platform is put into a stopped state */
void platform_quiesce(void);

__END_CDECLS;

#endif
