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

#include <err.h>
#include <debug.h>
#include <trace.h>
#include <rand.h>

#include <dev/display.h>
#include <lib/gfx.h>
#include <platform/gpio.h>
#include <target/memory_lcd.h>

#define LOCAL_TRACE 0

SPI_HandleTypeDef SpiHandle;

#define MLCD_WIDTH  ((uint16_t)400)
#define MLCD_HEIGHT ((uint16_t)240)

#define MLCD_WR 0x01  // LCD Write Command
#define MLCD_CM 0x04  // LCD Clear Memory Command
#define MLCD_NO 0x00  // LCD No-op command

#define MLCD_BYTES_LINE       (MLCD_WIDTH / 8)
#define MLCD_BUF_SIZE         (MLCD_HEIGHT * MLCD_BYTES_LINE + 5)

#define VCOM_HI 0x02
#define VCOM_LO 0x00

#define XMIT_TIMEOUT_LONG  5000
#define XMIT_TIMEOUT_SHORT 1000

static uint8_t framebuffer[MLCD_HEIGHT][MLCD_WIDTH];
static uint8_t vcom_state;

static void chip_select(bool s)
{
	if (s) {
		gpio_set(GPIO(GPIO_PORT_B, 12), GPIO_PIN_SET);
	} else {
		gpio_set(GPIO(GPIO_PORT_B, 12), GPIO_PIN_RESET);
	}
}

static void lcd_power(bool s)
{
	if (s) {
		gpio_set(GPIO(GPIO_PORT_K, 6), GPIO_PIN_SET);
	} else {
		gpio_set(GPIO(GPIO_PORT_K, 6), GPIO_PIN_RESET);
	}
}

static void mlcd_clear(void)
{

    uint8_t clear[2];
    clear[0] = MLCD_CM;
    clear[1] = 0;

    chip_select(true);
    HAL_SPI_Transmit(&SpiHandle, clear, 2, XMIT_TIMEOUT_SHORT);
    chip_select(false);
}


status_t memory_lcd_init(void)
{
	SpiHandle.Instance               = SPI2;
	SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	SpiHandle.Init.Direction         = SPI_DIRECTION_1LINE;
	SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
	SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
	SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_LSB;
	SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial     = 7;
	SpiHandle.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle.Init.Mode 			 = SPI_MODE_MASTER;

	if (HAL_SPI_Init(&SpiHandle) != HAL_OK) {
		return ERR_GENERIC;
	}

	vcom_state = VCOM_LO;

	lcd_power(true);

    mlcd_clear();

	return NO_ERROR;
}



static void mlcd_flush(uint starty, uint endy)
{
	chip_select(true);

    uint8_t localbuf[MLCD_BUF_SIZE];
    uint8_t *bufptr = localbuf;

    // The first line is preceeded with a write command.
    *bufptr++ = MLCD_WR | vcom_state;

    vcom_state = vcom_state == VCOM_HI ? VCOM_LO : VCOM_HI;

    // Send the image data.
    for (uint i = starty; i <= endy; ++i) {
		*bufptr++ = (i + 1);  // Preceed each write with the line number.

		int ctr = 0;
		uint8_t outpix = 0;
		for (uint j = 0; j < MLCD_WIDTH; j++) {
			uint8_t pixval = framebuffer[i][j] > 128 ? 0xFF : 0x0;
			outpix |= (pixval & (0x1 << ctr++));
			if (ctr == 8) {
				*bufptr++ = outpix;
				ctr = 0;
				outpix = 0;
			}
		}

		*bufptr++ = outpix;
		*bufptr = 0x0;

		if (HAL_SPI_Transmit(&SpiHandle, localbuf, bufptr - localbuf, XMIT_TIMEOUT_LONG) != HAL_OK) {
    		goto finish;
    	}

    	bufptr = localbuf;
    }

    uint8_t trailer = 0;
    if (HAL_SPI_Transmit(&SpiHandle, &trailer, 1, XMIT_TIMEOUT_SHORT) != HAL_OK) {
		goto finish;
	}


finish:
	chip_select(false);
}

status_t display_get_info(struct display_info *info)
{
	LTRACEF("display_info %p\n", info);

	info->framebuffer = (void*)framebuffer;
	info->format = GFX_FORMAT_MONO;
	info->width = MLCD_WIDTH;
	info->height = MLCD_HEIGHT;
	info->stride = MLCD_WIDTH;
	info->flush = mlcd_flush;

	return NO_ERROR;
}
