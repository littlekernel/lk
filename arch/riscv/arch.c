/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <assert.h>
#include <lk/trace.h>
#include <lk/debug.h>
#include <stdint.h>
#include <stdlib.h>
#include <arch/riscv.h>
#include <arch/ops.h>
#include <arch/mp.h>
#include <lk/init.h>
#include <lk/main.h>
#include <platform.h>

#include "riscv_priv.h"

#define LOCAL_TRACE 0

// per cpu structure, pointed to by xscratch
struct riscv_percpu percpu[SMP_MAX_CPUS];

// called extremely early from start.S prior to getting into any other C code on
// both the boot cpu and the secondaries
void riscv_configure_percpu_early(uint hart_id, uint __unused, uint cpu_num) {
    // point xscratch at the current cpu structure
    // set up the cpu number and hart id for the per cpu structure
    riscv_csr_write(RISCV_CSR_XSCRATCH, &percpu[cpu_num]);
    percpu[cpu_num].cpu_num = cpu_num;
    percpu[cpu_num].hart_id = hart_id;

#if WITH_SMP
    // do any MP percpu config
    riscv_configure_percpu_mp_early(hart_id, cpu_num);
#endif
    wmb();
}

// first C level code to initialize each cpu
void riscv_early_init_percpu(void) {
    // set the top level exception handler
    riscv_csr_write(RISCV_CSR_XTVEC, (uintptr_t)&riscv_exception_entry);

    // mask all exceptions, just in case
    riscv_csr_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE);
    riscv_csr_clear(RISCV_CSR_XIE, RISCV_CSR_XIE_SIE | RISCV_CSR_XIE_TIE | RISCV_CSR_XIE_EIE);

    // enable cycle counter (disabled for now, unimplemented on sifive-e)
    //riscv_csr_set(mcounteren, 1);
}

// called very early just after entering C code on boot processor
void arch_early_init(void) {
    riscv_early_init_percpu();

#if RISCV_S_MODE
    sbi_early_init();
#endif
}

// later init per cpu
void riscv_init_percpu(void) {
    dprintf(INFO, "RISCV: percpu cpu num %#x hart id %#x\n", arch_curr_cpu_num(), riscv_current_hart());
#if WITH_SMP
    // enable software interrupts, used for inter-processor-interrupts
    riscv_csr_set(RISCV_CSR_XIE, RISCV_CSR_XIE_SIE);
#endif

    // enable external interrupts
    riscv_csr_set(RISCV_CSR_XIE, RISCV_CSR_XIE_EIE);
}

// called later once the kernel is running before platform and target init
void arch_init(void) {
    riscv_init_percpu();

    // print some arch info
#if RISCV_M_MODE
    dprintf(INFO, "RISCV: Machine mode\n");
    dprintf(INFO, "RISCV: mvendorid %#lx marchid %#lx mimpid %#lx mhartid %#x\n",
            riscv_get_mvendorid(), riscv_get_marchid(),
            riscv_get_mimpid(), riscv_current_hart());
    dprintf(INFO, "RISCV: misa %#lx\n", riscv_csr_read(RISCV_CSR_MISA));
#else
    dprintf(INFO, "RISCV: Supervisor mode\n");
#if RISCV_MMU
    dprintf(INFO, "RISCV: MMU enabled sv%u\n", RISCV_MMU);
#endif
    sbi_init();
#endif

#if WITH_SMP
    riscv_boot_secondaries();
#endif
}

void arch_idle(void) {
    // let the platform/target disable wfi
#if !RISCV_DISABLE_WFI
    __asm__ volatile("wfi");
#endif
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

#if RISCV_S_MODE
/* switch to user mode, set the user stack pointer to user_stack_top, get into user space */
void arch_enter_uspace(vaddr_t entry_point, vaddr_t user_stack_top) {
    DEBUG_ASSERT(IS_ALIGNED(user_stack_top, 8));

    thread_t *ct = get_current_thread();

    vaddr_t kernel_stack_top = (uintptr_t)ct->stack + ct->stack_size;
    kernel_stack_top = ROUNDDOWN(kernel_stack_top, 8);

    PANIC_UNIMPLEMENTED;

#if 0

    uint32_t spsr = CPSR_MODE_USR;
    spsr |= (entry_point & 1) ? CPSR_THUMB : 0;

    arch_disable_ints();

    asm volatile(
        "ldmia  %[ustack], { sp }^;"
        "msr	spsr, %[spsr];"
        "mov	sp, %[kstack];"
        "movs	pc, %[entry];"
        :
        : [ustack]"r"(&user_stack_top),
        [kstack]"r"(kernel_stack_top),
        [entry]"r"(entry_point),
        [spsr]"r"(spsr)
        : "memory");
#endif
    __UNREACHABLE;
}
#endif

/* unimplemented cache operations */
#if RISCV_NO_CACHE_OPS
void arch_disable_cache(uint flags) { }
void arch_enable_cache(uint flags) { }

void arch_clean_cache_range(addr_t start, size_t len) { }
void arch_clean_invalidate_cache_range(addr_t start, size_t len) { }
void arch_invalidate_cache_range(addr_t start, size_t len) { }
void arch_sync_cache_range(addr_t start, size_t len) { }
#else
void arch_disable_cache(uint flags) { PANIC_UNIMPLEMENTED; }
void arch_enable_cache(uint flags) { PANIC_UNIMPLEMENTED; }

void arch_clean_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_clean_invalidate_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_invalidate_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_sync_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
#endif
