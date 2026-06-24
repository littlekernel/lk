// Copyright 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <uacpi/uacpi.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>

#include <lk/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

// Initialize the ACPI subsystem, load tables.
status_t acpi_init(void);

// Load the ACPI namespace and initialize objects (done after PCI initialization).
status_t acpi_init_namespace(void);

// Retrieve a parsed system description table by signature.
const struct acpi_sdt_hdr *acpi_get_table_by_sig(const char *sig);

// Dump information about all parsed tables.
void acpi_dump_tables(bool full_dump);

// Dump details of the Multiple APIC Description Table (MADT).
void acpi_dump_madt_table(void);

// Callback type for MADT entry iteration.
typedef void (*acpi_madt_entry_callback)(const struct acpi_entry_hdr *entry, void *cookie);

// Iterate through the MADT sub-tables and invoke the callback for matching types.
status_t acpi_process_madt_entries(uacpi_u8 type, acpi_madt_entry_callback callback, void *cookie);

// Transition the system to S5 sleep state (soft poweroff).
status_t acpi_shutdown(void);

// Reboot the system using the FADT reset register.
status_t acpi_reboot(void);

__END_CDECLS
