/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/trace.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <arch/or1k.h>
#include <arch/ops.h>
#include <arch/mmu.h>

void arch_early_init(void) {
}

void arch_init(void) {
    TRACE;
}

void arch_idle(void) {
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}
