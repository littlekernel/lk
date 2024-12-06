/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/mp.h>

#include <assert.h>
#include <arch/x86.h>
#include <sys/types.h>

// the boot cpu's percpu struct
static x86_percpu_t x86_boot_percpu;
// pointer to an array of percpu structs for each of the secondary cpus
static x86_percpu_t **x86_ap_percpus;

static x86_percpu_t *percpu_for_cpu(uint cpu_num) {
    DEBUG_ASSERT(cpu_num < SMP_MAX_CPUS);
    if (cpu_num == 0) {
        return &x86_boot_percpu;
    }
    DEBUG_ASSERT(x86_ap_percpus);
    return x86_ap_percpus[cpu_num - 1];
}

void x86_percpu_init_early(uint cpu_num, uint apic_id) {
    x86_percpu_t *percpu = percpu_for_cpu(cpu_num);

    percpu->self = percpu;
    percpu->cpu_num = cpu_num;
    percpu->apic_id = apic_id;

    // XXX load into gs:/fs/etc
#if ARCH_X86_64
    write_msr(X86_MSR_IA32_KERNEL_GS_BASE, 0);
    write_msr(X86_MSR_IA32_GS_BASE, (uint64_t)percpu);
#else
//#error implement
#endif
}