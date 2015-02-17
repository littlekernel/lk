/*
 * Copyright (c) 2012 Google, Inc.
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
#include <debug.h>
#include <err.h>
#include <compiler.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>

/*
 * default implementations of these routines, if the platform code
 * chooses not to implement.
 */
__WEAK void platform_halt(platform_halt_action suggested_action,
                          platform_halt_reason reason)
{
    dprintf(ALWAYS, "HALT: spinning forever... (reason = %d)\n", reason);
    arch_disable_ints();
    for(;;);
}

#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int cmd_reboot(int argc, const cmd_args *argv)
{
    platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RESET);
    return 0;
}

static int cmd_poweroff(int argc, const cmd_args *argv)
{
    platform_halt(HALT_ACTION_SHUTDOWN, HALT_REASON_SW_RESET);
    return 0;
}

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 1
    { "reboot", "soft reset", &cmd_reboot },
    { "poweroff", "powerdown", &cmd_poweroff },
#endif
STATIC_COMMAND_END(platform_power);

#endif

