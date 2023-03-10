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
// hart0_m_only is used to tell the PLIC to assume that hart0 only has machine
// mode, and occupies only one target on the PLIC. So far Sifive and other similar
// designs use this, whereas flatter designs do not. See plic.c for more details.
void plic_early_init(uintptr_t base, size_t num_irqs_, bool hart0_m_only);

void plic_init(void);

__END_CDECLS

