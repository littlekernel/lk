/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// PCI bring up on the PC platform: config space accessors from the MCFG (or the legacy PIO
// mechanisms), root busses from the ACPI namespace, and firmware assignments kept as they are.

#include "platform_p.h"

#if WITH_DEV_BUS_PCI

#include <dev/bus/pci.h>
#include <lib/cmdline.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <platform/pc.h>
#include <stdio.h>
#include <string.h>

#if WITH_LIB_ACPI
#include <lib/acpi.h>
#include <lib/acpi/pci.h>
#endif
#if WITH_DEV_VIRTIO
#include <dev/virtio.h>
#endif

#define LOCAL_TRACE 0

#if WITH_LIB_ACPI
// register the ecam aperture(s) the MCFG describes. returns true if at least one worked.
static bool pc_pci_init_ecam(void) {
    bool have_backend = false;

    const struct acpi_mcfg *table =
        (const struct acpi_mcfg *)acpi_get_table_by_sig(ACPI_MCFG_SIGNATURE);
    if (!table || table->hdr.length < sizeof(*table)) {
        return false;
    }

    size_t count = (table->hdr.length - sizeof(*table)) / sizeof(struct acpi_mcfg_allocation);
    for (size_t i = 0; i < count; i++) {
        const struct acpi_mcfg_allocation *entry = &table->entries[i];
        printf("PCI MCFG: segment %#hx bus [%hhu...%hhu] address %#llx\n", entry->segment,
               entry->start_bus, entry->end_bus, entry->address);

        status_t err = pci_init_ecam(entry->address, entry->segment, entry->start_bus,
                                     entry->end_bus);
        if (err == NO_ERROR) {
            have_backend = true;
        } else {
            printf("PCI MCFG: failed to init ecam entry %zu, error %d\n", i, err);
        }
    }

    return have_backend;
}

// one host bridge found in the namespace
static void pc_pci_add_acpi_root(struct acpi_pci_root *root, void *cookie) {
    size_t *count = cookie;

    if (LK_DEBUGLEVEL >= INFO) {
        acpi_pci_root_dump(root);
    }

    struct pci_root_desc desc = {
        .segment = root->segment,
        .bus_start = root->bus_start,
        .bus_end = root->bus_end,
        .windows = root->windows,
        .num_windows = root->num_windows,
        .intx_route = NULL,
        .intx_cookie = root,
    };

    status_t err = pci_bus_mgr_add_root(&desc);
    if (err != NO_ERROR) {
        printf("PCI: failed to add ACPI root %04x:%02x, error %d\n", root->segment,
               root->bus_start, err);
        return;
    }
    (*count)++;
}
#endif // WITH_LIB_ACPI

void pc_pci_init(bool have_acpi) {
    LTRACE_ENTRY;

    // 1. config space accessors
    bool have_backend = false;
#if WITH_LIB_ACPI
    if (have_acpi) {
        have_backend = pc_pci_init_ecam();
    }
#endif
    if (!have_backend) {
        // fall back to legacy pci if we couldn't find a pcie aperture
        status_t err = pci_init_legacy();
        have_backend = (err == NO_ERROR);
    }

    // 2. the ACPI namespace, which may touch pci config space from here on
    bool have_acpi_roots = false;
#if WITH_LIB_ACPI
    if (have_acpi) {
        status_t err = acpi_init_namespace();
        if (err == NO_ERROR && have_backend) {
            // 3. root busses from the host bridge devices in the namespace
            size_t count = 0;
            err = acpi_pci_enumerate_roots(pc_pci_add_acpi_root, &count);
            if (err != NO_ERROR) {
                printf("PCI: no host bridges found in the ACPI namespace (%d), using defaults\n",
                       err);
            }
            have_acpi_roots = (count > 0);
        }
    }
#endif

    if (!have_backend) {
        return;
    }

    // 4. probe everything (a default root covering segment 0 is created if none were added)
    pci_bus_mgr_init();

    // 5. firmware has assigned resources on a pc, keep them and only fill in what's missing,
    // which requires knowing the windows, so only when the roots came from ACPI.
    // pci.assign=all on the command line throws away firmware's work and reassigns everything
    // (a debug aid for the allocator), pci.assign=none leaves things exactly as found.
    if (have_acpi_roots) {
        char mode_str[8] = "";
        cmdline_get_string("pci.assign", mode_str, sizeof(mode_str), NULL);
        if (strcmp(mode_str, "none") == 0) {
            dprintf(INFO, "PCI: leaving firmware resource assignments untouched\n");
        } else if (strcmp(mode_str, "all") == 0) {
            printf("PCI: reassigning all resources from the ACPI windows (pci.assign=all)\n");
            pci_bus_mgr_assign_resources_mode(PCI_ASSIGN_ALL);
        } else {
            pci_bus_mgr_assign_resources_mode(PCI_ASSIGN_UNASSIGNED);
        }
    }

#if WITH_DEV_VIRTIO
    virtio_pci_init();
#endif
}

#endif // WITH_DEV_BUS_PCI
