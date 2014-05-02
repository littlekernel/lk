/*
 * Copyright (c) 2012 Corey Tabaka
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
#include <err.h>
#include <reg.h>
#include <debug.h>
#include <sys/types.h>
#include <arch/arm/mmu.h>
#include <platform.h>
#include "platform_p.h"
#include <dev/uart.h>

#include <hw_control_AM335x.h>
#include <soc_AM335x.h>
#include <hw_cm_wkup.h>
#include <hw_cm_per.h>
#include <hw_types.h>

void platform_init_mmu_mappings(void)
{
	/* do some memory map initialization */
	addr_t addr;

	arm_mmu_map_section(SDRAM_BASE, 0,
			MMU_MEMORY_L1_TYPE_NORMAL_WRITE_BACK_ALLOCATE |
			MMU_MEMORY_L1_AP_P_NA_U_NA);

	for (addr=SDRAM_BASE; addr < SDRAM_BASE + SDRAM_SIZE; addr += (1024*1024)) {
		arm_mmu_map_section(addr, addr,
				MMU_MEMORY_L2_TYPE_NORMAL_WRITE_BACK_ALLOCATE |
				MMU_MEMORY_L1_AP_P_RW_U_NA);
	}

	for (addr=0x40000000; addr < 0x40000000 + (512*1024*1024); addr += (1024*1024)) {
		arm_mmu_map_section(addr, addr,
				MMU_MEMORY_L1_TYPE_STRONGLY_ORDERED |
				MMU_MEMORY_L1_AP_P_RW_U_NA);
	}
}

static void wait_field(addr_t base, uint32_t reg, uint32_t mask, uint32_t val)
{
	while ((*REG32(base + reg) & mask) != val);
}

static void set_field(addr_t base, uint32_t reg, uint32_t mask, uint32_t bits)
{
	uint32_t val;

	val = *REG32(base + reg) & ~mask;
	*REG32(base + reg) = val |  bits;

	wait_field(base, reg, mask, bits);
}

