/*
 * Copyright (c) 2015 Carlos Pizano-Uribe <cpu@chromium.org>
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
/*
 * COPYRIGHT(c) 2015 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include <assert.h>
#include <err.h>
#include <debug.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <string.h>
#include <dev/gpio.h>
#include <dev/display.h>
#include <kernel/novm.h>
#include <platform/stm32.h>

/*
 * lcd initialization sequence, taken from
 * STM32Cube_FW_F7_V1.1.0/Drivers/BSP/STM32746G_Disco/stm32746g_discovery_lcd.[ch]
 *
 * This code only applies to The RK043FN48H LCD 480x272.
 */

/**
  * @brief  RK043FN48H Size
  */
#define  RK043FN48H_WIDTH    ((uint16_t)480)    /* LCD PIXEL WIDTH            */
#define  RK043FN48H_HEIGHT   ((uint16_t)272)    /* LCD PIXEL HEIGHT           */

/**
  * @brief  RK043FN48H Timing
  */
#define  RK043FN48H_HSYNC      ((uint16_t)41)   /* Horizontal synchronization */
#define  RK043FN48H_HBP        ((uint16_t)13)   /* Horizontal back porch      */
#define  RK043FN48H_HFP        ((uint16_t)32)   /* Horizontal front porch     */
#define  RK043FN48H_VSYNC      ((uint16_t)10)   /* Vertical synchronization   */
#define  RK043FN48H_VBP        ((uint16_t)2)    /* Vertical back porch        */
#define  RK043FN48H_VFP        ((uint16_t)2)    /* Vertical front porch       */


/**
  * @brief LCD special pins
  */
/* Display enable pin */
#define LCD_DISP_PIN            GPIO_PIN_12
#define LCD_DISP_GPIO_PORT      GPIOI
/* Backlight control pin */
#define LCD_BL_CTRL_PIN         GPIO_PIN_3
#define LCD_BL_CTRL_GPIO_PORT   GPIOK

#define LCD_DISP_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOI_CLK_ENABLE()
#define LCD_DISP_GPIO_CLK_DISABLE()     __HAL_RCC_GPIOI_CLK_DISABLE()
#define LCD_BL_CTRL_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOK_CLK_ENABLE()
#define LCD_BL_CTRL_GPIO_CLK_DISABLE()   __HAL_RCC_GPIOK_CLK_DISABLE()

/**
  * @brief  RK043FN48H frequency divider
  */
#define  RK043FN48H_FREQUENCY_DIVIDER    5      /* LCD Frequency divider      */

/** @defgroup STM32746G_DISCOVERY_LCD_Exported_Constants
  * @{
  */
#define MAX_LAYER_NUMBER       ((uint32_t)2)

#define LCD_LayerCfgTypeDef    LTDC_LayerCfgTypeDef

#define LTDC_ACTIVE_LAYER        ((uint32_t)1)   /* Layer 1 */
/**
  * @brief  LCD status structure definition
  */
#define LCD_OK                 ((uint8_t)0x00)
#define LCD_ERROR              ((uint8_t)0x01)
#define LCD_TIMEOUT            ((uint8_t)0x02)


static LTDC_HandleTypeDef  ltdc_handle;

/* Default LCD configuration with LCD Layer 1 */
static uint32_t active_layer = 0;

/**
  * @brief  Gets the LCD X size.
  * @retval Used LCD X size
  */
static uint32_t BSP_LCD_GetXSize(void)
{
    return ltdc_handle.LayerCfg[active_layer].ImageWidth;
}

/**
  * @brief  Gets the LCD Y size.
  * @retval Used LCD Y size
  */
static uint32_t BSP_LCD_GetYSize(void)
{
    return ltdc_handle.LayerCfg[active_layer].ImageHeight;
}

static size_t BSP_LCD_PixelSize(void)
{
    return (ltdc_handle.LayerCfg[active_layer].PixelFormat ==
            LTDC_PIXEL_FORMAT_ARGB8888) ? 4 : 2;
}

/**
  * @brief  Initializes the LCD layer in ARGB8888 format (32 bits per pixel).
  * @param  LayerIndex: Layer foreground or background
  * @param  FB_Address: Layer frame buffer
  * @retval None
  */
static void BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address)
{
    LCD_LayerCfgTypeDef  layer_cfg;

    /* Layer Init */
    layer_cfg.WindowX0 = 0;
    layer_cfg.WindowX1 = BSP_LCD_GetXSize();
    layer_cfg.WindowY0 = 0;
    layer_cfg.WindowY1 = BSP_LCD_GetYSize();
    layer_cfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
    layer_cfg.FBStartAdress = FB_Address;
    layer_cfg.Alpha = 255;
    layer_cfg.Alpha0 = 0;
    layer_cfg.Backcolor.Blue = 0;
    layer_cfg.Backcolor.Green = 0;
    layer_cfg.Backcolor.Red = 0;
    layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer_cfg.ImageWidth = BSP_LCD_GetXSize();
    layer_cfg.ImageHeight = BSP_LCD_GetYSize();

    HAL_LTDC_ConfigLayer(&ltdc_handle, &layer_cfg, LayerIndex);
}

