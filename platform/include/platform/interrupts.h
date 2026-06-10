/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <sys/types.h>

#if WITH_DEV_BUS_PCI
#include <dev/bus/pci.h>
#endif

__BEGIN_CDECLS

/* Routines implemented by the platform or system specific interrupt controller
 * to allow for installation and masking/unmask of interrupt vectors.
 *
 * Some platforms do not allow for dynamic registration.
 */
status_t mask_interrupt(unsigned int vector);
status_t unmask_interrupt(unsigned int vector);

typedef enum handler_return (*int_handler)(void *arg);
void register_int_handler(unsigned int vector, int_handler handler, void *arg);

/* Register a MSI interrupt handler. Basically the same as register_int_handler, except
 * interrupt controller may have additional setup.
 */
void register_int_handler_msi(unsigned int vector, int_handler handler, void *arg, bool edge);

/* Allocate a run of interrupts with alignment log2 in a platform specific way.
 * Used for PCI MSI and possibly other use cases.
 */
status_t platform_allocate_interrupts(size_t count, uint align_log2, bool msi, unsigned int *vector);

#if WITH_DEV_BUS_PCI
/* Map a PCI INTERRUPT_LINE value (legacy IRQ line, typically 0..15) to a
 * platform interrupt vector.
 *
 * Full PCI BDF context is provided so platform code can implement routing
 * policy however needed.
 */
status_t platform_pci_int_line_to_vector(unsigned int pci_int_line, pci_location_t loc,
        unsigned int *vector);

/* Map a PCI INTERRUPT_PIN value (1..4 for INTA..INTD) to a platform interrupt
 * vector. Platforms that route legacy interrupts using swizzle logic should
 * implement this entry point.
 */
status_t platform_pci_int_pin_to_vector(unsigned int pci_int_pin, pci_location_t loc,
        unsigned int *vector);
#endif

/* Ask the platform to compute for us the value to stuff in the MSI address and data fields. */
status_t platform_compute_msi_values(unsigned int vector, unsigned int cpu, bool edge,
        uint64_t *msi_address_out, uint16_t *msi_data_out);

__END_CDECLS
