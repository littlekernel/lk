/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

void riscv_early_init_percpu(void);
void riscv_init_percpu(void);
void riscv_boot_secondaries(void);

