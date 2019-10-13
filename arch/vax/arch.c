/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <assert.h>
#include <lk/trace.h>
#include <lk/debug.h>
#include <stdint.h>
#include <arch/ops.h>
#include <arch/vax.h>

#define LOCAL_TRACE 0

void arch_early_init(void) {
    // set the top level exception handler
    //riscv_csr_write(mtvec, (uintptr_t)&riscv_exception_entry);

    // mask all exceptions, just in case
    //riscv_csr_clear(mstatus, RISCV_STATUS_MIE);
    //riscv_csr_clear(mie, RISCV_MIE_MTIE | RISCV_MIE_MSIE | RISCV_MIE_SEIE | RISCV_MIE_MEIE);

    // enable cycle counter (disabled for now, unimplemented on sifive-e)
    //riscv_csr_set(mcounteren, 1);
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
    // disabled for now, QEMU seems to have some trouble emulating wfi properly
    // also have trouble breaking into sifive-e board with openocd when wfi
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
