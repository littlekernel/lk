/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "platform_p.h"

#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lk/err.h>
#include <lk/main.h>
#include <lk/trace.h>
#include <string.h>
#include <arch/x86/apic.h>

#if WITH_SMP

#include <lib/acpi_lite.h>

#define TRAMPOLINE_ADDRESS 0x4000

#define LOCAL_TRACE 1

extern void mp_boot_start(void);
extern void mp_boot_end(void);

struct bootstrap_args {
    // referenced in mp-boot.S, do not move without updating assembly
    uintptr_t trampoline_cr3;
    uintptr_t stack_top;

    // referenced in C, okay to move
    uintptr_t cpu_num;
    volatile uint32_t *boot_completed_ptr; // set by the secondary cpu when it's done
};

// called from assembly code in mp-boot.S
__NO_RETURN void secondary_entry(struct bootstrap_args *args) {
    volatile uint32_t *boot_completed = args->boot_completed_ptr;
    uint cpu_num = args->cpu_num;

    // context switch to the kernels cr3
    x86_set_cr3(vmm_get_kernel_aspace()->arch_aspace.cr3_phys);
    // from now on out the boot args structure is not visible

    // we're done, let the primary cpu know so it can reuse the args
    *boot_completed = 1;

    x86_secondary_entry(cpu_num);
}

static status_t start_cpu(uint cpu_num, uint32_t apic_id, struct bootstrap_args *args) {
    LTRACEF("cpu_num %u, apic_id %u\n", cpu_num, apic_id);

    // assert that this thread is pinned to the current cpu
    DEBUG_ASSERT(thread_pinned_cpu(get_current_thread()) == (int)arch_curr_cpu_num());

    volatile uint32_t boot_completed = 0;
    args->boot_completed_ptr = &boot_completed;

    // start x86 secondary cpu

    // send INIT IPI
    lapic_send_init_ipi(apic_id, true);
    thread_sleep(10);

    // deassert INIT
    lapic_send_init_ipi(apic_id, false);
    thread_sleep(10);

    // send Startup IPI up to 2 times as recommended by Intel
    for (int i = 0; i < 2; i++) {
        lapic_send_startup_ipi(apic_id, TRAMPOLINE_ADDRESS);

        // Wait a little bit for the cpu to start before trying a second time
        thread_sleep(10);
        if (boot_completed) {
            goto booted;
        }
    }

    // Wait up to a second for the cpu to finish starting
    for (int i = 0; i < 1000; i++) {
        if (boot_completed) {
            goto booted;
        }
        thread_sleep(10);
    }

    // we have failed to start this core
    // TODO: handle trying to shut the core down before moving on.
    printf("PC: failed to start cpu %u\n", cpu_num);
    return ERR_TIMED_OUT;

booted:
    LTRACEF("cpu %u booted\n", cpu_num);
    return NO_ERROR;
}

struct detected_cpus {
    uint32_t num_detected;
    uint32_t apic_ids[SMP_MAX_CPUS];
};

static void local_apic_callback(const void *_entry, size_t entry_len, void *cookie) {
    const struct acpi_madt_local_apic_entry *entry = _entry;
    struct detected_cpus *cpus = cookie;

    if ((entry->flags & ACPI_MADT_FLAG_ENABLED) == 0) {
        return;
    }

    if (entry->apic_id == x86_get_apic_id()) {
        // skip the boot cpu
        return;
    }
    if (cpus->num_detected < SMP_MAX_CPUS) {
        cpus->apic_ids[cpus->num_detected++] = entry->apic_id;
    }
}

void platform_start_secondary_cpus(void) {
    struct detected_cpus cpus;
    cpus.num_detected = 1;
    cpus.apic_ids[0] = 0; // the boot cpu

    acpi_process_madt_entries_etc(ACPI_MADT_TYPE_LOCAL_APIC, &local_apic_callback, &cpus);

    // TODO: fall back to legacy methods if ACPI fails
    // TODO: deal with cpu topology

    // start up the secondary cpus
    if (cpus.num_detected < 2) {
        dprintf(INFO, "PC: no secondary cpus detected\n");
        return;
    }

    // create a new aspace to build an identity map in
    vmm_aspace_t *aspace;
    status_t err = vmm_create_aspace(&aspace, "identity map", 0);
    if (err < 0) {
        panic("failed to create identity map aspace\n");
    }

    // set up an identity map for the trampoline code

    void *ptr = (void *)TRAMPOLINE_ADDRESS;
    err = vmm_alloc_physical(aspace, "trampoline", 0x10000, &ptr, 0,
        TRAMPOLINE_ADDRESS, VMM_FLAG_VALLOC_SPECIFIC, ARCH_MMU_FLAG_CACHED);
    if (err < 0) {
        panic("failed to allocate trampoline memory\n");
    }

    vmm_aspace_t *old_aspace = vmm_set_active_aspace(aspace);

    // set up bootstrap code page at TRAMPOLINE_ADDRESS for secondary cpu
    memcpy(ptr, mp_boot_start, mp_boot_end - mp_boot_start);

    // next page has args in it
    struct bootstrap_args *args = (struct bootstrap_args *)((uintptr_t)ptr + 0x1000);
    args->trampoline_cr3 = aspace->arch_aspace.cr3_phys;

    dprintf(INFO, "PC: detected %u cpus\n", cpus.num_detected);

    lk_init_secondary_cpus(cpus.num_detected - 1);
    err = x86_allocate_percpu_array(cpus.num_detected - 1);
    if (err < 0) {
        panic("failed to allocate percpu array\n");
    }

    for (uint i = 1; i < cpus.num_detected; i++) {
        dprintf(INFO, "PC: starting cpu %u\n", cpus.apic_ids[i]);

        args->cpu_num = i;

        x86_percpu_t *percpu = x86_get_percpu_for_cpu(i);
        args->stack_top = (uintptr_t)percpu->bootstrap_stack + sizeof(percpu->bootstrap_stack);

        LTRACEF("args for cpu %lu: trampoline_cr3 %#lx, stack_top 0x%lx\n", args->cpu_num, args->trampoline_cr3, args->stack_top);

        start_cpu(i, cpus.apic_ids[i], args);
    }

    // restore old aspace
    vmm_set_active_aspace(old_aspace);

    // free the trampoline aspace
    vmm_free_aspace(aspace);
}

#endif // WITH_SMP