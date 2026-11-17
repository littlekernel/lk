/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

// GICv3 ITS: translates a device's MSI write (a device id from the bus, the written data as an
// event id) into an LPI. Each cpu's redistributor is a collection; a device gets an
// interrupt translation table (ITT) of events, each mapped to an LPI and a collection.
// Commands go through a queue in memory. arm64 only, and only the first ITS is used.

#define ARM_GIC_ITS_EVENTS_PER_DEVICE 32
#define ARM_GIC_ITS_MAX_DEVICES       32

// Set up the ITS at paddr (mapped at vaddr), after arm_gicv3_lpi_init(). Fails, saying why,
// if its tables or command queue cannot be set up; MSIs through it then fail too.
status_t arm_gicv3_its_init(paddr_t paddr, vaddr_t vaddr);
bool arm_gicv3_its_ready(void);
paddr_t arm_gicv3_its_paddr(void);

// Map this cpu's collection to its redistributor. Run on each cpu once LPIs are enabled on it.
void arm_gicv3_its_init_percpu(uint cpu);

// One MSI for device_id: an LPI routed to cpu 0's collection, disabled until unmasked, and
// the message (GITS_TRANSLATER, event id) that raises it. ERR_NOT_FOUND if paddr is not the ITS.
status_t arm_gicv3_its_msi_alloc(paddr_t paddr, uint32_t device_id, uint *vector,
                                 uint64_t *msg_addr, uint32_t *msg_data);
status_t arm_gicv3_its_msi_free(paddr_t paddr, uint32_t device_id, uint vector);

// Make a property table change to an LPI from this ITS take effect (INV).
status_t arm_gicv3_its_lpi_invalidate(uint intid);

// For the unit tests: raise an LPI as if its device had written its message (INT), and move
// it to another cpu's collection (MOVI).
status_t arm_gicv3_its_inject(uint intid);
status_t arm_gicv3_its_move(uint intid, uint cpu);
