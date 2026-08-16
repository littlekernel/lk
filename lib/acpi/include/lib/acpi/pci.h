// Copyright 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

// PCI related ACPI helpers: host bridge (root) enumeration from the namespace and legacy
// interrupt routing through _PRT. Platform independent; the platform glue turns the roots
// into pci_bus_mgr_add_root() calls and the routed interrupts into vectors.

#include <dev/bus/pci.h>
#include <lk/compiler.h>
#include <sys/types.h>
#include <uacpi/namespace.h>
#include <uacpi/utilities.h>

__BEGIN_CDECLS

// a legacy (INTx) interrupt as routed by ACPI
struct acpi_pci_irq {
    uint32_t gsi;
    bool level_triggered;
    bool active_low;
};

#define ACPI_PCI_ROOT_MAX_WINDOWS 16

// A PCI host bridge (PNP0A03 / PNP0A08 device) found in the namespace.
struct acpi_pci_root {
    uacpi_namespace_node *node;

    // from _SEG and _BBN / the bus number descriptor in _CRS
    uint16_t segment;
    uint8_t bus_start;
    uint8_t bus_end;

    // address windows the bridge produces, from _CRS
    struct pci_root_window windows[ACPI_PCI_ROOT_MAX_WINDOWS];
    size_t num_windows;

    // the interrupt routing table, once acpi_pci_root_load_prt() has run
    uacpi_pci_routing_table *prt;
};

// Called once per root found. The root object is heap allocated and belongs to the callee.
typedef void (*acpi_pci_root_callback)(struct acpi_pci_root *root, void *cookie);

// Walk the namespace for PCI host bridges. Requires the namespace to be loaded. Roots are
// reported sorted by segment and starting bus, with bus ranges clamped so that they do not
// overlap when the firmware only provided _BBN.
status_t acpi_pci_enumerate_roots(acpi_pci_root_callback callback, void *cookie);

// Tell the firmware which interrupt model the OS uses (_PIC). Must be called before any _PRT is
// evaluated. Requires the namespace to be loaded.
status_t acpi_pci_set_interrupt_model(bool ioapic);

// Evaluate and cache the root's _PRT. Returns ERR_NOT_FOUND if the root has none.
status_t acpi_pci_root_load_prt(struct acpi_pci_root *root);

// Route pin (1..4 for INTA..INTD) of device dev on the root bus to a global system interrupt,
// resolving interrupt link devices as needed. Loads the _PRT on first use.
status_t acpi_pci_root_route_intx(struct acpi_pci_root *root, unsigned int dev,
                                  unsigned int pin, struct acpi_pci_irq *out);

// Debug dump of a root
void acpi_pci_root_dump(const struct acpi_pci_root *root);

__END_CDECLS
