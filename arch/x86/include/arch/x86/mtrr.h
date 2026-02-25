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

/*
 * MTRR (Memory Type Range Register) support for x86/x64.
 */

/* MTRR memory type values (bits 0-2 of PHYSBASE registers) */
#define MTRR_TYPE_UC 0 /* Uncached */
#define MTRR_TYPE_WC 1 /* Write-Combining */
#define MTRR_TYPE_WT 4 /* Write-Through */
#define MTRR_TYPE_WP 5 /* Write-Protected */
#define MTRR_TYPE_WB 6 /* Write-Back (cached) */

/* MTRR register layout */
#define MTRR_PHYSBASE_TYPE_MASK  0x00000007
#define MTRR_PHYSBASE_TYPE_SHIFT 0
#define MTRR_PHYSBASE_ADDR_MASK  0xFFFFFF00
#define MTRR_PHYSBASE_ADDR_SHIFT 8

#define MTRR_PHYSMASK_VALID       0x00000800
#define MTRR_PHYSMASK_VALID_SHIFT 11
#define MTRR_PHYSMASK_ADDR_MASK   0xFFFFFF00
#define MTRR_PHYSMASK_ADDR_SHIFT  8

#define MTRR_DEF_TYPE_ENABLE     0x00000800
#define MTRR_DEF_TYPE_FIX_ENABLE 0x00000400
#define MTRR_DEF_TYPE_TYPE_MASK  0x00000007

void x86_mtrr_early_init(void);
void x86_mtrr_early_init_percpu(void);
void x86_mtrr_init(void);

status_t x86_mtrr_set(int mtrr_index, uint64_t phys_base, uint64_t size, uint8_t type);
bool x86_mtrr_supported(void);
int x86_mtrr_count(void);
status_t x86_mtrr_set_framebuffer(uint64_t fb_phys_addr, uint64_t fb_size);
void x86_mtrr_dump(void);
