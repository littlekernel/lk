// Copyright 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include <lib/acpi.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <stdio.h>

#include <uacpi/acpi.h>
#include <uacpi/sleep.h>
#include <uacpi/tables.h>
#include <uacpi/uacpi.h>

status_t acpi_init(void) {
    uacpi_status status = uacpi_initialize(0);
    if (status != UACPI_STATUS_OK) {
        dprintf(ALWAYS, "uACPI: failed to initialize: %s (%d)\n", uacpi_status_to_string(status),
                status);
        return ERR_NOT_FOUND;
    }

    status = uacpi_namespace_load();
    if (status != UACPI_STATUS_OK) {
        dprintf(ALWAYS, "uACPI: failed to load namespace: %s (%d)\n",
                uacpi_status_to_string(status), status);
        uacpi_state_reset();
        return ERR_GENERIC;
    }

    status = uacpi_namespace_initialize();
    if (status != UACPI_STATUS_OK) {
        dprintf(ALWAYS, "uACPI: failed to initialize namespace: %s (%d)\n",
                uacpi_status_to_string(status), status);
        uacpi_state_reset();
        return ERR_GENERIC;
    }

    return NO_ERROR;
}

const struct acpi_sdt_hdr *acpi_get_table_by_sig(const char *sig) {
    uacpi_table tbl;
    uacpi_status status = uacpi_table_find_by_signature(sig, &tbl);
    if (status != UACPI_STATUS_OK) {
        return NULL;
    }
    // We intentionally do NOT call uacpi_table_unref(&tbl) here to keep the table mapped
    // in virtual memory for the caller.
    return tbl.hdr;
}

void acpi_dump_tables(bool full_dump) {
    uacpi_size count = uacpi_table_count();
    printf("root table (%zu tables):\n", count);
    for (uacpi_size i = 0; i < count; i++) {
        uacpi_table tbl;
        uacpi_status status = uacpi_table_get_by_index(i, &tbl);
        if (status == UACPI_STATUS_OK && tbl.hdr) {
            printf("table %zu: '%.4s' len %u\n", i, tbl.hdr->signature, tbl.hdr->length);
            if (full_dump) {
                hexdump(tbl.hdr, tbl.hdr->length);
            }
            uacpi_table_unref(&tbl);
        }
    }
}

status_t acpi_process_madt_entries(uacpi_u8 type, acpi_madt_entry_callback callback, void *cookie) {
    uacpi_table tbl;
    uacpi_status status = uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &tbl);
    if (status != UACPI_STATUS_OK) {
        return ERR_NOT_FOUND;
    }

    struct acpi_madt *madt = (struct acpi_madt *)tbl.hdr;
    if (!madt) {
        uacpi_table_unref(&tbl);
        return ERR_NOT_FOUND;
    }

    uacpi_u8 *ptr = (uacpi_u8 *)madt->entries;
    uacpi_u8 *end = (uacpi_u8 *)madt + madt->hdr.length;

    while (ptr < end) {
        struct acpi_entry_hdr *entry = (struct acpi_entry_hdr *)ptr;
        if (entry->length < sizeof(struct acpi_entry_hdr)) {
            break;
        }

        if (entry->type == type) {
            callback(entry, cookie);
        }

        ptr += entry->length;
    }

    uacpi_table_unref(&tbl);
    return NO_ERROR;
}

static void dump_local_apic_cb(const struct acpi_entry_hdr *hdr, void *cookie) {
    const struct acpi_madt_lapic *entry = (const struct acpi_madt_lapic *)hdr;
    printf("\tLOCAL APIC id %d, processor id %d, flags %#x\n", entry->id, entry->uid, entry->flags);
}

static void dump_io_apic_cb(const struct acpi_entry_hdr *hdr, void *cookie) {
    const struct acpi_madt_ioapic *entry = (const struct acpi_madt_ioapic *)hdr;
    printf("\tIO APIC id %d, address %#x gsi base %u\n", entry->id, entry->address,
           entry->gsi_base);
}

static void dump_int_override_cb(const struct acpi_entry_hdr *hdr, void *cookie) {
    const struct acpi_madt_interrupt_source_override *entry =
        (const struct acpi_madt_interrupt_source_override *)hdr;
    printf("\tINT OVERRIDE bus %u, source %u, gsi %u, flags %#x\n", entry->bus, entry->source,
           entry->gsi, entry->flags);
}

void acpi_dump_madt_table(void) {
    printf("MADT/APIC table:\n");
    acpi_process_madt_entries(ACPI_MADT_ENTRY_TYPE_LAPIC, dump_local_apic_cb, NULL);
    acpi_process_madt_entries(ACPI_MADT_ENTRY_TYPE_IOAPIC, dump_io_apic_cb, NULL);
    acpi_process_madt_entries(ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE, dump_int_override_cb,
                              NULL);
}

status_t acpi_shutdown(void) {
    uacpi_status status = uacpi_enter_sleep_state_simple(UACPI_SLEEP_STATE_S5);
    return (status == UACPI_STATUS_OK) ? NO_ERROR : ERR_GENERIC;
}

status_t acpi_reboot(void) {
    uacpi_status status = uacpi_reboot();
    return (status == UACPI_STATUS_OK) ? NO_ERROR : ERR_GENERIC;
}
