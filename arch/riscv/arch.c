/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <trace.h>
#include <debug.h>
#include <stdint.h>
#include <arch/riscv.h>

#define LOCAL_TRACE 0

void arch_early_init(void) {
    // set the top level exception handler
    riscv_csr_write(mtvec, (uintptr_t)&riscv_exception_entry);

    // mask all exceptions, just in case
    riscv_csr_clear(mstatus, RISCV_STATUS_MIE);
    riscv_csr_clear(mie, RISCV_MIE_MTIE | RISCV_MIE_MSIE | RISCV_MIE_SEIE | RISCV_MIE_MEIE);

    // enable cycle counter
    riscv_csr_set(mcounteren, 1);
}

void arch_init(void) {
    // enable external interrupts
    riscv_csr_set(mie, RISCV_MIE_MEIE);
}

void arch_idle(void) {
    // disabled for now, QEMU seems to have some trouble emulating wfi properly
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
