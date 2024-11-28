/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/x86.h>
#include <arch/x86/mmu.h>
#include <arch/x86/descriptor.h>
#include <arch/x86/feature.h>
#include <arch/fpu.h>
#include <arch/mmu.h>
#include <kernel/vm.h>
#include <platform.h>
#include <sys/types.h>
#include <string.h>

/* Describe how start.S sets up the MMU.
 * These data structures are later used by vm routines to lookup pointers
 * to physical pages based on physical addresses.
 */
struct mmu_initial_mapping mmu_initial_mappings[] = {
#if ARCH_X86_64
    /* 64GB of memory mapped where the kernel lives */
    {
        .phys = MEMBASE,
        .virt = KERNEL_ASPACE_BASE,
        .size = PHYSMAP_SIZE, /* x86-64 maps first 64GB by default, 1GB on x86-32 */
        .flags = 0,
        .name = "physmap"
    },
#endif
    /* 1GB of memory mapped where the kernel lives */
    {
        .phys = MEMBASE,
        .virt = KERNEL_BASE,
#if X86_LEGACY
        .size = 16*MB, /* only map the first 16MB on legacy x86 due to page table usage */
#else
        .size = 1*GB, /* x86 maps first 1GB by default */
#endif
        .flags = 0,
        .name = "kernel"
    },

    /* null entry to terminate the list */
    { 0 }
};

/* early stack */
uint8_t _kstack[PAGE_SIZE] __ALIGNED(sizeof(unsigned long));

/* save a pointer to the multiboot information coming in from whoever called us */
/* make sure it lives in .data to avoid it being wiped out by bss clearing */
__SECTION(".data") uint32_t _multiboot_info;

/* main tss */
static tss_t system_tss __ALIGNED(16);

/* early initialization of the system, on the boot cpu, usually before any sort of
 * printf output is available.
 */
void arch_early_init(void) {
    /* enable caches here for now */
    clear_in_cr0(X86_CR0_NW | X86_CR0_CD);

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

    x86_feature_early_init();

    x86_mmu_early_init();

#if X86_WITH_FPU
    x86_fpu_early_init();
#endif
}

/* later initialization pass, once the main kernel is initialized and scheduling has begun */
void arch_init(void) {
    x86_feature_init();
    x86_mmu_init();

#if X86_WITH_FPU
    x86_fpu_init();
#endif
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

void arch_enter_uspace(vaddr_t entry_point, vaddr_t user_stack_top) {
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
