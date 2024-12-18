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

#define LOCAL_TRACE 1

static bool lapic_present = false;
static bool lapic_x2apic = false;
static volatile uint32_t *lapic_mmio;

// local apic registers
enum lapic_regs {
    LAPIC_ID = 0x20,
    LAPIC_VERSION = 0x30,
    LAPIC_TPR = 0x80,
    LAPIC_APR = 0x90,
    LAPIC_PPR = 0xa0,
    LAPIC_EOI = 0xb0,
    LAPIC_RRD = 0xc0,
    LAPIC_LDR = 0xd0,
    LAPIC_DFR = 0xe0,
    LAPIC_SVR = 0xf0,
    LAPIC_ISR0 = 0x100,

    LAPIC_TMR0 = 0x180,

    LAPIC_IRR0 = 0x200,

    LAPIC_ESR = 0x280,

    LAPIC_CMCI = 0x2f0,
    LAPIC_ICRLO = 0x300,
    LAPIC_ICRHI = 0x310,
    LAPIC_TIMER = 0x320,
    LAPIC_THERMAL = 0x330,
    LAPIC_PERF = 0x340,
    LAPIC_LINT0 = 0x350,
    LAPIC_LINT1 = 0x360,
    LAPIC_ERROR = 0x370,
    LAPIC_TICR = 0x380,
    LAPIC_TCCR = 0x390,
    LAPIC_DIV = 0x3e0,

    // Extended features
    LAPIC_EXT_FEATURES = 0x400,
    LAPIC_EXT_CONTROL = 0x410,
    LAPIC_EXT_SEOI = 0x420,
    LAPIC_EXT_IER0 = 0x480,
    LAPIC_EXT_LVT0 = 0x500,
};

static uint32_t lapic_read(enum lapic_regs reg) {
    LTRACEF("reg %#x\n", reg);
    DEBUG_ASSERT(reg != LAPIC_ICRLO && reg != LAPIC_ICRHI);
    if (lapic_x2apic) {
        // TODO: do we need barriers here?
        return read_msr(X86_MSR_IA32_X2APIC_BASE + reg / 0x10);
    } else {
        return mmio_read32(lapic_mmio + reg / 4);
    }
}

static void lapic_write(enum lapic_regs reg, uint32_t val) {
    LTRACEF("reg %#x val %#x\n", reg, val);
    DEBUG_ASSERT(reg != LAPIC_ICRLO && reg != LAPIC_ICRHI);
    if (lapic_x2apic) {
        write_msr(X86_MSR_IA32_X2APIC_BASE + reg / 0x10, val);
    } else {
        mmio_write32(lapic_mmio + reg / 4, val);
    }
}

// special case to write to the ICR register
static void lapic_write_icr(uint32_t low, uint32_t apic_id) {
    LTRACEF("%#x apic_id %#x\n", low, apic_id);
    if (lapic_x2apic) {
        write_msr(X86_MSR_IA32_X2APIC_BASE + 0x30, ((uint64_t)apic_id << 32) | low);
    } else {
        lapic_write(LAPIC_ICRHI, apic_id << 24);
        lapic_write(LAPIC_ICRLO, low);
    }
}


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
    uint64_t apic_base = read_msr(X86_MSR_IA32_APIC_BASE);
    LTRACEF("raw apic base msr %#llx\n", apic_base);

    // make sure it's enabled
    if ((apic_base & (1u<<11)) == 0) {
        dprintf(INFO, "X86: enabling lapic\n");
        apic_base |= (1u<<11);
        write_msr(0x1b, apic_base);
    }

    dprintf(INFO, "X86: lapic physical address %#llx\n", apic_base & ~0xfff);

    // see if x2APIC mode is supported and enable
    if (x86_feature_test(X86_FEATURE_X2APIC)) {
        lapic_x2apic = true;
        dprintf(INFO, "X86: local apic supports x2APIC mode\n");

        write_msr(X86_MSR_IA32_APIC_BASE, apic_base | (1u<<10));
    }

    // map the lapic into the kernel since it's not guaranteed that the physmap covers it
    if (!lapic_mmio) {
        dprintf(INFO, "X86: mapping lapic into kernel\n");
        status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "lapic", PAGE_SIZE, (void **)&lapic_mmio, 0,
                                apic_base & ~0xfff, /* vmm_flags */ 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
        ASSERT(err == NO_ERROR);
    }

    // Read the local apic id and version and features
    uint32_t id = lapic_read(LAPIC_ID);
    uint32_t version = lapic_read(LAPIC_VERSION);
    bool eas = version & (1u<<31);
    uint32_t max_lvt = (version >> 16) & 0xff;
    version &= 0xff;
    dprintf(INFO, "X86: local apic id %#x version %#x\n", id, version);
    dprintf(INFO, "X86: local apic max lvt entries %u\n", max_lvt);
    if (eas) {
        dprintf(INFO, "X86: local apic EAS features %#x\n", lapic_read(LAPIC_EXT_FEATURES));
    }
}

LK_INIT_HOOK(lapic, lapic_init_postvm, LK_INIT_LEVEL_VM);

void lapic_eoi(unsigned int vector) {
    LTRACEF("vector %#x\n", vector);
    if (!lapic_present) {
        return;
    }

    lapic_write(LAPIC_EOI, 0);
}

void lapic_send_init_ipi(uint32_t apic_id, bool level) {
    if (!lapic_present) {
        return;
    }

    lapic_write_icr((5u << 8) | (level ? (1u << 14) : 0), apic_id);
}

void lapic_send_startup_ipi(uint32_t apic_id, uint32_t startup_vector) {
    if (!lapic_present) {
        return;
    }

    lapic_write_icr((6u << 8) | (startup_vector >> 12), apic_id);
}

void lapic_send_ipi(uint32_t apic_id, uint32_t vector) {
    if (!lapic_present) {
        return;
    }

    lapic_write_icr(vector, apic_id);
}