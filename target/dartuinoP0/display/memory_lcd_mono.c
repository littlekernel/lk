/*
 * Copyright (c) 2016 Craig Stout <cstout@chromium.org>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#if defined (LCD_LS027B7DH01)
#include <target/display/LS027B7DH01.h>
#elif defined (LCD_LS013B7DH03)
#include <target/display/LS013B7DH03.h>
#else
#error Undefined display header
#endif

#include <string.h>
#include <assert.h>

#define SET_BIT(BUF, BITNUM) ((BUF)[(BITNUM) >> 3] |= (0x1 << ((BITNUM) & 0x07)))

uint8_t lcd_get_line(struct display_image *image, uint8_t idx, uint8_t *result) {
    uint8_t *framebuffer = (uint8_t *) image->pixels + image->rowbytes * idx;

    if (image->format == IMAGE_FORMAT_MONO_1) {
        memcpy(result, framebuffer, MLCD_BYTES_LINE);
    } else if (image->format == IMAGE_FORMAT_MONO_8) {
        memset(result, 0, MLCD_BYTES_LINE);
        for (uint i = 0; i < MLCD_WIDTH; ++i) {
            if (framebuffer[i] > 128) {
                SET_BIT(result, i);
            }
        }
    } else {
        DEBUG_ASSERT(false);
    }
    return MLCD_BYTES_LINE;
}
