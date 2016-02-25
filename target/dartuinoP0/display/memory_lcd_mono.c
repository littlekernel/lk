/*
 * Copyright (c) 2016 Craig Stout <cstout@chromium.org>
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

#if defined (LCD_LS027B7DH01)
#include <target/display/LS027B7DH01.h>
#elif defined (LCD_LS013B7DH03)
#include <target/display/LS013B7DH03.h>
#else
#error Undefined display header
#endif

#include <string.h>
#include <assert.h>

#define SET_BIT(BUF, BITNUM) ((BUF)[(BITNUM) >> 3] |= (0xff & (0x1 << ((BITNUM) & 0x07))))

uint8_t lcd_get_line(uint8_t *framebuffer, uint8_t idx, uint8_t *result)
{
    framebuffer += FB_STRIDE * idx;

#if FB_FORMAT == DISPLAY_FORMAT_MONO_1
    memcpy(result, framebuffer, MLCD_BYTES_LINE);

#elif FB_FORMAT == DISPLAY_FORMAT_MONO_8
    memset(result, 0, MLCD_BYTES_LINE);
    for (uint i = 0; i < MLCD_WIDTH; ++i) {
        if (framebuffer[i] > 128) {
            SET_BIT(result, i);
        }
    }

#else
#error Unhandled FB_FORMAT
#endif

    return MLCD_BYTES_LINE;
}
