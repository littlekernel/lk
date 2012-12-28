/*
 * Copyright (c) 2009 Corey Tabaka
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
#include <arch.h>
#include <arch/ops.h>
#include <arch/x86.h>
#include <arch/x86/mmu.h>
#include <arch/x86/descriptor.h>
#include <platform.h>
#include <sys/types.h>
#include <string.h>

static tss_t system_tss;

void arch_early_init(void)
{
	x86_mmu_init();

	platform_init_mmu_mappings();

	/* enable caches here for now */
	clear_in_cr0(X86_CR0_NW | X86_CR0_CD);

	memset(&system_tss, 0, sizeof(tss_t));

	system_tss.esp0 = 0;
	system_tss.ss0 = DATA_SELECTOR;
	system_tss.ss1 = 0;
	system_tss.ss2 = 0;
	system_tss.eflags = 0x00003002;
	system_tss.bitmap = offsetof(tss_t, tss_bitmap);
	system_tss.trace = 1; // trap on hardware task switch

	set_global_desc(TSS_SELECTOR, &system_tss, sizeof(tss_t), 1, 0, 0, SEG_TYPE_TSS, 0, 0);

	x86_ltr(TSS_SELECTOR);
}

void arch_init(void)
{
}


