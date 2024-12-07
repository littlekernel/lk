/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/mp.h>

#include <assert.h>
#include <lk/err.h>
#include <arch/mp.h>
#include <arch/x86.h>
#include <arch/x86/descriptor.h>
#include <arch/arch_ops.h>
#include <sys/types.h>

// the boot cpu's percpu struct
static x86_percpu_t x86_boot_percpu;
// pointer to an array of percpu structs for each of the secondary cpus
static x86_percpu_t **x86_ap_percpus;

x86_percpu_t *x86_get_percpu_for_cpu(uint cpu_num) {
    DEBUG_ASSERT(cpu_num < SMP_MAX_CPUS);
    if (cpu_num == 0) {
        return &x86_boot_percpu;
    }
    DEBUG_ASSERT(x86_ap_percpus);
    return x86_ap_percpus[cpu_num - 1];
}

void x86_percpu_init_early(uint cpu_num, uint apic_id) {
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
    uint16_t selector = PERCPU_SELECTOR_BASE + cpu_num;
    x86_set_gdt_descriptor(selector, percpu, sizeof(*percpu), 1, 0, 1, SEG_TYPE_DATA_RW, 0, 1);
    x86_set_gs(selector);
#endif
    __UNUSED volatile uint foo = x86_get_cpu_num();
}

status_t arch_mp_send_ipi(mp_cpu_mask_t target, mp_ipi_t ipi) {
    PANIC_UNIMPLEMENTED;
}

void arch_mp_init_percpu(void) {
}
