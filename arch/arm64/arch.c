/*
 * Copyright (c) 2014-2016 Travis Geiselbrecht
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
#include <stdlib.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm64.h>
#include <arch/arm64/mmu.h>
#include <arch/mp.h>
#include <kernel/thread.h>
#include <lk/init.h>
#include <lk/main.h>
#include <platform.h>
#include <target.h>
#include <trace.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0

#if WITH_SMP
/* smp boot lock */
static spin_lock_t arm_boot_cpu_lock = 1;
static volatile int secondaries_to_init = 0;
#endif

static void arm64_cpu_early_init(void)
{
    /* set the vector base */
    ARM64_WRITE_SYSREG(VBAR_EL1, (uint64_t)&arm64_exception_base);

    /* switch to EL1 */
    unsigned int current_el = ARM64_READ_SYSREG(CURRENTEL) >> 2;
    if (current_el > 1) {
        arm64_el3_to_el1();
    }

    arch_enable_fiqs();
}

void arch_early_init(void)
{
    arm64_cpu_early_init();
    platform_init_mmu_mappings();
}

void arch_init(void)
{
#if WITH_SMP
    arch_mp_init_percpu();

    LTRACEF("midr_el1 0x%llx\n", ARM64_READ_SYSREG(midr_el1));

    secondaries_to_init = SMP_MAX_CPUS - 1; /* TODO: get count from somewhere else, or add cpus as they boot */

    lk_init_secondary_cpus(secondaries_to_init);

    LTRACEF("releasing %d secondary cpus\n", secondaries_to_init);

    /* release the secondary cpus */
    spin_unlock(&arm_boot_cpu_lock);

    /* flush the release of the lock, since the secondary cpus are running without cache on */
    arch_clean_cache_range((addr_t)&arm_boot_cpu_lock, sizeof(arm_boot_cpu_lock));
#endif
}

void arch_quiesce(void)
{
}

void arch_idle(void)
{
    __asm__ volatile("wfi");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3)
{
    int ret;
    LTRACEF("entry %p, args 0x%lx 0x%lx 0x%lx 0x%lx\n", entry, arg0, arg1, arg2, arg3);

    /* we are going to shut down the system, start by disabling interrupts */
    arch_disable_ints();

    /* give target and platform a chance to put hardware into a suitable
     * state for chain loading.
     */
    target_quiesce();
    platform_quiesce();

    paddr_t entry_pa;
    paddr_t loader_pa;
    paddr_t loader_payam;

#if WITH_KERNEL_VM
    /* get the physical address of the entry point we're going to branch to */
    entry_pa = vaddr_to_paddr((addr_t)entry);
    if (entry_pa == 0) {
        panic("error translating entry physical address\n");
    }

    /* add the low bits of the virtual address back */
    entry_pa |= ((addr_t)entry & 0xfff);

    LTRACEF("entry pa 0x%lx\n", entry_pa);

    /* figure out the mapping for the chain load routine */
    loader_pa = vaddr_to_paddr((addr_t)&arm_chain_load);
    if (loader_pa == 0) {
        panic("error translating loader physical address\n");
    }

    /* add the low bits of the virtual address back */
    loader_pa |= ((addr_t)&arm_chain_load & 0xfff);

    paddr_t loader_pa_section = ROUNDDOWN(loader_pa, SECTION_SIZE);

    LTRACEF("loader address %p, phys 0x%lx, surrounding large page 0x%lx\n",
            &arm_chain_load, loader_pa, loader_pa_section);

    vmm_aspace_t *myspace;
    ret = vmm_create_aspace(&myspace, "bootload", 0);
    if (ret != 0) {
        panic("Could not create new aspace %d\n", ret);
    }

    get_current_thread()->aspace = myspace;
    thread_sleep(1);

    /* using large pages, map around the target location */
    if ((ret = arch_mmu_map(&myspace->arch_aspace, loader_pa_section,
        loader_pa_section, ( (2 * 1024 * 1024) / PAGE_SIZE), 0)) != 0) {
        panic("Could not map loader into new space %d\n", ret);
    }
#else
    /* for non vm case, just branch directly into it */
    entry_pa = (paddr_t)entry;
    loader_pa = (paddr_t)&arm_chain_load;
#endif

    LTRACEF("disabling instruction/data cache\n");
    arch_disable_cache(UCACHE);

    LTRACEF("branching to physical address of loader, (va --> pa) (%p --> %p)\n",
        loader_pa,          vaddr_to_paddr((addr_t)loader_pa));

    void (*loader)(paddr_t entry, ulong, ulong, ulong, ulong) __NO_RETURN = (void *)loader_pa;
    loader(entry_pa, arg0, arg1, arg2, arg3);
}

/* switch to user mode, set the user stack pointer to user_stack_top, put the svc stack pointer to the top of the kernel stack */
void arch_enter_uspace(vaddr_t entry_point, vaddr_t user_stack_top)
{
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
}

#if WITH_SMP
void arm64_secondary_entry(ulong asm_cpu_num)
{
    uint cpu = arch_curr_cpu_num();
    if (cpu != asm_cpu_num)
        return;

    arm64_cpu_early_init();

    spin_lock(&arm_boot_cpu_lock);
    spin_unlock(&arm_boot_cpu_lock);

    /* run early secondary cpu init routines up to the threading level */
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_THREADING - 1);

    arch_mp_init_percpu();

    LTRACEF("cpu num %d\n", cpu);

    /* we're done, tell the main cpu we're up */
    atomic_add(&secondaries_to_init, -1);
    __asm__ volatile("sev");

    lk_secondary_cpu_entry();
}
#endif

