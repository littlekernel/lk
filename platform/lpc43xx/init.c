/*
 * Copyright (c) 2015 Brian Swetland
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
#include <arch/arm/cm.h>
#include <kernel/thread.h>
#include <platform.h>

void lpc43xx_debug_early_init(void);

void platform_early_init(void)
{
	lpc43xx_debug_early_init();
	arm_cm_systick_init(96000000);
}

void platform_init(void)
{
}

void platform_halt(platform_halt_action suggested_action,
			platform_halt_reason reason)
{
	arch_disable_ints();
	if (suggested_action == HALT_ACTION_REBOOT) {
		// CORE reset
		writel(1, 0x40053100);
	} else {
		dprintf(ALWAYS, "HALT: spinning forever... (reason = %d)\n", reason);
	}
	for(;;);
}
