/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <lk/init.h>
#include <assert.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <kernel/spinlock.h>
#include "platform_p.h"
#include <platform/pc.h>
#include <kernel/vm.h>

#define LOCAL_TRACE 0

static bool lapic_present = false;
static uint8_t *lapic_mmio;

void lapic_init(void) {
    // discover the presence of the local apic and map it
    LTRACE_ENTRY;

    // check feature bit 9 in edx of leaf 1 for presence of lapic
    lapic_present = x86_feature_test(X86_FEATURE_APIC);
}

void lapic_init_postvm(uint level) {
    if (!lapic_present)
        return;

    dprintf(INFO, "X86: local apic detected\n");

    // IA32_APIC_BASE_MSR
    uint64_t apic_base = read_msr(0x1b);
    LTRACEF("apic base %#llx\n", apic_base);

    // TODO: assert that it's enabled

    apic_base &= ~0xfff;
    dprintf(INFO, "X86: lapic physical address %#llx\n", apic_base);

    // map the lapic into the kernel since it's not guaranteed that the physmap covers it
    status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "lapic", PAGE_SIZE, (void **)&lapic_mmio, 0,
                             apic_base & ~0xfff, /* vmm_flags */ 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
    ASSERT(err == NO_ERROR);
}

LK_INIT_HOOK(lapic, lapic_init_postvm, LK_INIT_LEVEL_VM);

void lapic_eoi(unsigned int vector) {
    LTRACEF("vector %#x\n", vector);
    if (lapic_present) {
        *REG32(lapic_mmio + 0xb0) = 1;
    }
}

