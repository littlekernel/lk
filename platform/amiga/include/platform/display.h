/*
 * Copyright (c) 2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

enum { W = 320,
       H = 256,
       // Round up width to next multiple. bitplane DMA is word-aligned (16 pixels).
       BYTES_PER_ROW = (((W + 15) & ~15) / 8),
       BPL_BYTES = (BYTES_PER_ROW * H) };

void make_copper_list(uint8_t *bpl);
void platform_init_display(void);
void init_display(void);
