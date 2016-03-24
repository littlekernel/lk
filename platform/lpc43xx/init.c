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
void lpc43xx_debug_init(void);

uint8_t __lpc43xx_main_clock_sel;
uint32_t __lpc43xx_main_clock_mhz;

void platform_early_init(void)
{
#ifndef WITH_NO_CLOCK_INIT
    unsigned cfg;
    // Different boot modes will enable different sets of clocks.
    // To keep it simple, we drop back to the 12MHz internal osc,
    // power down the other clocks, and bring things back up in an
    // orderly fashion.  This costs a few hundred microseconds.

    // switch CPU clock to 12MHz internal osc
    writel(readl(BASE_M4_CLK) | BASE_AUTOBLOCK, BASE_M4_CLK);
    writel(BASE_CLK_SEL(CLK_IRC) | BASE_AUTOBLOCK, BASE_M4_CLK);

    // Disable PLL1, if it was already running
    writel(PLL1_CTRL_PD, PLL1_CTRL);

    // Disable PLL0USB, if it was already running
    writel(PLL0_CTRL_PD, PLL0USB_CTRL);

    // Disable XTAL osc if it was already running
    writel(readl(XTAL_OSC_CTRL) | 1, XTAL_OSC_CTRL);
    // Disable BYPASS or HF modes:
    writel(1, XTAL_OSC_CTRL);
    // Enable, HF=0 BYPASS=0
    writel(0, XTAL_OSC_CTRL);
    // Wait
    spin_cycles(3000); // 250uS @ 12MHz

    // PLL1: 12MHz -> N=(/2) -> M=(x32) -> P=(/2)   96MHz
    cfg = PLL1_CTRL_NSEL_2 | PLL1_CTRL_PSEL_1 | PLL1_CTRL_MSEL(32) |
          PLL1_CTRL_CLK_SEL(CLK_XTAL) | PLL1_CTRL_AUTOBLOCK;
    writel(cfg, PLL1_CTRL);
    while (!(readl(PLL1_STAT) & PLL1_STAT_LOCK)) ;

    writel(BASE_CLK_SEL(CLK_PLL1) | BASE_AUTOBLOCK, BASE_M4_CLK);

    // when moving from < 90 MHz to > 110MHz, must spend 50uS
    // at 90-110MHz before shifting to high speeds
    spin_cycles(4800); // 50uS @ 96MHz

    // disable P divider  192MHz
    writel(cfg | PLL1_CTRL_DIRECT, PLL1_CTRL);

    // 12MHz -> 480MHz settings, per boot rom
    writel(0x01967FFA, PLL0USB_MDIV);
    writel(0x00302062, PLL0USB_NP_DIV);
    // Enable PLL, wait for lock
    cfg = PLL0_CTRL_CLK_SEL(CLK_XTAL) | PLL0_CTRL_DIRECTO | PLL0_CTRL_AUTOBLOCK;
    writel(cfg, PLL0USB_CTRL);
    while (!(readl(PLL0USB_STAT) & PLL0_STAT_LOCK)) ;
    // Enable clock output
    writel(cfg | PLL0_CTRL_CLKEN, PLL0USB_CTRL);

#if 0
    // route PLL1 / 2 to CLK0 pin for verification
    writel(0x11, 0x40086C00); // CLK0 = CLK_OUT, no PU/PD
    writel(IDIV_CLK_SEL(CLK_PLL1) | IDIV_N(2), IDIVE_CTRL);
    writel(BASE_CLK_SEL(CLK_IDIVE), BASE_OUT_CLK);
#endif
#if 0
    // route PLL0USB / 4 to CLK0 pin for verification
    writel(0x11, 0x40086C00); // CLK0 = CLK_OUT, no PU/PD
    writel(IDIV_CLK_SEL(CLK_PLL0USB) | IDIV_N(4), IDIVA_CTRL);
    writel(BASE_CLK_SEL(CLK_IDIVA), BASE_OUT_CLK);
#endif
    __lpc43xx_main_clock_mhz = 192000000;
    __lpc43xx_main_clock_sel = CLK_PLL1;
#else
    __lpc43xx_main_clock_mhz = 96000000;
    __lpc43xx_main_clock_sel = CLK_IDIVC;
#endif
    arm_cm_systick_init(__lpc43xx_main_clock_mhz);
    lpc43xx_debug_early_init();
}

void lpc43xx_usb_init(u32 dmabase, size_t dmasize);

void platform_init(void)
{
    lpc43xx_debug_init();
    lpc43xx_usb_init(0x20000000, 4096);
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
    for (;;);
}