void per_L3_config(void)
{
	vaddr_t base = SOC_CM_PER_REGS;

	/* configure L3 interface clocks. */
	set_field(base, CM_PER_L3_CLKCTRL,
	          CM_PER_L3_CLKCTRL_MODULEMODE,
	          CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE);

	set_field(base, CM_PER_L3_INSTR_CLKCTRL,
	          CM_PER_L3_INSTR_CLKCTRL_MODULEMODE,
	          CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE);

	set_field(base, CM_PER_OCPWP_CLKCTRL,
	          CM_PER_OCPWP_CLKCTRL_MODULEMODE,
	          CM_PER_OCPWP_CLKCTRL_MODULEMODE_ENABLE);

	set_field(base, CM_PER_L3_CLKSTCTRL,
	          CM_PER_L3_CLKSTCTRL_CLKTRCTRL,
	          CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

	set_field(base, CM_PER_L3S_CLKSTCTRL,
	          CM_PER_L3S_CLKSTCTRL_CLKTRCTRL,
	          CM_PER_L3S_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

	set_field(base, CM_PER_OCPWP_L3_CLKSTCTRL,
	          CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL,
	          CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

	/* wait for completion */
	wait_field(base, CM_PER_L3_CLKCTRL,
	           CM_PER_L3_CLKCTRL_IDLEST,
	           CM_PER_L3_CLKCTRL_IDLEST_FUNC << CM_PER_L3_CLKCTRL_IDLEST_SHIFT);

	wait_field(base, CM_PER_L3_INSTR_CLKCTRL,
	           CM_PER_L3_INSTR_CLKCTRL_IDLEST,
	           CM_PER_L3_INSTR_CLKCTRL_IDLEST_FUNC << CM_PER_L3_INSTR_CLKCTRL_IDLEST_SHIFT);

	wait_field(base, CM_PER_OCPWP_CLKCTRL,
	           CM_PER_OCPWP_CLKCTRL_IDLEST,
	           CM_PER_OCPWP_CLKCTRL_IDLEST_FUNC << CM_PER_OCPWP_CLKCTRL_IDLEST_SHIFT);

	wait_field(base, CM_PER_L3_CLKSTCTRL,
	           CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK,
	           CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK);

	wait_field(base, CM_PER_OCPWP_L3_CLKSTCTRL,
	           CM_PER_OCPWP_L3_CLKSTCTRL_CLKACTIVITY_OCPWP_L3_GCLK,
	           CM_PER_OCPWP_L3_CLKSTCTRL_CLKACTIVITY_OCPWP_L3_GCLK);

	wait_field(base, CM_PER_L3S_CLKSTCTRL,
	           CM_PER_L3S_CLKSTCTRL_CLKACTIVITY_L3S_GCLK,
	           CM_PER_L3S_CLKSTCTRL_CLKACTIVITY_L3S_GCLK);
}

void per_L4_config(void)
{
	vaddr_t base = SOC_CM_PER_REGS;

	/* configure L4 interface clocks. */
	set_field(base, CM_PER_L4LS_CLKCTRL,
	          CM_PER_L4LS_CLKCTRL_MODULEMODE,
	          CM_PER_L4LS_CLKCTRL_MODULEMODE_ENABLE);

	set_field(base, CM_PER_L4LS_CLKSTCTRL,
	          CM_PER_L4LS_CLKSTCTRL_CLKTRCTRL,
	          CM_PER_L4LS_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

	wait_field(base, CM_PER_L4LS_CLKCTRL,
	           CM_PER_L4LS_CLKCTRL_IDLEST,
	           CM_PER_L4LS_CLKCTRL_IDLEST_FUNC << CM_PER_L4LS_CLKCTRL_IDLEST_SHIFT);

	wait_field(base, CM_PER_L4LS_CLKSTCTRL,
	           CM_PER_L4LS_CLKSTCTRL_CLKACTIVITY_L4LS_GCLK,
	           CM_PER_L4LS_CLKSTCTRL_CLKACTIVITY_L4LS_GCLK);
}

void wkup_clk_config(void)
{
	vaddr_t base = SOC_CM_WKUP_REGS;

	/* configure wkup domain */
	set_field(base, CM_WKUP_CONTROL_CLKCTRL,
	          CM_WKUP_CONTROL_CLKCTRL_MODULEMODE,
	          CM_WKUP_CONTROL_CLKCTRL_MODULEMODE_ENABLE);

	set_field(base, CM_WKUP_CLKSTCTRL,
	          CM_WKUP_CLKSTCTRL_CLKTRCTRL,
	          CM_WKUP_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

	set_field(base, CM_WKUP_CM_L3_AON_CLKSTCTRL,
	          CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKTRCTRL,
	          CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

	wait_field(base, CM_WKUP_CONTROL_CLKCTRL,
	           CM_WKUP_CONTROL_CLKCTRL_IDLEST,
	           CM_WKUP_CONTROL_CLKCTRL_IDLEST_FUNC << CM_WKUP_CONTROL_CLKCTRL_IDLEST_SHIFT);

	wait_field(base, CM_WKUP_CM_L3_AON_CLKSTCTRL,
	           CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKACTIVITY_L3_AON_GCLK,
	           CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKACTIVITY_L3_AON_GCLK);

	wait_field(base, CM_WKUP_L4WKUP_CLKCTRL,
	           CM_WKUP_L4WKUP_CLKCTRL_IDLEST,
	           CM_WKUP_L4WKUP_CLKCTRL_IDLEST_FUNC << CM_WKUP_L4WKUP_CLKCTRL_IDLEST_SHIFT);

	wait_field(base, CM_WKUP_CLKSTCTRL,
	           CM_WKUP_CLKSTCTRL_CLKACTIVITY_L4_WKUP_GCLK,
	           CM_WKUP_CLKSTCTRL_CLKACTIVITY_L4_WKUP_GCLK);

	wait_field(base, CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL,
	           CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL_CLKACTIVITY_L4_WKUP_AON_GCLK,
	           CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL_CLKACTIVITY_L4_WKUP_AON_GCLK);
}

void platform_early_init(void)
{
	/* initialize the basic clocks */
	per_L3_config();
	per_L4_config();
	wkup_clk_config();

	/* initialize the tx half of the debug uart */
	platform_init_debug();

	/* initialize the interrupt controller */
	platform_init_interrupts();

	/* initialize the timer */
	platform_init_timer();
}

void platform_init(void)
{
	/* initialize the rest of the debug uart */
	uart_init();
}

