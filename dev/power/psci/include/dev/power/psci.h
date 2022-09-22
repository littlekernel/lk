/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

uint32_t psci_version(void);
int psci_cpu_on(int corenr, ulong entrypoint);
void psci_system_off(void);
void psci_system_reset(void);

__END_CDECLS
