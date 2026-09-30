/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/x86/mmu.h>
#include <kernel/mp.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

/* past this many pages a full flush is cheaper than a page at a time */
#define X86_TLB_SHOOTDOWN_MAX_PAGES 64

struct x86_tlb_shootdown_args {
    vaddr_t vaddr;
    uint count;
};

static void x86_tlb_shootdown_task(void *arg) {
    const struct x86_tlb_shootdown_args *args = arg;

    if (args->count > X86_TLB_SHOOTDOWN_MAX_PAGES) {
        /* Reloading cr3 leaves global entries alone, and kernel pages are
         * global, so turn PGE off and on around it when it is in use. A cpu
         * without PGE may have no cr4 at all. */
        if (x86_feature_test(X86_FEATURE_PGE)) {
            ulong cr4 = x86_get_cr4();
            if (cr4 & X86_CR4_PGE) {
                x86_set_cr4(cr4 & ~X86_CR4_PGE);
                x86_set_cr4(cr4);
                return;
            }
        }
        x86_set_cr3(x86_get_cr3());
        return;
    }

    vaddr_t vaddr = args->vaddr;
    for (uint i = 0; i < args->count; i++, vaddr += PAGE_SIZE) {
        tlbsync_local(vaddr);
    }
}

void x86_tlb_shootdown(vaddr_t vaddr, uint count) {
    struct x86_tlb_shootdown_args args = { .vaddr = vaddr, .count = count };

    LTRACEF("vaddr %#lx count %u\n", vaddr, count);

    /* Every cpu, the local one included: it may have moved since the local
     * invalidates the unmap itself did. invlpg on a cpu running another aspace
     * only drops a translation that would be refilled anyway. */
    mp_sync_exec(MP_IPI_TARGET_ALL, 0, x86_tlb_shootdown_task, &args);
}
