// Copyright 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include <lib/acpi.h>

#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <stdio.h>
#include <string.h>
#include <uacpi/acpi.h>
#include <uacpi/namespace.h>
#include <uacpi/sleep.h>
#include <uacpi/tables.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>

status_t acpi_init(void) {
    uacpi_status status = uacpi_initialize(0);
    if (status != UACPI_STATUS_OK) {
        dprintf(ALWAYS, "uACPI: failed to initialize: %s (%d)\n", uacpi_status_to_string(status),
                status);
        return ERR_NOT_FOUND;
    }

    return NO_ERROR;
}

status_t acpi_init_namespace(void) {
    uacpi_status status = uacpi_namespace_load();
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

static uacpi_iteration_decision dump_ns_cb(void *user, uacpi_namespace_node *node,
                                           uacpi_u32 depth) {
    for (uacpi_u32 i = 0; i < depth; i++) {
        printf("  ");
    }

    uacpi_object_name name = uacpi_namespace_node_name(node);
    uacpi_object_type type = UACPI_OBJECT_UNINITIALIZED;
    uacpi_namespace_node_type(node, &type);

    const char *type_str = "Unknown";
    switch (type) {
        case UACPI_OBJECT_UNINITIALIZED:
            type_str = "Uninitialized";
            break;
        case UACPI_OBJECT_INTEGER:
            type_str = "Integer";
            break;
        case UACPI_OBJECT_STRING:
            type_str = "String";
            break;
        case UACPI_OBJECT_BUFFER:
            type_str = "Buffer";
            break;
        case UACPI_OBJECT_PACKAGE:
            type_str = "Package";
            break;
        case UACPI_OBJECT_FIELD_UNIT:
            type_str = "FieldUnit";
            break;
        case UACPI_OBJECT_DEVICE:
            type_str = "Device";
            break;
        case UACPI_OBJECT_EVENT:
            type_str = "Event";
            break;
        case UACPI_OBJECT_METHOD:
            type_str = "Method";
            break;
        case UACPI_OBJECT_MUTEX:
            type_str = "Mutex";
            break;
        case UACPI_OBJECT_OPERATION_REGION:
            type_str = "OpRegion";
            break;
        case UACPI_OBJECT_POWER_RESOURCE:
            type_str = "PowerResource";
            break;
        case UACPI_OBJECT_PROCESSOR:
            type_str = "Processor";
            break;
        case UACPI_OBJECT_THERMAL_ZONE:
            type_str = "ThermalZone";
            break;
        case UACPI_OBJECT_BUFFER_FIELD:
            type_str = "BufferField";
            break;
        case UACPI_OBJECT_DEBUG:
            type_str = "Debug";
            break;
        case UACPI_OBJECT_REFERENCE:
            type_str = "Reference";
            break;
        case UACPI_OBJECT_BUFFER_INDEX:
            type_str = "BufferIndex";
            break;
        default:
            type_str = "Other";
            break;
    }

    printf("%.4s [%s]", name.text, type_str);

    if (type == UACPI_OBJECT_DEVICE) {
        uacpi_id_string *hid = NULL;
        if (uacpi_eval_hid(node, &hid) == UACPI_STATUS_OK && hid) {
            printf(" HID: %s", hid->value);
            uacpi_free_id_string(hid);
        }

        uacpi_u32 sta = 0;
        if (uacpi_eval_sta(node, &sta) == UACPI_STATUS_OK) {
            printf(" STA: 0x%X", sta);
        }

        uacpi_u64 adr = 0;
        if (uacpi_eval_adr(node, &adr) == UACPI_STATUS_OK) {
            printf(" ADR: 0x%llX", adr);
        }
    }
    printf("\n");

    return UACPI_ITERATION_DECISION_CONTINUE;
}

static uacpi_iteration_decision list_devices_cb(void *user, uacpi_namespace_node *node,
                                                uacpi_u32 depth) {
    uacpi_object_type type = UACPI_OBJECT_UNINITIALIZED;
    uacpi_namespace_node_type(node, &type);

    if (type != UACPI_OBJECT_DEVICE && type != UACPI_OBJECT_PROCESSOR &&
        type != UACPI_OBJECT_THERMAL_ZONE) {
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    const uacpi_char *path = uacpi_namespace_node_generate_absolute_path(node);
    if (path) {
        printf("Device: %s", path);
        uacpi_free_absolute_path(path);
    } else {
        uacpi_object_name name = uacpi_namespace_node_name(node);
        printf("Device: %.4s", name.text);
    }

    if (type == UACPI_OBJECT_DEVICE) {
        uacpi_id_string *hid = NULL;
        if (uacpi_eval_hid(node, &hid) == UACPI_STATUS_OK && hid) {
            printf(" (HID: %s)", hid->value);
            uacpi_free_id_string(hid);
        }

        uacpi_u32 sta = 0;
        if (uacpi_eval_sta(node, &sta) == UACPI_STATUS_OK) {
            printf(" (STA: 0x%X)", sta);
        }
    } else if (type == UACPI_OBJECT_PROCESSOR) {
        printf(" (Processor)");
    } else if (type == UACPI_OBJECT_THERMAL_ZONE) {
        printf(" (Thermal Zone)");
    }
    printf("\n");

    return UACPI_ITERATION_DECISION_CONTINUE;
}

static int cmd_acpi(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
        printf("usage: %s <command> [args]\n", argv[0].str);
        printf("commands:\n");
        printf("\ttables                dump the ACPI tables\n");
        printf("\tdump_tables           dump the ACPI tables with full hex dump\n");
        printf("\tmadt                  dump the MADT (APIC) table\n");
        printf("\tdevices               list all devices in the ACPI namespace\n");
        printf("\tdump_ns               dump the entire ACPI namespace tree\n");
        printf("\tshutdown              poweroff the system via ACPI\n");
        printf("\treboot                reboot the system via ACPI\n");
        return 0;
    }

    if (strcmp(argv[1].str, "tables") == 0) {
        acpi_dump_tables(false);
    } else if (strcmp(argv[1].str, "dump_tables") == 0) {
        acpi_dump_tables(true);
    } else if (strcmp(argv[1].str, "madt") == 0) {
        acpi_dump_madt_table();
    } else if (strcmp(argv[1].str, "devices") == 0) {
        printf("ACPI Devices:\n");
        uacpi_namespace_for_each_child(uacpi_namespace_root(), list_devices_cb, UACPI_NULL,
                                       UACPI_OBJECT_ANY_BIT, UACPI_MAX_DEPTH_ANY, NULL);
    } else if (strcmp(argv[1].str, "dump_ns") == 0) {
        printf("ACPI Namespace Tree:\n");
        printf("\\ [Root]\n");
        uacpi_namespace_for_each_child(uacpi_namespace_root(), dump_ns_cb, UACPI_NULL,
                                       UACPI_OBJECT_ANY_BIT, UACPI_MAX_DEPTH_ANY, NULL);
    } else if (strcmp(argv[1].str, "shutdown") == 0) {
        acpi_shutdown();
    } else if (strcmp(argv[1].str, "reboot") == 0) {
        acpi_reboot();
    } else {
        printf("unknown command: %s\n", argv[1].str);
    }
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("acpi", "ACPI commands", &cmd_acpi)
STATIC_COMMAND_END(acpi);
