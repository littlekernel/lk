/*
 * Copyright (c) 2014-2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch.h>
#include <arch/arm64.h>
#include <arch/arm64/mmu.h>
#include <arch/arm64/mp.h>
#include <arch/atomic.h>
#include <arch/mp.h>
#include <arch/ops.h>
#include <assert.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/init.h>
#include <lk/main.h>
#include <lk/trace.h>
#include <platform.h>
#include <stdlib.h>

#include "arm64_priv.h"

#define LOCAL_TRACE 0

/* Defined in start.S. */
extern uint64_t arm64_boot_el;

// initial setup per cpu immediately after entering C code
void arm64_early_init_percpu(void) {
    // set the vector base
    ARM64_WRITE_SYSREG(VBAR_EL1, (uint64_t)&arm64_exception_table);

    // hard set up the SCTLR ignoring what was there before
    uint64_t sctlr = 0;
    sctlr |= (1 << 0);  // M: enable mmu
    sctlr |= (1 << 2);  // C: enable data cache
    sctlr |= (1 << 3);  // S: enable stack alignment check for EL1
    sctlr |= (1 << 4);  // SA0: enable stack alignment check for EL0
    sctlr |= (1 << 8);  // SED: disable access to SETEND instructions in EL0 (or RES1)
    sctlr |= (1 << 11); // EOS: exceptions are context synchronizing (or RES1)
    sctlr |= (1 << 12); // I: enable instruction cache
    sctlr |= (1 << 14); // DZE: enable access to DC ZVA instruction in EL0
    sctlr |= (1 << 15); // UCT: enable user access to CTR_EL0
    sctlr |= (1 << 18); // nTWE: do not trap WFE instructions in EL0
    sctlr |= (1 << 20); // TSCXT: trap access to SCXTNUM_EL0 in EL0 (or RES1)
    sctlr |= (1 << 22); // EIS: exception entry is context synchronizing (or RES1)
    sctlr |= (1 << 23); // SPAN: PSTATE.PAN is left alone on exception entry (or RES1)
    sctlr |= (1 << 26); // UCI: allow EL0 access to cache maintenance instructions
    sctlr |= (1 << 28); // nTLSMD: do not trap load/store multiple instructions to uncached memory in EL0 (or RES1)
    sctlr |= (1 << 29); // LSMAOE: load/store multiple ordering according to armv8.0 (or RES1)
    // all other bits are RES0 and we can ignore for now
    ARM64_WRITE_SYSREG(SCTLR_EL1, sctlr);

    ARM64_WRITE_SYSREG(CPACR_EL1, 0UL); // disable coprocessors

    ARM64_WRITE_SYSREG(MDSCR_EL1, 0UL); // disable debug

    // clear the tpidr registers
    ARM64_WRITE_SYSREG(TPIDR_EL0, 0UL);
    ARM64_WRITE_SYSREG(TPIDRRO_EL0, 0UL);

    // TODO: read feature bits on cpu 0
    // TODO: enable cycle counter if present

    arch_enable_fiqs();
}

// called very early in the main boot sequence on the boot cpu
void arch_early_init(void) {
    arm64_early_init_percpu();

    // allow the platform a chance to inject some mappings
    platform_init_mmu_mappings();
}

// called after the kernel has been initialized and threading is enabled on the boot cpu
void arch_init(void) {
    arm64_mp_init();

    dprintf(INFO, "ARM64: boot EL%llu\n", arm64_get_boot_el());
}

uint64_t arm64_get_boot_el(void) {
    return arm64_boot_el >> 2;
}

void arch_quiesce(void) {
}

void arch_idle(void) {
    __asm__ volatile("wfi");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

/* switch to user mode, set the user stack pointer to user_stack_top, put the svc stack pointer to the top of the kernel stack */
void arch_enter_uspace(vaddr_t entry_point, vaddr_t user_stack_top) {
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
    uint64_t spsr = 0;

    arch_disable_ints();

    asm volatile(
        "mov    sp, %[kstack];"
        "msr    sp_el0, %[ustack];"
        "msr    elr_el1, %[entry];"
        "msr    spsr_el1, %[spsr];"
        "eret;"
        :
        : [ustack] "r"(user_stack_top),
          [kstack] "r"(kernel_stack_top),
          [entry] "r"(entry_point),
          [spsr] "r"(spsr)
        : "memory");
    __UNREACHABLE;
}

void arch_stacktrace(uint64_t fp, uint64_t pc) {
    struct arm64_stackframe frame;

    if (!fp) {
        frame.fp = (uint64_t)__builtin_frame_address(0);
        frame.pc = (uint64_t)arch_stacktrace;
    } else {
        frame.fp = fp;
        frame.pc = pc;
    }

    printf("stack trace:\n");
    while (frame.fp) {
        printf("0x%llx\n", frame.pc);

        /* Stack frame pointer should be 16 bytes aligned */
        if (frame.fp & 0xF) {
            break;
        }

        frame.pc = *((uint64_t *)(frame.fp + 8));
        frame.fp = *((uint64_t *)frame.fp);
    }
}
