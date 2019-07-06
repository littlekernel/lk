/*
 * Copyright (c) 2016 Craig Stout <cstout@chromium.org>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// 1.28 Inch Monocromatic Sharp Memory LCD

#pragma once

#include <dev/display.h>

#define MLCD_WIDTH  ((uint16_t)128)
#define MLCD_HEIGHT ((uint16_t)128)

// Ensure width corresponds to an integral number of bytes
STATIC_ASSERT((MLCD_WIDTH & 0x3) == 0);

// 1 bit per pixel divided by 8 bits per byte
#define MLCD_BYTES_LINE  (MLCD_WIDTH / 8)
#define MLCD_FORMAT      (DISPLAY_FORMAT_MONO_1)

uint8_t lcd_get_line(struct display_image *image, uint8_t idx, uint8_t *result);
