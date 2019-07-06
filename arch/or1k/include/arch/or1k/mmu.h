/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once
#define SECTION_SIZE            (16U*1024U*1024U)

#define OR1K_MMU_PG_FLAGS_MASK  0x7ffU
#define OR1K_MMU_PG_PRESENT     0x400
#define OR1K_MMU_PG_L           0x200
#define OR1K_MMU_PG_X           0x100
#define OR1K_MMU_PG_W           0x080
#define OR1K_MMU_PG_U           0x040
#define OR1K_MMU_PG_D           0x020
#define OR1K_MMU_PG_A           0x010
#define OR1K_MMU_PG_WOM         0x008
#define OR1K_MMU_PG_WBC         0x004
#define OR1K_MMU_PG_CI          0x002
#define OR1K_MMU_PG_CC          0x001
