/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "platform_p.h"

#include <lk/main.h>
#include <lib/acpi_lite.h>
#include <lk/trace.h>

#define LOCAL_TRACE 1

static void start_cpu(uint cpu_num, uint32_t apic_id) {
    LTRACEF("cpu_num %u, apic_id %u\n", cpu_num, apic_id);

    // XXX do work here
}

struct detected_cpus {
    uint32_t num_detected;
    uint32_t apic_ids[SMP_MAX_CPUS];
};

static void local_apic_callback(const void *_entry, size_t entry_len, void *cookie) {
    const struct acpi_madt_local_apic_entry *entry = _entry;
    struct detected_cpus *cpus = cookie;

    if (entry->apic_id == 0) {
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
    if (cpus.num_detected > 1) {
        dprintf(INFO, "PC: detected %u cpus\n", cpus.num_detected);

        lk_init_secondary_cpus(cpus.num_detected - 1);

        for (uint i = 1; i < cpus.num_detected; i++) {
            dprintf(INFO, "PC: starting cpu %u\n", cpus.apic_ids[i]);
            start_cpu(i, cpus.apic_ids[i]);
        }
    }
}

