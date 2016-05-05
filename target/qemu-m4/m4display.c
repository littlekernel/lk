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

#include <target/m4display.h>

#include <stdio.h>
#include <trace.h>
#include <err.h>

#include <stm32f4xx.h>
#include <stm32f4xx_spi.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>

#include <dev/display.h>
#include <dev/gpio.h>
#include <platform/gpio.h>
#include <assert.h>

#define LOCAL_TRACE 0

#define CMD_DISPLAY_NULL        0x00
#define CMD_DISPLAY_SET_PARAM   0x01
#define CMD_DISPLAY_OFF         0x02
#define CMD_DISPLAY_ON          0x03
#define CMD_DISPLAY_DRAW_SCENE  0x04
#define CMD_DISPLAY_FRAME_BEGIN 0x05

#define SCENE_BLACK  0x00
#define SCENE_SPLASH 0x01
#define SCENE_UPDATE 0x02
#define SCENE_ERROR  0x03

#define M4DISPLAY_WIDTH  180
#define M4DISPLAY_HEIGHT 180

static uint8_t framebuffer[M4DISPLAY_HEIGHT][M4DISPLAY_WIDTH];

static const char programming_header[] = "  Lattice\0iCEcube2 2014.08.26723\0Part: iCE40LP1K-CM36\0Date: Jan 30 2015 15:11:";

static void chip_select(bool val)
{
    if (val) {
        gpio_set(GPIO(GPIO_PORT_G, 8), true);
    } else {
        gpio_set(GPIO(GPIO_PORT_G, 8), false);
    }
}

static void reset(bool val)
{
    if (val) {
        gpio_set(GPIO(GPIO_PORT_G, 15), true);
    } else {
        gpio_set(GPIO(GPIO_PORT_G, 15), false);
    }
}

static void setup_pins(void)
{

    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // connect SPI6 pins to SPI alternate function
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI6);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI6);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI6);

    // enable clock for used IO pins
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

    /* Configure the chip select pin
       in this case we will use PE7 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIOE->BSRRL |= GPIO_Pin_7; // set PE7 high

    // Setup display CS Pin
    gpio_config(GPIO(GPIO_PORT_G, 8), GPIO_OUTPUT);

    // Setup display reset pin
    gpio_config(GPIO(GPIO_PORT_G, 15), GPIO_OUTPUT);

    // enable peripheral clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI6, ENABLE);
}

void init_display(void)
{
    // Setting up pins.
    setup_pins();

    SPI_InitTypeDef init_struct;

    init_struct.SPI_Direction           = SPI_Direction_1Line_Tx;
    init_struct.SPI_Mode                = SPI_Mode_Master;
    init_struct.SPI_DataSize            = SPI_DataSize_8b;
    init_struct.SPI_CPOL                = SPI_CPOL_Low;
    init_struct.SPI_CPHA                = SPI_CPHA_1Edge;
    init_struct.SPI_NSS                 = SPI_NSS_Hard;
    init_struct.SPI_BaudRatePrescaler   = SPI_BaudRatePrescaler_8;
    init_struct.SPI_FirstBit            = SPI_FirstBit_MSB;

    SPI_Init(SPI6, &init_struct);

    SPI_Cmd(SPI6, ENABLE); // enable SPI6

    chip_select(true);
    reset(true);
    chip_select(false);

    const uint8_t draw_splash_cmds[] = { 0x04, 0x01 };

    for (size_t i = 0; i < countof(draw_splash_cmds); i++) {
        SPI_I2S_SendData(SPI6, draw_splash_cmds[i]);
    }

    chip_select(true);
    chip_select(false);

    uint8_t enable_display_cmds[] = { 0x03 };

    for (size_t i = 0; i < countof(enable_display_cmds); i++) {
        SPI_I2S_SendData(SPI6, enable_display_cmds[i]);
    }

    chip_select(true);
    reset(false);
    chip_select(false);
    reset(true);


    for (size_t i = 0; i < countof(programming_header); i++) {
        SPI_I2S_SendData(SPI6, programming_header[i]);
    }

    chip_select(true);
}

static void s4lcd_flush(uint starty, uint endy)
{

    chip_select(false);

    uint8_t frame_begin_cmd[] = { 0x05 };

    for (size_t i = 0; i < countof(frame_begin_cmd); i++) {
        SPI_I2S_SendData(SPI6, frame_begin_cmd[i]);
    }

    size_t msb_idx = M4DISPLAY_WIDTH / 2;

    uint8_t scramble_buf[M4DISPLAY_WIDTH];
    for (int i = 0; i < M4DISPLAY_HEIGHT; i++) {
        uint8_t *lsb = scramble_buf;
        uint8_t *msb = &(scramble_buf[msb_idx]);
        for (int j = 0; j < M4DISPLAY_WIDTH; j += 2) {

            *msb++ = ((framebuffer[i][j] & 0b01010100) | ((framebuffer[i][j+1] & 0b01010100) << 1)) >> 2;
            *lsb++ = ((framebuffer[i][j] & 0b10101000) | ((framebuffer[i][j+1] & 0b10101000) >> 1)) >> 2;
        }

        for (int j = 0; j < M4DISPLAY_WIDTH; j++) {
            SPI_I2S_SendData(SPI6, scramble_buf[j]);
        }
    }

    chip_select(true);
}

status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    DEBUG_ASSERT(fb);
    LTRACEF("display_get_framebuffer %p\n", fb);

    fb->image.pixels = (void *)framebuffer;
    fb->image.format = IMAGE_FORMAT_RGB_2220;
    fb->image.width = M4DISPLAY_WIDTH;
    fb->image.height = M4DISPLAY_HEIGHT;
    fb->image.stride = M4DISPLAY_WIDTH;
    fb->image.rowbytes = M4DISPLAY_WIDTH;
    fb->flush = s4lcd_flush;
    fb->format = DISPLAY_FORMAT_UNKNOWN; //TODO

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    DEBUG_ASSERT(info);
    LTRACEF("display_info %p\n", info);

    info->format = DISPLAY_FORMAT_UNKNOWN; //TODO
    info->width = M4DISPLAY_WIDTH;
    info->height = M4DISPLAY_HEIGHT;

    return NO_ERROR;
}

status_t display_present(struct display_image *image, uint starty, uint endy)
{
  TRACEF("display_present - not implemented");
  DEBUG_ASSERT(false);
  return NO_ERROR;
}
