/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

// Called at early initialization time, generally platform_early_init.
// Arguments are base address of mapped registers, number of fixed irqs,
// and a special third argument.
//
// Each hart's interrupt target (context) on the PLIC comes from plic_set_hart_context(),
// which a platform with a device tree calls before this, usually via fdtwalk_setup_plic().
// For harts with no recorded context the driver falls back to a fixed layout:
// hart0_m_only says hart0 only has machine mode and occupies a single target, as on
// SiFive and similar designs, whereas flatter designs give every hart an M and S target.
// See plic.c for more details.
void plic_early_init(uintptr_t base, size_t num_irqs_, bool hart0_m_only);

// Record the PLIC context that routes external interrupts for the current privilege
// mode to the given hart.
void plic_set_hart_context(unsigned int hart, unsigned int context);

void plic_init(void);

__END_CDECLS

