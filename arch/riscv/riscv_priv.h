/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

void riscv_early_init_percpu(void);
void riscv_init_percpu(void);
void riscv_boot_secondaries(void);
void riscv_configure_percpu_mp_early(uint hart_id, uint cpu_num);
void riscv_early_mmu_init(void);
void riscv_mmu_init(void);
void riscv_mmu_init_secondaries(void);

__END_CDECLS
