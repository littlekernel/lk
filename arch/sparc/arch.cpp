//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/trace.h>
#include <lk/debug.h>
#include <arch.h>
#include <platform.h>
#include <arch/sparc.h>
#include <arch/interrupts.h>

extern "C" void sparc_vectab(void);

static void sparc_early_init_percpu(void) {
    // set the trap base register to point to our vector table
    __asm__ volatile("wr %0, 0, %%tbr\n"
         "nop\n"
         "nop\n"
         "nop" :: "r"(sparc_vectab));

    // make sure external interrupts are fully masked off
    arch_disable_ints();

    // now that the trap base register is set, enable traps on the cpu
    sparc_write_psr(sparc_read_psr() | SPARC_PSR_ET);
}


void arch_early_init(void) {
    sparc_early_init_percpu();


}

void arch_init(void) {
}

void arch_idle(void) {
#if 0
    /* In SPARC, entering idle state can be done by setting a power-saving register
     * or executing a specific instruction if supported. For now, just nop. */
    __asm__ volatile("nop");
#endif
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

void arch_disable_cache(uint flags) {
    PANIC_UNIMPLEMENTED;
}

void arch_enable_cache(uint flags) {
    PANIC_UNIMPLEMENTED;
}

void arch_clean_cache_range(addr_t start, size_t len) {
    PANIC_UNIMPLEMENTED;
}

void arch_clean_invalidate_cache_range(addr_t start, size_t len) {
    PANIC_UNIMPLEMENTED;
}

void arch_invalidate_cache_range(addr_t start, size_t len) {
    PANIC_UNIMPLEMENTED;
}

void arch_sync_cache_range(addr_t start, size_t len) {
    PANIC_UNIMPLEMENTED;
}
