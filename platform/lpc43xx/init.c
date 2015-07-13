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

#include <platform/lpc43xx-clocks.h>

void lpc43xx_debug_early_init(void);

void platform_early_init(void)
{
	unsigned cfg;

	// switch CPU clock to 12MHz internal osc
	writel(readl(BASE_M4_CLK) | BASE_AUTOBLOCK, BASE_M4_CLK);
	writel(BASE_CLK_SEL(CLK_IRC) | BASE_AUTOBLOCK, BASE_M4_CLK);

	// Disable PLL1, if it was already running
	writel(PLL1_CTRL_PD, PLL1_CTRL);

	// PLL1: 12MHz -> N=(/2) -> M=(x32) -> P=(/2)   96MHz
	cfg = PLL1_CTRL_NSEL_2 | PLL1_CTRL_PSEL_1 | PLL1_CTRL_MSEL(32) |
		PLL1_CTRL_CLK_SEL(CLK_IRC) | PLL1_CTRL_AUTOBLOCK;
	writel(cfg, PLL1_CTRL);
	while (!(readl(PLL1_STAT) & PLL1_STAT_LOCK)) ;

	writel(BASE_CLK_SEL(CLK_PLL1) | BASE_AUTOBLOCK, BASE_M4_CLK);

	// when moving from < 90 MHz to > 110MHz, must spend 50uS
	// at 90-110MHz before shifting to high speeds

	spin_cycles(4800); // 50uS @ 96MHz

	// disable P divider  192MHz
	writel(cfg | PLL1_CTRL_DIRECT, PLL1_CTRL);

#if 0
	// route PLL1 / 2 to CLK0 pin for verification
	writel(0x11, 0x40086C00); // CLK0 = CLK_OUT, no PU/PD
	writel(IDIV_CLK_SEL(CLK_PLL1) | IDIV_N(2), IDIVE_CTRL);
	writel(BASE_CLK_SEL(CLK_IDIVE), BASE_OUT_CLK);
#endif

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
