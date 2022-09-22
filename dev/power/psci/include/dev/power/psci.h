/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

extern int psci_call(ulong arg0, ulong arg1, ulong arg2, ulong arg3);
extern uint32_t psci_version(void);
extern int psci_cpu_on(int corenr, ulong entrypoint);
extern void psci_system_off(void);
extern void psci_system_reset(void);

