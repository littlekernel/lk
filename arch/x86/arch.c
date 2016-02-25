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
#include <arch/mmu.h>
#include <platform.h>
#include <sys/types.h>
#include <string.h>

/* early stack */
uint8_t _kstack[PAGE_SIZE] __ALIGNED(8);

/* save a pointer to the multiboot information coming in from whoever called us */
/* make sure it lives in .data to avoid it being wiped out by bss clearing */
__SECTION(".data") void *_multiboot_info;

/* main tss */
static tss_t system_tss;

void arch_early_init(void)
{
    /* enable caches here for now */
    clear_in_cr0(X86_CR0_NW | X86_CR0_CD);

    memset(&system_tss, 0, sizeof(system_tss));

#if ARCH_X86_32
    system_tss.esp0 = 0;
    system_tss.ss0 = DATA_SELECTOR;
    system_tss.ss1 = 0;
    system_tss.ss2 = 0;
    system_tss.eflags = 0x00003002;
    system_tss.bitmap = offsetof(tss_32_t, tss_bitmap);
    system_tss.trace = 1; // trap on hardware task switch
#endif

    set_global_desc(TSS_SELECTOR, &system_tss, sizeof(system_tss), 1, 0, 0, SEG_TYPE_TSS, 0, 0);
    x86_ltr(TSS_SELECTOR);

    x86_mmu_early_init();
}

void arch_init(void)
{
    x86_mmu_init();

#ifdef X86_WITH_FPU
    fpu_init();
#endif
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3)
{
    PANIC_UNIMPLEMENTED;
}

void arch_enter_uspace(vaddr_t entry_point, vaddr_t user_stack_top)
{
    PANIC_UNIMPLEMENTED;
#if 0
    DEBUG_ASSERT(IS_ALIGNED(user_stack_top, 16));

    thread_t *ct = get_current_thread();

    vaddr_t kernel_stack_top = (uintptr_t)ct->stack + ct->stack_size;
    kernel_stack_top = ROUNDDOWN(kernel_stack_top, 16);

    /* set up a default spsr to get into 64bit user space:
     * zeroed NZCV
     * no SS, no IL, no D
     * all interrupts enabled
     * mode 0: EL0t
     */
    uint32_t spsr = 0;

    arch_disable_ints();

    asm volatile(
        "mov    sp, %[kstack];"
        "msr    sp_el0, %[ustack];"
        "msr    elr_el1, %[entry];"
        "msr    spsr_el1, %[spsr];"
        "eret;"
        :
        : [ustack]"r"(user_stack_top),
        [kstack]"r"(kernel_stack_top),
        [entry]"r"(entry_point),
        [spsr]"r"(spsr)
        : "memory");
    __UNREACHABLE;
#endif
}