/**
  * @brief  Selects the LCD Layer.
  * @param  LayerIndex: Layer foreground or background
  * @retval None
  */
void BSP_LCD_SelectLayer(uint32_t LayerIndex)
{
    active_layer = LayerIndex;
}

/**
  * @brief  Sets an LCD Layer visible
  * @param  LayerIndex: Visible Layer
  * @param  State: New state of the specified layer
  *          This parameter can be one of the following values:
  *            @arg  ENABLE
  *            @arg  DISABLE
  * @retval None
  */
void BSP_LCD_SetLayerVisible(uint32_t LayerIndex, FunctionalState State)
{
    if (State == ENABLE) {
        __HAL_LTDC_LAYER_ENABLE(&ltdc_handle, LayerIndex);
    } else {
        __HAL_LTDC_LAYER_DISABLE(&ltdc_handle, LayerIndex);
    }
    __HAL_LTDC_RELOAD_CONFIG(&ltdc_handle);
}

/**
  * @brief  Sets an LCD layer frame buffer address.
  * @param  LayerIndex: Layer foreground or background
  * @param  Address: New LCD frame buffer value
  * @retval None
  */
void BSP_LCD_SetLayerAddress(uint32_t LayerIndex, uint32_t Address)
{
    HAL_LTDC_SetAddress(&ltdc_handle, Address, LayerIndex);
}

/**
  * @brief  Enables the display.
  * @retval None
  */
void BSP_LCD_DisplayOn(void)
{
    /* Display On */
    __HAL_LTDC_ENABLE(&ltdc_handle);
    HAL_GPIO_WritePin(LCD_DISP_GPIO_PORT, LCD_DISP_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
}

/**
  * @brief  Disables the display.
  * @retval None
  */
void BSP_LCD_DisplayOff(void)
{
    /* Display Off */
    __HAL_LTDC_DISABLE(&ltdc_handle);
    HAL_GPIO_WritePin(LCD_DISP_GPIO_PORT, LCD_DISP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  Initializes the LTDC MSP.
  * @param  hltdc: LTDC handle
  * @param  Params
  * @retval None
  */
static void BSP_LCD_MspInit(LTDC_HandleTypeDef *hltdc, void *Params)
{
    GPIO_InitTypeDef gpio_init_structure;

    /* Enable the LTDC and DMA2D clocks */
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_DMA2D_CLK_ENABLE();

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();
    LCD_DISP_GPIO_CLK_ENABLE();
    LCD_BL_CTRL_GPIO_CLK_ENABLE();

    /*** LTDC Pins configuration ***/
    /* GPIOE configuration */
    gpio_init_structure.Pin       = GPIO_PIN_4;
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Speed     = GPIO_SPEED_FAST;
    gpio_init_structure.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOE, &gpio_init_structure);

    /* GPIOG configuration */
    gpio_init_structure.Pin       = GPIO_PIN_12;
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOG, &gpio_init_structure);

    /* GPIOI LTDC alternate configuration */
    gpio_init_structure.Pin       = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | \
                                    GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOI, &gpio_init_structure);

    /* GPIOJ configuration */
    gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                                    GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | \
                                    GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
                                    GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOJ, &gpio_init_structure);

    /* GPIOK configuration */
    gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | \
                                    GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOK, &gpio_init_structure);

    /* LCD_DISP GPIO configuration
       Note that LCD_DISP pin has to be manually controlled.
     */
    gpio_init_structure.Pin       = LCD_DISP_PIN;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LCD_DISP_GPIO_PORT, &gpio_init_structure);

    /* LCD_BL_CTRL GPIO configuration
       Note that LCD_BL_CTRL pin has to be manually controlled.
     */
    gpio_init_structure.Pin       = LCD_BL_CTRL_PIN;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &gpio_init_structure);
}

/**
  * @brief  Clock Config.
  * @param  hltdc: LTDC handle
  * @param  Params
  * @note   This API is called by BSP_LCD_Init()
  * @retval None
  */
static void BSP_LCD_ClockConfig(LTDC_HandleTypeDef *hltdc, void *Params)
{
    static RCC_PeriphCLKInitTypeDef  periph_clk_init_struct;

    /* RK043FN48H LCD clock configuration */
    /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */
    /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */
    periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    periph_clk_init_struct.PLLSAI.PLLSAIN = 192;
    periph_clk_init_struct.PLLSAI.PLLSAIR = RK043FN48H_FREQUENCY_DIVIDER;
    periph_clk_init_struct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
    HAL_RCCEx_PeriphCLKConfig(&periph_clk_init_struct);
}

/**
  * @brief  Initializes the LCD.
  * @retval LCD state
  */
