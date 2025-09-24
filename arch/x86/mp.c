/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/mp.h>

#include <assert.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/main.h>
#include <arch/mp.h>
#include <string.h>
#include <arch/x86.h>
#include <arch/x86/descriptor.h>
#include <arch/arch_ops.h>
#include <sys/types.h>
#include <arch/x86/apic.h>
#include <arch/x86/feature.h>

#define LOCAL_TRACE 0

#if WITH_SMP

// the boot cpu's percpu struct
static x86_percpu_t x86_boot_percpu;
// pointer to an array of percpu structs for each of the secondary cpus
static x86_percpu_t *x86_ap_percpus;

x86_percpu_t *x86_get_percpu_for_cpu(uint cpu_num) {
    DEBUG_ASSERT(cpu_num < SMP_MAX_CPUS);
    if (cpu_num == 0) {
        return &x86_boot_percpu;
    }
    DEBUG_ASSERT(x86_ap_percpus);
    return &x86_ap_percpus[cpu_num - 1];
}

void x86_configure_percpu_early(uint cpu_num, uint apic_id) {
    x86_percpu_t *percpu = x86_get_percpu_for_cpu(cpu_num);

    // initialize the percpu structure for this cpu
    percpu->self = percpu;
    percpu->cpu_num = cpu_num;
    percpu->apic_id = apic_id;

#if ARCH_X86_64
    // use the 64-bit gs base msr to set up a pointer to the percpu struct
    write_msr(X86_MSR_IA32_KERNEL_GS_BASE, 0);
    write_msr(X86_MSR_IA32_GS_BASE, (uint64_t)percpu);
#else
    // set up a gs descriptor for this cpu
    uint16_t selector = PERCPU_SELECTOR_BASE + cpu_num * 8;
    x86_set_gdt_descriptor(selector, percpu, sizeof(*percpu), 1, 0, 1, SEG_TYPE_DATA_RW, 0, 1);
    x86_set_gs(selector);
#endif
}

status_t arch_mp_send_ipi(mp_cpu_mask_t target, mp_ipi_t ipi) {
    LTRACEF("cpu %u target 0x%x, ipi 0x%x\n", arch_curr_cpu_num(), target, ipi);

    DEBUG_ASSERT(arch_ints_disabled());
    uint curr_cpu_num = arch_curr_cpu_num();

    // translate the target bitmap to apic id
    while (target) {
        uint cpu_num = __builtin_ctz(target);
        target &= ~(1u << cpu_num);

        // skip the current cpu
        if (cpu_num == curr_cpu_num) {
            continue;
        }

        x86_percpu_t *percpu = x86_get_percpu_for_cpu(cpu_num);
        uint32_t apic_id = percpu->apic_id;

        // send the ipi to the target cpu
        lapic_send_ipi(apic_id, ipi);
    }

    return NO_ERROR;
}

void x86_secondary_entry(uint cpu_num) {
    // Read the local apic id from the local apic.
    // NOTE: assumes a local apic is present but since this is a secondary cpu,
    // it should be a safe assumption.
    lapic_enable_on_local_cpu();
    uint32_t apic_id = lapic_get_apic_id();

    x86_configure_percpu_early(cpu_num, apic_id);

    x86_early_init_percpu();

    // run early secondary cpu init routines up to the threading level
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_THREADING - 1);

    dprintf(INFO, "SMP: secondary cpu %u started, apic id %u\n", arch_curr_cpu_num(), apic_id);

    lk_secondary_cpu_entry();

    // should never get here except for an error condition
    for (;;);
}

status_t x86_allocate_percpu_array(uint num_cpus) {
    x86_ap_percpus = memalign(_Alignof(x86_percpu_t), num_cpus * sizeof(x86_percpu_t));
    if (!x86_ap_percpus) {
        return ERR_NO_MEMORY;
    }

    memset(x86_ap_percpus, 0, num_cpus * sizeof(x86_percpu_t));
    return NO_ERROR;
}

#else

void x86_configure_percpu_early(uint cpu_num, uint apic_id) {}

#endif