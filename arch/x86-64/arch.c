/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
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
#include <arch/fpu.h>
#include <platform.h>
#include <sys/types.h>
#include <string.h>

static tss_t system_tss;

void arch_early_init(void)
{
    /* enable caches here for now */
    clear_in_cr0(X86_CR0_NW | X86_CR0_CD);

    memset(&system_tss, 0, sizeof(tss_t));

    set_global_desc(TSS_SELECTOR, &system_tss, sizeof(tss_t), 1, 0, 0, SEG_TYPE_TSS, 0, 0);
    x86_ltr(TSS_SELECTOR);
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3)
{
    PANIC_UNIMPLEMENTED;
}

void arch_init(void)
{
#if X86_WITH_FPU
    fpu_init();
#endif
}
