/*
 * Copyright (c) 2015 Gurjant Kalsi <me@gurjantkalsi.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// 1.33 Inch 3-Bit RGB Sharp Color LCD
#include <target/display/LS013B7DH06.h>
#include <string.h>

#define SET_BIT(BUF, BITNUM) ((BUF)[(BITNUM) >> 3] |= (0xff & (0x1 << ((BITNUM) & 0x07))))

uint8_t lcd_get_line(uint8_t *framebuffer, uint8_t idx, uint8_t *result)
{
    framebuffer += MLCD_WIDTH * idx;

    memset(result, 0, MLCD_BYTES_LINE);

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
    return MLCD_BYTES_LINE;
}