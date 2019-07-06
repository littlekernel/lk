/*
 * Copyright (c) 2015 Gurjant Kalsi <me@gurjantkalsi.com>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// 1.33 Inch 3-Bit RGB Sharp Color LCD
#include <target/display/LS013B7DH06.h>
#include <string.h>
#include <assert.h>

#define SET_BIT(BUF, BITNUM) ((BUF)[(BITNUM) >> 3] |= (0x1 << ((BITNUM) & 0x07)))

uint8_t lcd_get_line(struct display_image *image, uint8_t idx, uint8_t *result) {
    uint8_t *framebuffer = (uint8_t *)image->pixels + image->rowbytes * idx;

    memset(result, 0, MLCD_BYTES_LINE);

    if (image->format == IMAGE_FORMAT_RGB_332) {
        for (int i = 0; i < MLCD_WIDTH; ++i) {
            uint8_t inpix = framebuffer[i];

            int j = i * 3;

            if (inpix & 0x80) {
                SET_BIT(result, j);
            }
            if (inpix & 0x10) {
                SET_BIT(result, j + 1);
            }
            if (inpix & 0x02) {
                SET_BIT(result, j + 2);
            }
        }
    } else if (image->format == IMAGE_FORMAT_RGB_x111) {
        int j = 0;
        for (uint i = 0; i < image->width; i += 2) {
            uint8_t val = *framebuffer++;
            uint8_t inpix = val & 0xf;

            if (inpix & 0x4) {
                SET_BIT(result, j);
            }
            if (inpix & 0x2) {
                SET_BIT(result, j + 1);
            }
            if (inpix & 0x1) {
                SET_BIT(result, j + 2);
            }

            inpix = val >> 4;
            if (inpix & 0x4) {
                SET_BIT(result, j + 3);
            }
            if (inpix & 0x2) {
                SET_BIT(result, j + 4);
            }
            if (inpix & 0x1) {
                SET_BIT(result, j + 5);
            }
            j += 6;
        }
        if (image->width & 1) {
            uint8_t val = *framebuffer;
            uint8_t inpix = val & 0xf;

            if (inpix & 0x4) {
                SET_BIT(result, j);
            }
            if (inpix & 0x2) {
                SET_BIT(result, j + 1);
            }
            if (inpix & 0x1) {
                SET_BIT(result, j + 2);
            }
        }
    } else {
        DEBUG_ASSERT(false);
    }

    return MLCD_BYTES_LINE;
}