uint8_t BSP_LCD_Init(void)
{
    /* Timing Configuration */
    ltdc_handle.Init.HorizontalSync = (RK043FN48H_HSYNC - 1);
    ltdc_handle.Init.VerticalSync = (RK043FN48H_VSYNC - 1);
    ltdc_handle.Init.AccumulatedHBP = (RK043FN48H_HSYNC + RK043FN48H_HBP - 1);
    ltdc_handle.Init.AccumulatedVBP = (RK043FN48H_VSYNC + RK043FN48H_VBP - 1);
    ltdc_handle.Init.AccumulatedActiveH = (RK043FN48H_HEIGHT + RK043FN48H_VSYNC + RK043FN48H_VBP - 1);
    ltdc_handle.Init.AccumulatedActiveW = (RK043FN48H_WIDTH + RK043FN48H_HSYNC + RK043FN48H_HBP - 1);
    ltdc_handle.Init.TotalHeigh = (RK043FN48H_HEIGHT + RK043FN48H_VSYNC + RK043FN48H_VBP + RK043FN48H_VFP - 1);
    ltdc_handle.Init.TotalWidth = (RK043FN48H_WIDTH + RK043FN48H_HSYNC + RK043FN48H_HBP + RK043FN48H_HFP - 1);

    /* LCD clock configuration */
    BSP_LCD_ClockConfig(&ltdc_handle, NULL);

    /* Initialize the LCD pixel width and pixel height */
    ltdc_handle.LayerCfg->ImageWidth  = RK043FN48H_WIDTH;
    ltdc_handle.LayerCfg->ImageHeight = RK043FN48H_HEIGHT;

    /* Background value */
    ltdc_handle.Init.Backcolor.Blue = 0;
    ltdc_handle.Init.Backcolor.Green = 0;
    ltdc_handle.Init.Backcolor.Red = 0;

    /* Polarity */
    ltdc_handle.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    ltdc_handle.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    ltdc_handle.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    ltdc_handle.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    ltdc_handle.Instance = LTDC;

    if (HAL_LTDC_GetState(&ltdc_handle) == HAL_LTDC_STATE_RESET) {
        BSP_LCD_MspInit(&ltdc_handle, NULL);
    }

    HAL_LTDC_Init(&ltdc_handle);

    /* allocate the framebuffer */
    size_t fb_size_pages = PAGE_ALIGN(RK043FN48H_WIDTH * RK043FN48H_HEIGHT * 4) / PAGE_SIZE;
    void *fb_address = novm_alloc_pages(fb_size_pages, NOVM_ARENA_SECONDARY);
    if (!fb_address)
        panic("failed to allocate framebuffer for LCD\n");

    BSP_LCD_LayerDefaultInit(0, (uint32_t)fb_address);
    BSP_LCD_SelectLayer(0);

    /* clear framebuffer */
    memset((void *)ltdc_handle.LayerCfg[active_layer].FBStartAdress, 0,
           BSP_LCD_GetXSize() * BSP_LCD_GetYSize() * BSP_LCD_PixelSize());

    /* turn the display on */
    BSP_LCD_DisplayOn();
    return LCD_OK;
}

/* LK display (lib/gfx.h) calls this function */
status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    fb->image.pixels = (void *)ltdc_handle.LayerCfg[active_layer].FBStartAdress;

    if (ltdc_handle.LayerCfg[active_layer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888) {
        fb->format = DISPLAY_FORMAT_ARGB_8888;
        fb->image.format = IMAGE_FORMAT_ARGB_8888;
        fb->image.rowbytes = BSP_LCD_GetXSize() * 4;
    } else if (ltdc_handle.LayerCfg[active_layer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) {
        fb->format = DISPLAY_FORMAT_RGB_565;
        fb->image.format = IMAGE_FORMAT_RGB_565;
        fb->image.rowbytes = BSP_LCD_GetXSize() * 2;
    } else {
        panic("unhandled pixel format\n");
        return ERR_NOT_FOUND;
    }

    fb->image.width = BSP_LCD_GetXSize();
    fb->image.height = BSP_LCD_GetYSize();
    fb->image.stride = BSP_LCD_GetXSize();
    fb->flush = NULL;

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    if (ltdc_handle.LayerCfg[active_layer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888) {
        info->format = DISPLAY_FORMAT_ARGB_8888;
    } else if (ltdc_handle.LayerCfg[active_layer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) {
        info->format = DISPLAY_FORMAT_RGB_565;
    } else {
        panic("unhandled pixel format\n");
        return ERR_NOT_FOUND;
    }

    info->width = BSP_LCD_GetXSize();
    info->height = BSP_LCD_GetYSize();

    return NO_ERROR;
}

status_t display_present(struct display_image *image, uint starty, uint endy)
{
  TRACEF("display_present - not implemented");
  DEBUG_ASSERT(false);
  return NO_ERROR;
}
