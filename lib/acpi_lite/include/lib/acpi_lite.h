// Copyright 2020 The Fuchsia Authors
// Copyright 2021 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lib/acpi_lite/structs.h>

#include <stdbool.h>
#include <stdint.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

status_t acpi_lite_init(paddr_t rsdt);
void acpi_lite_dump_tables(bool full_dump);
void acpi_lite_dump_madt_table(void);

const struct acpi_sdt_header* acpi_get_table_by_sig(const char* sig);

// A routine to iterate over all the MADT entries of a particular type via a callback
//using MadtEntryCallback = fbl::Function<void(const void* entry, size_t entry_len)>;
typedef void (*madt_entry_callback)(const void* entry, size_t entry_len, void *cookie);
status_t acpi_process_madt_entries_etc(uint8_t search_type, madt_entry_callback, void *cookie);


__END_CDECLS
