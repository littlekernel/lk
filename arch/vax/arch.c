/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <assert.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <stdint.h>
#include <arch/ops.h>
#include <arch/vax.h>

#define LOCAL_TRACE 0

// initial boot stack that start.S leaves us on
extern uint8_t boot_stack[1024];

uint32_t SCB[scb_max_index] __ALIGNED(512);

struct vax_pcb pcb;

extern void vax_undefined_exception(void);
extern void vax_exception_table(void);

static void dump_pr(const char *name, int reg) {
    printf("%s\t%#x\n", name, mfpr(reg));
}
#define dump_pr_byname(name) \
    dump_pr(#name, PR_##name)

static void dump_regs(void) {
    dump_pr_byname(SID);

    dump_pr_byname(KSP);    // Kernel Stack Pointer
    dump_pr_byname(ESP);    // Executive Stack Pointer
    dump_pr_byname(SSP);    // Supervisor Stack Pointer
    dump_pr_byname(USP);    // User Stack Pointer
    dump_pr_byname(ISP);    // Interrupt Stack Pointer

    dump_pr_byname(P0BR);   // P0 Base Register
    dump_pr_byname(P0LR);   // P0 Length Register
    dump_pr_byname(P1BR);   // P1 Base Register
    dump_pr_byname(P1LR);   // P1 Length Register
    dump_pr_byname(SBR);    // System Base Register
    dump_pr_byname(SLR);    // System Limit Register

    dump_pr_byname(PCBB);   // Process Control Block Base
    dump_pr_byname(SCBB);   // System Control Block Base
    dump_pr_byname(IPL);    // Interrupt Priority Level

    dump_pr_byname(MAPEN);  // Memory Management Enable
}

void arch_early_init(void) {

    // initialize the SCB
    for (int i = 0; i < scb_max_index; i++) {
        SCB[i] = ((uint32_t)&vax_exception_table + (i * 16)) | SCB_FLAG_KERNEL_STACK;

    }
    mtpr((uint32_t)SCB, PR_SCBB);
    mtpr((uint32_t)&pcb, PR_PCBB);

    dump_regs();
}

void arch_init(void) {
    // print some arch info
    //dprintf(INFO, "RISCV: mvendorid %#lx marchid %#lx mimpid %#lx mhartid %#lx\n",
    //        riscv_csr_read(mvendorid), riscv_csr_read(marchid),
    //        riscv_csr_read(mimpid), riscv_csr_read(mhartid));
    //dprintf(INFO, "RISCV: misa %#lx\n", riscv_csr_read(misa));

    // enable external interrupts
    //riscv_csr_set(mie, RISCV_MIE_MEIE);
}

void arch_idle(void) {
//    __asm__ volatile("wfi");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

/* unimplemented cache operations */
void arch_disable_cache(uint flags) { PANIC_UNIMPLEMENTED; }
void arch_enable_cache(uint flags) { PANIC_UNIMPLEMENTED; }

void arch_clean_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_clean_invalidate_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_invalidate_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_sync_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
