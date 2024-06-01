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
#include <arch.h>

#include "arch/riscv/feature.h"
#include "riscv_priv.h"

#define LOCAL_TRACE 0

// per cpu structure, pointed to by xscratch
struct riscv_percpu percpu[SMP_MAX_CPUS];

// called extremely early from start.S prior to getting into any other C code on
// both the boot cpu and the secondaries
void riscv_configure_percpu_early(uint hart_id, uint __unused, uint cpu_num);
void riscv_configure_percpu_early(uint hart_id, uint __unused, uint cpu_num) {
    // point tp reg at the current cpu structure
    riscv_set_percpu(&percpu[cpu_num]);

    // set up the cpu number and hart id for the per cpu structure
    percpu[cpu_num].cpu_num = cpu_num;
    percpu[cpu_num].hart_id = hart_id;
    wmb();

#if WITH_SMP
    // do any MP percpu config
    riscv_configure_percpu_mp_early(hart_id, cpu_num);
#endif
}

// first C level code to initialize each cpu
void riscv_early_init_percpu(void) {
    // clear the scratch register in case we take an exception early
    riscv_csr_write(RISCV_CSR_XSCRATCH, 0);

    // set the top level exception handler
    riscv_csr_write(RISCV_CSR_XTVEC, (uintptr_t)&riscv_exception_entry);

    // mask all exceptions, just in case
    riscv_csr_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE);
    riscv_csr_clear(RISCV_CSR_XIE, RISCV_CSR_XIE_SIE | RISCV_CSR_XIE_TIE | RISCV_CSR_XIE_EIE);

#if RISCV_FPU
    // enable the fpu and zero it out
    riscv_csr_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_FS_MASK);
    riscv_csr_set(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_FS_INITIAL);

    riscv_fpu_zero();
#endif

    // enable cycle counter (disabled for now, unimplemented on sifive-e)
    //riscv_csr_set(mcounteren, 1);
}

// called very early just after entering C code on boot processor
void arch_early_init(void) {
    riscv_early_init_percpu();

    riscv_feature_early_init();

#if RISCV_S_MODE
    sbi_early_init();
#endif
#if RISCV_MMU
    riscv_early_mmu_init();
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
    const char *mode_string;
#if RISCV_M_MODE
    mode_string = "Machine";
#elif RISCV_S_MODE
    mode_string = "Supervisor";
#else
#error need to define M or S mode
#endif

    dprintf(INFO, "RISCV: %s mode\n", mode_string);
    dprintf(INFO, "RISCV: mvendorid %#lx marchid %#lx mimpid %#lx mhartid %#x\n",
            riscv_get_mvendorid(), riscv_get_marchid(),
            riscv_get_mimpid(), riscv_current_hart());

    riscv_feature_init();

#if RISCV_M_MODE
    dprintf(INFO, "RISCV: misa %#lx\n", riscv_csr_read(RISCV_CSR_MISA));
#elif RISCV_S_MODE
    sbi_init();
#if RISCV_MMU
    dprintf(INFO, "RISCV: MMU enabled sv%u\n", RISCV_MMU);
    riscv_mmu_init();
#endif
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
    kernel_stack_top = ROUNDDOWN(kernel_stack_top, 16);

    printf("kernel sstatus %#lx\n", riscv_csr_read(sstatus));

    // build a user status register
    ulong status;
    status = RISCV_CSR_XSTATUS_PIE |
             RISCV_CSR_XSTATUS_SUM;

    printf("user sstatus %#lx\n", status);

    arch_disable_ints();

    riscv_csr_write(sstatus, status);
    riscv_csr_write(sepc, entry_point);
    riscv_csr_write(sscratch, kernel_stack_top);

#if RISCV_FPU
    status |= RISCV_CSR_XSTATUS_FS_INITIAL; // mark fpu state 'initial'
    riscv_fpu_zero();
#endif

    // put the current tp (percpu pointer) just below the top of the stack
    // the exception code will recover it when coming from user space
    ((uintptr_t *)kernel_stack_top)[-1] = (uintptr_t)riscv_get_percpu();
    asm volatile(
        // set the user stack pointer
        "mv  sp, %0\n"
        // zero out the rest of the integer state
        "li  a0, 0\n"
        "li  a1, 0\n"
        "li  a2, 0\n"
        "li  a3, 0\n"
        "li  a4, 0\n"
        "li  a5, 0\n"
        "li  a6, 0\n"
        "li  a7, 0\n"
        "li  t0, 0\n"
        "li  t1, 0\n"
        "li  t2, 0\n"
        "li  t3, 0\n"
        "li  t4, 0\n"
        "li  t5, 0\n"
        "li  t6, 0\n"
        "li  s0, 0\n"
        "li  s1, 0\n"
        "li  s2, 0\n"
        "li  s3, 0\n"
        "li  s4, 0\n"
        "li  s5, 0\n"
        "li  s6, 0\n"
        "li  s7, 0\n"
        "li  s8, 0\n"
        "li  s9, 0\n"
        "li  s10, 0\n"
        "li  s11, 0\n"
        "li  ra, 0\n"
        "li  gp, 0\n"
        "li  tp, 0\n"
        "sret"
        :: "r" (user_stack_top)
    );

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
