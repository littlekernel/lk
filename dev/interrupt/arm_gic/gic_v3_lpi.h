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

#include "arm_gic_common.h"

// GICv3 LPIs: message based interrupts numbered from 8192, configured by a property table in
// memory (one byte each: priority and enable) and tracked pending in a per cpu table. They only
// arrive through an ITS, so this layer is set up only when there is one (see gic_v3_its.h).
// arm64 only.

#define GIC_LPI_BASE 8192

#ifndef ARM_GIC_MAX_LPIS
#define ARM_GIC_MAX_LPIS 256 // LPIs LK hands out, 8192..8192+ARM_GIC_MAX_LPIS-1
#endif

// Allocate the property and pending tables, once, from the boot cpu. Fails (saying why) when
// the distributor has no LPI support.
status_t arm_gicv3_lpi_init(void);
bool arm_gicv3_lpi_ready(void);

// Point this cpu's redistributor at the tables and enable LPIs on it. Run on each cpu after
// arm_gicv3_lpi_init(); a cpu whose redistributor cannot take LPIs is skipped with a message.
status_t arm_gicv3_lpi_init_percpu(uint cpu);
bool arm_gicv3_lpi_cpu_enabled(uint cpu);

// The value an ITS command's RDbase field takes for cpu: the redistributor's physical address
// >> 16 when the ITS's GITS_TYPER.PTA is set, its processor number otherwise.
uint64_t arm_gicv3_lpi_rdbase(uint cpu, bool pta);

// Hand out and return LPI INTIDs. A new LPI starts disabled.
status_t arm_gicv3_lpi_alloc(uint *intid);
void arm_gicv3_lpi_free(uint intid);

// Set an LPI's enable bit in the property table and make the write visible to the GIC. The
// redistributor only rereads it on an ITS INV or INVALL, which the caller sends.
void arm_gicv3_lpi_set_enable(uint intid, bool enable);

// The handler slot of an allocated LPI, NULL for anything else.
struct int_handler_struct *arm_gicv3_lpi_handler(uint intid);
